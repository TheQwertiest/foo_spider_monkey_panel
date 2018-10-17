#pragma once

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/string_helpers.h>

#pragma warning( push )
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <js/Wrapper.h>
#pragma warning( pop )

#include <type_traits>
#include <vector>

/// @brief Defines a function named `functionName`, which executes `functionImpl` in a safe way:
///        traps C++ exceptions and converts them to JS exceptions,
///        while adding `functionName` to error report.
#define MJS_DEFINE_JS_FN( functionName, functionImpl )                                    \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )                      \
    {                                                                                     \
        return mozjs::error::Execute_JsSafe( cx, #functionName, functionImpl, argc, vp ); \
    }

/// @brief Defines a function named `functionName`, which converts all JS arguments to corresponding arguments
///        of `functionImpl` prototype and executes it in a safe way (see MJS_DEFINE_JS_FN).
///        Allows for execution with less arguments than in `functionImpl` prototype
///        (`optArgCount` is the maximum amount of optional arguments),
///        in that case `functionImplWithOpt` will be called.
#define MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( functionName, functionImpl, functionImplWithOpt, optArgCount ) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )                                          \
    {                                                                                                         \
        auto wrappedFunc = []( JSContext* cx, unsigned argc, JS::Value* vp ) {                                \
            InvokeNativeCallback<optArgCount>( cx, &functionImpl, &functionImplWithOpt, argc, vp );           \
        };                                                                                                    \
        return mozjs::error::Execute_JsSafe( cx, #functionName, wrappedFunc, argc, vp );                      \
    }

/// @brief Same as MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT, but with zero optional arguments.
#define MJS_DEFINE_JS_FN_FROM_NATIVE( functionName, functionImpl ) \
    MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( functionName, functionImpl, functionImpl, 0 )

namespace mozjs
{

template <size_t OptArgCount = 0, typename BaseClass, typename ReturnType, typename FuncOptType, typename... ArgTypes>
void InvokeNativeCallback( JSContext* cx,
                           ReturnType ( BaseClass::*fn )( ArgTypes... ),
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    InvokeNativeCallback_Impl<
        OptArgCount,
        BaseClass,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}

template <size_t OptArgCount = 0, typename BaseClass, typename ReturnType, typename FuncOptType, typename... ArgTypes>
void InvokeNativeCallback( JSContext* cx,
                           ReturnType ( BaseClass::*fn )( ArgTypes... ) const,
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    InvokeNativeCallback_Impl<
        OptArgCount,
        BaseClass,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}

template <size_t OptArgCount, typename BaseClass, typename ReturnType, typename FuncType, typename FuncOptType, typename... ArgTypes>
void InvokeNativeCallback_Impl( JSContext* cx,
                                FuncType fn,
                                FuncOptType fnWithOpt,
                                unsigned argc, JS::Value* vp )
{
    constexpr size_t maxArgCount = sizeof...( ArgTypes );
    static_assert( OptArgCount <= maxArgCount );

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    if ( args.length() < ( maxArgCount - OptArgCount ) )
    {
        throw smp::SmpException( "Invalid number of arguments" );
    }

    BaseClass* baseClass = InvokeNativeCallback_GetThisObject<BaseClass>( cx, args.thisv() );
    if ( !baseClass )
    {
        throw smp::SmpException( "Invalid `this` context" );
    }

    // Parse arguments

    auto callbackArguments =
        JsToNativeArguments<maxArgCount, ArgTypes...>(
            args,
            [maxArgCount, cx]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index ) {
                using ArgType = typename std::remove_const_t<std::remove_reference_t<decltype( argTypeStruct )::type>>;

                // Not an error: default value might be set in callback
                const bool isDefaultValue = ( index >= jsArgs.length() || index > maxArgCount );

                if constexpr ( std::is_same_v<ArgType, JS::HandleValue> )
                { // Skip conversion, pass through
                    if ( isDefaultValue )
                    {
                        return jsArgs[0]; ///< Dummy value
                    }
                    return jsArgs[index];
                }
                else
                {
                    if ( isDefaultValue )
                    {
                        return ArgType(); ///< Dummy value
                    }

                    return convert::to_native::ToValue<ArgType>( cx, jsArgs[index] );
                }
            } );

    // Call function
    // May return raw JS pointer! (see below)
    auto invokeNative = [&]() {
        return InvokeNativeCallback_Call<!!OptArgCount, ReturnType>( baseClass, fn, fnWithOpt, callbackArguments, ( maxArgCount > args.length() ? maxArgCount - args.length() : 0 ) );
    };

    // Return value
    if constexpr ( std::is_same_v<ReturnType, JSObject*> )
    { // A raw JS pointer! Be careful when editing this code!
        args.rval().setObjectOrNull( invokeNative() );
    }
    else if constexpr ( std::is_same_v<ReturnType, JS::Value> )
    { // Unrooted JS::Value! Be careful when editing this code!
        args.rval().set( invokeNative() );
    }
    else if constexpr ( std::is_same_v<ReturnType, void> )
    {
        invokeNative();
        args.rval().setUndefined();
    }
    else
    {
        convert::to_js::ToValue( cx, invokeNative(), args.rval() );
    }
}

template <typename BaseClass>
BaseClass* InvokeNativeCallback_GetThisObject( JSContext* cx, JS::HandleValue jsThis )
{
    if ( jsThis.isObject() )
    {
        JS::RootedObject jsObject( cx, &jsThis.toObject() );
        return GetInnerInstancePrivate<BaseClass>( cx, jsObject );
    }

    if constexpr ( std::is_same_v<BaseClass, JsGlobalObject> )
    {
        if ( jsThis.isUndefined() )
        { // global has undefined `this`
            JS::RootedObject jsObject( cx, JS::CurrentGlobalOrNull( cx ) );
            return static_cast<BaseClass*>( JS_GetInstancePrivate( cx, jsObject, &BaseClass::JsClass, nullptr ) );
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
    return std::make_tuple( func( jsArgs, TypeWrapper<ArgTypes>{}, Indexes )... );
}

template <
    bool HasOptArg,
    typename ReturnType,
    typename BaseClass,
    typename FuncType,
    typename FuncOptType,
    typename ArgTupleType>
ReturnType InvokeNativeCallback_Call( BaseClass* baseClass,
                                      FuncType fn, FuncOptType fnWithOpt,
                                      const ArgTupleType& argTuple, size_t optArgCount )
{
    if constexpr ( !HasOptArg )
    {
        (void)fnWithOpt;
        (void)optArgCount;
        return std::apply( fn, std::tuple_cat( std::make_tuple( baseClass ), argTuple ) );
    }
    else
    { // Invoke callback with optional argument handler
        (void)fn;
        return std::apply( fnWithOpt, std::tuple_cat( std::make_tuple( baseClass, optArgCount ), argTuple ) );
    }
}

} // namespace mozjs
