#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_engine/js_error_codes.h>

#include <type_traits>

// TODO: replace Mjs status with JS_ReportErrorASCII

#define MJS_STRINGIFY(x) #x

#define MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT(baseClass, functionName, functionWithOptName, optArgCount) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        Mjs_Status mjsRet = \
            InvokeNativeCallback<optArgCount>( cx, &baseClass::functionName, &baseClass::functionWithOptName, argc, vp );\
        if (Mjs_Ok != mjsRet)\
        {\
            std::string errorText = MJS_STRINGIFY(functionName);\
            errorText += " failed: ";\
            errorText += ErrorCodeToString( mjsRet );\
            JS_ReportErrorASCII( cx, errorText.c_str() );\
        }\
        return Mjs_Ok == mjsRet;\
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

// Workarounds for MSVC bug (see below)
template<typename ReturnType>
constexpr inline bool NeedToPrepareNativeValue()
{
    return std::is_pointer<std::tuple_element<1, ReturnType>::type>::value;
}

template <size_t OptArgCount = 0, typename BaseClass, typename ReturnType, typename FuncOptTupe, typename ... ArgTypes>
Mjs_Status InvokeNativeCallback( JSContext* cx, 
                                 ReturnType( BaseClass::*fn )(ArgTypes ...), 
                                 FuncOptTupe fnWithOpt,
                                 unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );   

    if ( args.length() < (sizeof ...(ArgTypes) - OptArgCount)
         || args.length() > sizeof ...(ArgTypes) )
    {        
        return Mjs_InvalidArgumentCount;
    }

    // Parse arguments

    bool bRet = true;
    auto callbackArguments =
        JsToNativeValueTuple<sizeof ...(ArgTypes), ArgTypes...>( 
            args,
            [&]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index )
    {
        using ArgType = typename decltype(argTypeStruct)::type;
        ArgType nativeValue = ArgType();
        if ( index < jsArgs.length() )
        {
            bRet &= JsToNativeValue( cx, jsArgs[index], nativeValue );
        }
        return nativeValue;
    } );
    if (!bRet)
    {
        return Mjs_InvalidArgumentType;
    }
    
    // Call function

    BaseClass* baseClass = static_cast<BaseClass*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !baseClass )
    {
        return Mjs_InternalError;
    }

    ReturnType retVal = 
        InvokeNativeCallback_Call<!!OptArgCount, ReturnType>( baseClass, fn, fnWithOpt, callbackArguments, sizeof ...(ArgTypes) - args.length());
    Mjs_Status mjsRet = std::get<0>( retVal );
    if ( Mjs_Ok != mjsRet )
    {
        return mjsRet;
    }

    mjsRet = InvokeNativeCallback_SetReturnValue( cx, retVal, args.rval() );
    if ( Mjs_Ok != mjsRet )
    {
        return mjsRet;
    }

    return Mjs_Ok;
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

template <typename TupleType>
Mjs_Status InvokeNativeCallback_SetReturnValue( JSContext* cx, const TupleType& returnedTuple, JS::MutableHandleValue jsReturnValue )
{  
    constexpr size_t returnTupleSize = std::tuple_size<TupleType>::value;
    static_assert(returnTupleSize <= 2, "Invalid size of returned tuple");

    jsReturnValue.setUndefined();

    if constexpr(returnTupleSize == 2)
    {
        if constexpr(NeedToPrepareNativeValue<TupleType>())
        {// bug in MSVC: evaluates std::tuple_element even in discarded constexpr branches.
         // can't use unique_ptr because of that as well...
            auto pRetObj( std::get<1>( returnedTuple ) );
            if ( !NativeToJsValue( cx, pRetObj->GetJsObject(), jsReturnValue ) )
            {
                delete pRetObj;
                return Mjs_InternalError;
            }
            delete pRetObj;
        }
        else
        {
            if ( !NativeToJsValue( cx, std::get<1>( retVal ), jsReturnValue ) )
            {
                return Mjs_InternalError;
            }
        }
    }

    return Mjs_Ok;
}

}
