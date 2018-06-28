#pragma once

#include <js_engine/js_to_native_converter.h>
#include <js_engine/native_to_js_converter.h>
#include <js_engine/converter_utils.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>

#include <type_traits>
#include <vector>

#define MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT(baseClass, functionName, functionWithOptName, optArgCount) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        bool bRet = \
            InvokeNativeCallback<optArgCount>( cx, &baseClass::functionName, &baseClass::functionWithOptName, argc, vp );\
        if (!bRet)\
        {\
            std::string innerErrorText(mozjs::GetCurrentExceptionText(cx));\
            if (!innerErrorText.empty())\
            {\
                std::string tmpString = ": \n";\
                tmpString += innerErrorText;\
                innerErrorText.swap( tmpString );\
            }\
            JS_ReportErrorASCII( cx, "'%s' failed%s", #functionName, innerErrorText.c_str() ); \
        }\
        return bRet;\
    }

#define MJS_DEFINE_JS_TO_NATIVE_FN(baseClass, functionName) \
    MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT(baseClass, functionName, functionName, 0 )

#define MJS_WRAP_JS_TO_NATIVE_FN(functionName, functionImplName) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        bool bRet = functionImplName(cx, argc, vp);\
        if (!bRet)\
        {\
            std::string innerErrorText(mozjs::GetCurrentExceptionText(cx));\
            if (!innerErrorText.empty())\
            {\
                std::string tmpString = ": \n";\
                tmpString += innerErrorText;\
                innerErrorText.swap( tmpString );\
            }\
            JS_ReportErrorASCII( cx, "'%s' failed%s", #functionName, innerErrorText.c_str() ); \
        }\
        return bRet;\
    }

namespace mozjs
{

template <size_t OptArgCount = 0, typename BaseClass, typename ReturnType, typename FuncOptType, typename ... ArgTypes>
bool InvokeNativeCallback( JSContext* cx,
                           ReturnType( BaseClass::*fn )( ArgTypes ... ),
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    return InvokeNativeCallback_Impl<
        OptArgCount,
        BaseClass,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}

template <size_t OptArgCount = 0, typename BaseClass, typename ReturnType, typename FuncOptType, typename ... ArgTypes>
bool InvokeNativeCallback( JSContext* cx,
                           ReturnType( BaseClass::*fn )( ArgTypes ... ) const,
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    return InvokeNativeCallback_Impl<
        OptArgCount,
        BaseClass,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}


template <size_t OptArgCount, typename BaseClass, typename ReturnType, typename FuncType, typename FuncOptType, typename ... ArgTypes>
bool InvokeNativeCallback_Impl( JSContext* cx,
                                FuncType fn,
                                FuncOptType fnWithOpt,
                                unsigned argc, JS::Value* vp )
{
    constexpr size_t maxArgCount = sizeof ...( ArgTypes );

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    args.rval().setUndefined();

    if ( args.length() < ( maxArgCount - OptArgCount )
         || args.length() > maxArgCount )
    {
        JS_ReportErrorASCII( cx, "Invalid number of arguments" );
        return false;
    }

    // Parse arguments

    bool bRet = true;
    size_t failedIdx = 0;    
    auto callbackArguments =
        JsToNativeArguments<maxArgCount, ArgTypes...>(
            args,
            [&cx, &bRet, &failedIdx]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index )
            {
                using ArgType = typename decltype( argTypeStruct )::type;

                if constexpr( std::is_same_v<ArgType, JS::HandleValue> )
                {// Skip conversion, pass through
                    if ( index >= jsArgs.length() )
                    {// Dummy value
                        return jsArgs[0];
                    }
                    return jsArgs[index];
                }
                else 
                {
                    if ( index >= jsArgs.length() )
                    {
                        return ArgType();
                    }

                    auto& curArg = jsArgs[index];

                    if constexpr (convert::is_primitive_v<ArgType>)
                    {// Construct and copy
                        if ( !convert::to_native::IsValue<ArgType>( cx, curArg ) )
                        {
                            failedIdx = index;
                            bRet = false;
                            return ArgType();
                        }

                        return convert::to_native::ToValue<ArgType>( cx, curArg );
                    }
                    else if constexpr (std::is_pointer_v<ArgType>)
                    {// Extract native pointer
                        if ( !curArg.isObjectOrNull() )
                        {
                            failedIdx = index;
                            bRet = false;
                            return static_cast<ArgType>(nullptr);
                        }

                        JS::RootedObject jsObject( cx, curArg.toObjectOrNull() );
                        if ( !jsObject )
                        {// Not an error
                            return static_cast<ArgType>(nullptr);
                        }

                        if ( !JS_InstanceOf( cx, jsObject, &std::remove_pointer_t<ArgType>::GetClass(), nullptr ) )
                        {
                            failedIdx = index;
                            bRet = false;
                            return static_cast<ArgType>(nullptr);
                        }

                        return static_cast<ArgType>(JS_GetPrivate( jsObject ));
                    }
                    else
                    {
                        static_assert(0, "Unsupported argument type");
                    }
                }
            } );
    if ( !bRet )
    {
        JS_ReportErrorASCII( cx, "Argument #%d is of wrong type", failedIdx );
        return false;
    }

    // Call function

    BaseClass* baseClass = static_cast<BaseClass*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !baseClass )
    {
        JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    ReturnType retVal =
        InvokeNativeCallback_Call<!!OptArgCount, ReturnType>( baseClass, fn, fnWithOpt, callbackArguments, maxArgCount - args.length() );
    if ( !retVal )
    {
        return false;
    }

    // Return value

    if constexpr( std::is_same_v<ReturnType::value_type, JSObject*> )
    {// retVal.value() is a raw JS pointer! Be careful when editing this code!
        args.rval().setObjectOrNull( retVal.value() );
    }
    else if constexpr( std::is_same_v<ReturnType::value_type, nullptr_t> )
    {
        args.rval().setUndefined();
    }
    else
    {
        if ( !NativeToJsValue( cx, retVal.value(), args.rval() ) )
        {
            args.rval().setUndefined();

            JS_ReportErrorASCII( cx, "Internal error: failed to convert return value" );
            return false;
        }
    }

    return true;
}

template <typename T>
struct TypeWrapper
{
    using type = T;
};

template <size_t TupleSize, typename... ArgTypes, typename FuncType>
auto JsToNativeArguments( const JS::CallArgs& jsArgs, FuncType&& func )
{
    return JsToNativeArguments_Impl<ArgTypes...>( jsArgs, func, std::make_index_sequence<TupleSize>{} );
}

template <typename... ArgTypes, typename FuncType, size_t... Indexes>
auto JsToNativeArguments_Impl( const JS::CallArgs& jsArgs, FuncType&& func, std::index_sequence<Indexes...> )
{
    return std::make_tuple( func( jsArgs, TypeWrapper<ArgTypes>{}, Indexes ) ... );
}

template <
    bool HasOptArg,
    typename ReturnType,
    typename BaseClass,
    typename FuncType,
    typename FuncOptType,
    typename ArgTupleType
>
ReturnType InvokeNativeCallback_Call( BaseClass* baseClass,
                                      FuncType fn, FuncOptType fnWithOpt,
                                      const ArgTupleType& argTuple, size_t optArgCount )
{
    if constexpr( !HasOptArg )
    {
        return std::apply( fn, std::tuple_cat( std::make_tuple( baseClass ), argTuple ) );
    }
    else
    {// Invoke callback with optional argument handler
        return std::apply( fnWithOpt, std::tuple_cat( std::make_tuple( baseClass, optArgCount ), argTuple ) );
    }
}

}
