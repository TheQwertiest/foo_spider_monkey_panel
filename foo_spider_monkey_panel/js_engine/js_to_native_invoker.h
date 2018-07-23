#pragma once

#include <js_engine/js_to_native_converter.h>
#include <js_engine/native_to_js_converter.h>
#include <js_engine/converter_utils.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>

#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#   include <js/Wrapper.h>
#pragma warning( pop ) 


#include <type_traits>
#include <vector>

#define MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT(baseClass, functionName, functionWithOptName, optArgCount) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        bool bRet = \
            InvokeNativeCallback<optArgCount>( cx, &baseClass::functionName, &baseClass::functionWithOptName, argc, vp );\
        if (!bRet)\
        {\
            mozjs::RethrowExceptionWithFunctionName(cx, #functionName);\
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
            mozjs::RethrowExceptionWithFunctionName(cx, #functionName);\
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
    if ( args.length() < ( maxArgCount - OptArgCount ) )
    {
        JS_ReportErrorUTF8( cx, "Invalid number of arguments" );
        return false;
    }

    BaseClass* baseClass = InvokeNativeCallback_GetThisObject<BaseClass>( cx, args.thisv() );
    if ( !baseClass )
    {
        JS_ReportErrorUTF8( cx, "Invalid `this` context" );
        return false;
    }

    // Parse arguments

    bool bRet = true;
    size_t failedIdx = 0;    
    auto callbackArguments =
        JsToNativeArguments<maxArgCount, ArgTypes...>(
            args,
            [maxArgCount, &cx, &bRet, &failedIdx]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index )
            {                
                using ArgType = typename std::remove_const_t<std::remove_reference_t<decltype( argTypeStruct )::type>>;

                if constexpr( std::is_same_v<ArgType, JS::HandleValue> )
                {// Skip conversion, pass through
                    if ( index >= jsArgs.length() || index > maxArgCount )
                    {// Not an error: default value might be set in callback
                        return jsArgs[0]; ///< Dummy value
                    }
                    return jsArgs[index];
                }
                else 
                {
                    if ( index >= jsArgs.length() || index > maxArgCount )
                    {// Not an error: default value might be set in callback
                        return ArgType();
                    }

                    auto& curArg = jsArgs[index];

                    if constexpr (convert::to_native::is_convertable_v<ArgType>)
                    {// Construct and copy
                        auto retVal = convert::to_native::ToValue<ArgType>( cx, curArg );
                        if ( !retVal )
                        {
                            failedIdx = index;
                            bRet = false;
                            return ArgType();
                        }

                        return retVal.value();
                    }
                    else if constexpr (std::is_pointer_v<ArgType>)
                    {// Extract native pointer
                        // TODO: think if there is a good way to move this to convert::to_native
                        if ( !curArg.isObjectOrNull() )
                        {
                            failedIdx = index;
                            bRet = false;
                            return static_cast<ArgType>(nullptr);
                        }

                        if (curArg.isNull())
                        {// Not an error: null might be a valid argument                            
                            return static_cast<ArgType>(nullptr);
                        }

                        JS::RootedObject jsObject(cx, &curArg.toObject());
                        auto retVal = convert::to_native::ToValue<ArgType>(cx, jsObject);
                        if (!retVal )
                        {
                            failedIdx = index;
                            bRet = false;
                            return static_cast<ArgType>(nullptr);
                        }

                        return retVal.value();
                    }
                    else
                    {
                        static_assert(0, "Unsupported argument type");
                    }
                }
            } );
    if ( !bRet )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {// do not overwrite internal errors
            JS_ReportErrorUTF8( cx, "Argument #%d is of wrong type", failedIdx );
        }
        return false;
    }

    // Call function
    // May return raw JS pointer! (see below)
    ReturnType retVal =
        InvokeNativeCallback_Call<!!OptArgCount, ReturnType>( baseClass, fn, fnWithOpt, callbackArguments, maxArgCount - args.length() );
    if ( !retVal )
    {
        return false;
    }

    // Return value
    if constexpr( std::is_same_v<ReturnType::value_type, JSObject*> )
    {// A raw JS pointer! Be careful when editing this code!
        args.rval().setObjectOrNull( retVal.value() );
    }
    else if constexpr(std::is_same_v<ReturnType::value_type, JS::Heap<JS::Value>>
                       || std::is_same_v<ReturnType::value_type, JS::Value> )
    {// Might contain unrooted JS::Value! Be careful when editing this code!
        args.rval().set( retVal.value() );
    }
    else if constexpr( std::is_same_v<ReturnType::value_type, nullptr_t> )
    {
        args.rval().setUndefined();
    }
    else
    {
        if ( !convert::to_js::ToValue( cx, retVal.value(), args.rval() ) )
        {
            JS_ReportErrorUTF8( cx, "Internal error: failed to convert return value" );
            return false;
        }
    }

    return true;
}

template <typename BaseClass>
BaseClass* InvokeNativeCallback_GetThisObject( JSContext* cx, JS::HandleValue jsThis )
{
    
    if ( jsThis.isObject() )
    {
        if ( js::IsProxy( &jsThis.toObject() ) )
        {
            JS::RootedObject jsObject(cx, js::GetProxyTargetObject( &jsThis.toObject() ) );
            return static_cast<BaseClass*>(JS_GetInstancePrivate( cx, jsObject, &BaseClass::JsClass, nullptr ));
        }

        JS::RootedObject jsObject(cx, &jsThis.toObject() );
        return static_cast<BaseClass*>(JS_GetInstancePrivate( cx, jsObject, &BaseClass::JsClass, nullptr ));
    }

    if constexpr(std::is_same_v<BaseClass, JsGlobalObject>)
    {
        if ( jsThis.isUndefined() )
        {// global has undefined `this`
            JS::RootedObject jsObject(cx, JS::CurrentGlobalOrNull( cx ) );
            return static_cast<BaseClass*>(JS_GetInstancePrivate( cx, jsObject, &BaseClass::JsClass, nullptr ));
        }
    }

    return nullptr;
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
        (void)fnWithOpt;
        (void)optArgCount;
        return std::apply( fn, std::tuple_cat( std::make_tuple( baseClass ), argTuple ) );
    }
    else
    {// Invoke callback with optional argument handler
        (void)fn;        
        return std::apply( fnWithOpt, std::tuple_cat( std::make_tuple( baseClass, optArgCount ), argTuple ) );
    }
}

}
