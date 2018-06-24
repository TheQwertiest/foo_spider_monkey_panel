#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_engine/js_error_codes.h>

#include <type_traits>

#define MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT(baseClass, functionName, functionWithOptName, optArgCount) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        bool bRet = \
            InvokeNativeCallback<optArgCount>( cx, &baseClass::functionName, &baseClass::functionWithOptName, argc, vp );\
        if (!bRet)\
        {\
            std::string innerErrorText(GetCurrentExceptionText(cx));\
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

namespace mozjs
{

template <typename T>
struct TypeWrapper { using type = T; };

template <typename... ArgTypes, typename FuncType, size_t... Indexes>
auto JsToNativeValueTupleImpl( const JS::CallArgs& jsArgs, FuncType&& func, std::index_sequence<Indexes...> )
{
    return std::make_tuple(func( jsArgs, TypeWrapper<ArgTypes>{}, Indexes ) ...);
}

template <size_t TupleSize, typename... ArgTypes, typename FuncType>
auto JsToNativeValueTuple( const JS::CallArgs& jsArgs, FuncType&& func )
{    
    return JsToNativeValueTupleImpl<ArgTypes...>( jsArgs, func, std::make_index_sequence<TupleSize>{} );
}

template <size_t OptArgCount = 0, typename BaseClass, typename ReturnType, typename FuncOptTupe, typename ... ArgTypes>
bool InvokeNativeCallback( JSContext* cx, 
                                 ReturnType( BaseClass::*fn )(ArgTypes ...), 
                                 FuncOptTupe fnWithOpt,
                                 unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );   

    if ( args.length() < (sizeof ...(ArgTypes) - OptArgCount)
         || args.length() > sizeof ...(ArgTypes) )
    {        
        JS_ReportErrorASCII( cx, "Invalid number of arguments" );
        return false;
    }

    // Parse arguments

    bool bRet = true;
    size_t failedIdx = 0;
    auto callbackArguments =
        JsToNativeValueTuple<sizeof ...(ArgTypes), ArgTypes...>( 
            args,
            [&]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index )
    {
        using ArgType = typename decltype(argTypeStruct)::type;
        ArgType nativeValue = ArgType();
        if ( index < jsArgs.length() )
        {
            if ( !JsToNativeValue( cx, jsArgs[index], nativeValue ) )
            {
                failedIdx = index;
                bRet = false;
            }
        }
        return nativeValue;
    } );
    if (!bRet)
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
        InvokeNativeCallback_Call<!!OptArgCount, ReturnType>( baseClass, fn, fnWithOpt, callbackArguments, sizeof ...(ArgTypes) - args.length());    
    if ( !retVal )
    {        
        return false;
    }

    bRet = InvokeNativeCallback_SetReturnValue( cx, retVal.value(), args.rval() );
    if ( !bRet )
    {
        return false;
    }

    return true;
}

template <
    bool HasOptArg, 
    typename ReturnType,
    typename BaseClass, 
    typename FuncType, 
    typename FuncOptType, 
    typename ArgTupleType 
>
ReturnType InvokeNativeCallback_Call( BaseClass* baseClass, FuncType fn, FuncOptType fnWithOpt, const ArgTupleType& argTuple, size_t optArgCount )
{
    if constexpr(!HasOptArg)
    {
        return std::apply( fn, std::tuple_cat( std::make_tuple( baseClass ), argTuple ) );
    }
    else
    {// Invoke callback with optional argument handler
        return std::apply( fnWithOpt, std::tuple_cat( std::make_tuple( baseClass, optArgCount ), argTuple ) );
    }
}

template <typename ReturnType>
bool InvokeNativeCallback_SetReturnValue( JSContext* cx, const ReturnType& returnedValue, JS::MutableHandleValue jsReturnValue )
{  
    jsReturnValue.setUndefined();

    if constexpr(!std::is_same<ReturnType, nullptr_t>::value)
    {
        if constexpr(std::is_pointer<ReturnType>::value)
        {
            auto pRetObj( returnedValue );
            if ( !NativeToJsValue( cx, pRetObj->GetJsObject(), jsReturnValue ) )
            {
                delete pRetObj;
                JS_ReportErrorASCII( cx, "Internal error: failed to convert return value" );
                return false;
            }
            delete pRetObj;
        }
        else
        {
            if ( !NativeToJsValue( cx, returnedValue, jsReturnValue ) )
            {
                JS_ReportErrorASCII( cx, "Internal error: failed to convert return value" );
                return false;
            }
        }
    }

    return true;
}

}
