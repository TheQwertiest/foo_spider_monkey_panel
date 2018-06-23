#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_engine/js_error_codes.h>

#include <type_traits>

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
constexpr inline bool NeedsToPrepare()
{
    return std::is_pointer<std::tuple_element<1, ReturnType>::type>::value;
}

template <typename BaseClass, typename ReturnType, typename ... ArgTypes>
Mjs_Status InvokeNativeCallback( JSContext* cx, ReturnType( BaseClass::*fnCallback )(ArgTypes ...), unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );        
    if ( args.length() != sizeof ...(ArgTypes) )
    {        
        return Mjs_InvalidArgumentCount;
    }

    bool bRet = true;
    auto callbackArguments =
        JsToNativeValueTuple<sizeof ...(ArgTypes), ArgTypes...>( args,
                                                     [&]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index )
    {
        using ArgType = typename decltype(argTypeStruct)::type;
        ArgType nativeValue;
        bRet &= JsToNativeValue( cx, jsArgs[index], nativeValue );
        return nativeValue;
    } );
    if (!bRet)
    {
        return Mjs_InvalidArgumentType;
    }
    
    BaseClass* baseClass = static_cast<BaseClass*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !baseClass )
    {
        return Mjs_InternalError;
    }

    ReturnType retVal = std::apply( fnCallback, std::tuple_cat( std::make_tuple( baseClass ), callbackArguments ) );
    Mjs_Status mjsRet = std::get<0>( retVal );
    if ( Mjs_Ok != mjsRet )
    {
        return mjsRet;
    }

    args.rval().setUndefined();

    constexpr size_t returnTupleSize = std::tuple_size<ReturnType>::value;
    static_assert(returnTupleSize <= 2, "Invalid size of returned tuple");


    if constexpr(returnTupleSize == 2 )
    {
        if constexpr(NeedsToPrepare<ReturnType>())
        {// bug in MSVC: evaluates std::tuple_element even in discarded constexpr branches.
         // can't use unique_ptr because of that as well...
            auto pRetObj( std::get<1>( retVal ) );
            if ( !NativeToJsValue( cx, pRetObj->GetJsObject(), args.rval() ) )
            {
                delete pRetObj;
                return Mjs_InternalError;
            }
            delete pRetObj;
        }
        else
        {
            if ( !NativeToJsValue( cx, std::get<1>( retVal ), args.rval() ) )
            {
                return Mjs_InternalError;
            }
        }    
    }

    return Mjs_Ok;
}

}
