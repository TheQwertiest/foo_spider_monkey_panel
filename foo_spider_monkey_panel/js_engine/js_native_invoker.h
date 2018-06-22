#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


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


template <typename BaseClass, typename FuncType, typename ... ArgTypes>
bool InvokeNativeCallback( JSContext* cx, FuncType( BaseClass::*fnCallback )(ArgTypes ...), unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );        
    if ( args.length() != sizeof ...(ArgTypes) )
    {
        //JS_ReportErrorNumberASCII( cx, my_GetErrorMessage, nullptr,
        //args.length() < 1 ? JSSMSG_NOT_ENOUGH_ARGS : JSSMSG_TOO_MANY_ARGS,
        //"evaluate" );
        return false;
    }

    bool bRet = true;
    auto callbackArguments =
        JsToNativeValueTuple<sizeof ...(ArgTypes), ArgTypes...>( args,
                                                     [&]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index )
    {
        using ArgType = typename decltype(argTypeStruct)::type;
        ArgType nativeValue;
        bRet &= JsToNativeValue( jsArgs[index], nativeValue );
        return nativeValue;
    } );
    if (!bRet)
    {
        return false;
    }
    
    BaseClass* baseClass = static_cast<BaseClass*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !baseClass )
    {
        return false;
    }

    args.rval().setUndefined();

    return std::apply( fnCallback, std::tuple_cat( std::make_tuple( baseClass ), callbackArguments ) );
}

}
