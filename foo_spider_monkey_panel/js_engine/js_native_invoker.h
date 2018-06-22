#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <vector>
#include <variant>

namespace mozjs
{


template <typename ArgType, typename... Args>
auto UnwrapArguments( const JS::CallArgs& wrappedArgs, uint8_t argIndex, ArgType arg, Args&&... dummyArgs )
{
    ArgType unwrappedArg;
    if ( !UnwrapValue( arg, unwrappedArg ) )
    {
        throw std::runtime_error;
    }

    return std::tuple_cat( std::make_tuple( unwrappedArg ), UnwrapArguments(wrappedArgs, argIndex + 1, dummyArgs...) )
}

template <typename ArgType>
auto UnwrapArguments( const JS::CallArgs& wrappedArgs, uint8_t argIndex, ArgType arg )
{
    ArgType unwrappedArg;
    if ( !UnwrapValue( arg, unwrappedArg ) )
    {
        throw std::runtime_error;
    }

    return std::make_tuple( unwrappedArg );
}




template <typename F, size_t... Is>
auto gen_tuple_impl( F func, std::index_sequence<Is...> )
{
    return std::make_tuple( func( Is )... );
}

template <size_t N, typename F>
auto gen_tuple( F func )
{
    return gen_tuple_impl( func, std::make_index_sequence<N>{} );
}

template <typename BaseClass, typename FunctionType, typename ... ArgTypes> 
constexpr std::integral_constant<unsigned, sizeof ...( ArgTypes )>
getArgumentCount3( FunctionType( BaseClass::*f )( ArgTypes ... ) )
{
    return std::integral_constant<unsigned, sizeof ...( ArgTypes )>{};
}

template <typename BaseClass, typename FuncType, typename ... ArgTypes>
bool InvokeNativeCallback( JSContext* cx, FuncType fnCallback, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );


    constexpr size_t argCount = decltype( getArgumentCount3<BaseClass>( fnCallback ) )::value;
    /*
    if ( args.length() != argCount )
    {
        //JS_ReportErrorNumberASCII( cx, my_GetErrorMessage, nullptr,
        //args.length() < 1 ? JSSMSG_NOT_ENOUGH_ARGS : JSSMSG_TOO_MANY_ARGS,
        //"evaluate" );
        return false;
    }*/
    /*
    std::vector<std::variant<ArgTypes...>> argVector;

    float x, y, w, h, line_width;
    uint32_t colour;

    if ( !UnwrapValue( args[0], x )
         || !UnwrapValue( args[1], y )
         || !UnwrapValue( args[2], w )
         || !UnwrapValue( args[3], h )
         || !UnwrapValue( args[4], line_width )
         || !UnwrapValue( args[5], colour ) )
    {
        return false;
    }

    argVector.push_back( x );
    argVector.push_back( y );
    argVector.push_back( w );
    argVector.push_back( h );
    argVector.push_back( line_width );
    argVector.push_back( colour );
    */
    auto callbackArguments = UnwrapArguments<ArgTypes...>(args);
    
    BaseClass* baseClass = static_cast<BaseClass*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !baseClass )
    {
        return false;
    }

    return std::apply( fnCallback, std::tuple_cat( std::make_tuple( baseClass ), callbackArguments ) );
}

}
