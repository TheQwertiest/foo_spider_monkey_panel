#pragma once

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_backend/objects/core/global_object.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/js_object_constants.h>
#include <js_backend/utils/panel_from_global.h>

#include <qwr/string_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <type_traits>
#include <vector>

namespace mozjs::internal
{

template <typename... ArgTypes, typename FuncType, size_t... Indexes>
auto ProcessJsArgs_Impl( const JS::CallArgs& jsArgs, FuncType&& func, std::index_sequence<Indexes...> )
{
    return std::make_tuple( func( jsArgs, std::type_identity<ArgTypes>{}, Indexes )... );
}

template <typename... ArgTypes, typename FuncType>
auto ProcessJsArgs( const JS::CallArgs& jsArgs, FuncType&& func )
{
    return ProcessJsArgs_Impl<ArgTypes...>( jsArgs, func, std::make_index_sequence<sizeof...( ArgTypes )>{} );
}

template <typename BaseClass>
BaseClass* InvokeNativeCallback_GetThisObject( JSContext* cx, JS::HandleValue jsThis )
{
    if ( jsThis.isObject() )
    {
        JS::RootedObject jsObject( cx, &jsThis.toObject() );
        return JsObjectBase<BaseClass>::ExtractNative( cx, jsObject );
    }

    if constexpr ( std::is_same_v<BaseClass, JsGlobalObject> )
    {
        if ( jsThis.isUndefined() )
        { // global has undefined `this`
            JS::RootedObject jsObject( cx, JS::CurrentGlobalOrNull( cx ) );
            return JsGlobalObject::ExtractNative( cx, jsObject );
        }
    }

    return nullptr;
}

template <typename FirstArg, typename... Args>
constexpr bool InvokeNativeCallback_ParseArguments_Check()
{
    // Check that only last argument has JS::HandleValueArray type
    if constexpr ( sizeof...( Args ) <= 1 )
    {
        return true;
    }
    else
    {
        return !std::is_same_v<FirstArg, JS::HandleValueArray> && InvokeNativeCallback_ParseArguments_Check<Args...>();
    }
}

template <typename... ArgTypes>
auto InvokeNativeCallback_ParseArguments( JSContext* cx, JS::MutableHandleValueVector valueVector, const JS::CallArgs& jsArgs )
{
    constexpr size_t argCount = sizeof...( ArgTypes );
    if constexpr ( argCount )
    {
        static_assert( InvokeNativeCallback_ParseArguments_Check<ArgTypes...>() );
    }

    constexpr bool hasValueArray = []() constexpr {
        if constexpr ( !argCount )
        {
            return false;
        }
        else
        {
            return std::is_same_v<std::tuple_element<argCount - 1, std::tuple<ArgTypes...>>::type, JS::HandleValueArray>;
        }
    }();

    if constexpr ( hasValueArray )
    {
        if ( argCount <= jsArgs.length() )
        {
            // Reserve memory, so that we can initialize JS::HandleValueArray correctly
            if ( !valueVector.resize( jsArgs.length() - ( argCount - 1 ) ) )
            {
                throw std::bad_alloc();
            }
        }
    }

    auto callbackArguments =
        ProcessJsArgs<ArgTypes...>(
            jsArgs,
            [cx, &valueVector]( const JS::CallArgs& jsArgs, auto argTypeStruct, size_t index ) {
                using ArgType = typename std::remove_const_t<std::remove_reference_t<decltype( argTypeStruct )::type>>;

                constexpr size_t MaxArgCount = sizeof...( ArgTypes );
                // Not an error: default value might be set in callback
                const bool isDefaultValue = ( index >= jsArgs.length() || index > MaxArgCount );

                try
                {
                    if constexpr ( std::is_same_v<ArgType, JS::HandleValue> )
                    {
                        if ( isDefaultValue )
                        {
                            return ArgType( JS::UndefinedHandleValue );
                        }
                        else
                        {
                            return ArgType( jsArgs[index] );
                        }
                    }
                    else if constexpr ( std::is_same_v<ArgType, JS::HandleValueArray> )
                    {
                        if ( isDefaultValue )
                        {
                            return JS::HandleValueArray::empty();
                        }
                        else
                        {
                            return JS::HandleValueArray::fromMarkedLocation( valueVector.length(), valueVector.begin() );
                        }
                    }
                    else
                    {
                        if ( isDefaultValue )
                        {
                            if constexpr ( !std::is_default_constructible_v<ArgType> )
                            {
                                throw qwr::QwrException( "Internal error: non default-constructible type is used as optional argument" );
                            }
                            else
                            {
                                return ArgType(); ///< Dummy value
                            }
                        }
                        else
                        {
                            return convert::to_native::ToValue<ArgType>( cx, jsArgs[index] );
                        }
                    }
                }
                catch ( const qwr::QwrException& e )
                { // TODO: we lose stacktrace info here, make this configurable
                    throw qwr::QwrException( "Argument {}: {}", index, e.what() );
                }
            } );

    if constexpr ( hasValueArray )
    {
        if ( !valueVector.empty() )
        {
            size_t startIdx = 0;
            for ( auto i: ranges::views::indices( argCount - 1, jsArgs.length() ) )
            {
                valueVector[startIdx++].set( jsArgs[i] );
            }
        }
    }

    return callbackArguments;
}

// TODO: maybe move self arg removal to before InvokeNativeCallback call?

template <bool HasSelfArg, typename... ArgTypes>
auto InvokeNativeCallback_ParseArguments_RemoveSelfArg( JSContext* cx, JS::MutableHandleValueVector valueVector, const JS::CallArgs& jsArgs )
{
    return InvokeNativeCallback_ParseArguments<ArgTypes...>( cx, valueVector, jsArgs );
}

template <bool HasSelfArg, typename T, typename... ArgTypes>
    requires( HasSelfArg )
auto InvokeNativeCallback_ParseArguments_RemoveSelfArg( JSContext* cx, JS::MutableHandleValueVector valueVector, const JS::CallArgs& jsArgs )
{
    return InvokeNativeCallback_ParseArguments<ArgTypes...>( cx, valueVector, jsArgs );
}

template <bool HasOptArg,
          bool HasSelfArg,
          typename ReturnType,
          typename BaseClass,
          typename FuncType,
          typename FuncOptType,
          typename ArgTupleType>
ReturnType InvokeNativeCallback_Call_Member( JS::HandleObject jsSelf,
                                             BaseClass* baseClass,
                                             FuncType fn, FuncOptType fnWithOpt,
                                             const ArgTupleType& argTuple, size_t optArgCount )
{
    const auto getResolvedFn = [&] {
        if constexpr ( HasOptArg )
        {
            return fnWithOpt;
        }
        else
        {
            return fn;
        }
    };
    const auto getNativeCallArgs = [&] {
        if constexpr ( HasOptArg )
        {
            return std::tuple_cat( std::make_tuple( optArgCount ), argTuple );
        }
        else
        {
            return argTuple;
        }
    };
    const auto getFullCallArgs = [&] {
        if constexpr ( HasSelfArg )
        {
            return std::tuple_cat( std::make_tuple( baseClass, jsSelf ), getNativeCallArgs() );
        }
        else
        {
            return std::tuple_cat( std::make_tuple( baseClass ), getNativeCallArgs() );
        }
    };

    return std::apply( getResolvedFn(), getFullCallArgs() );
}

template <bool HasOptArg,
          typename ReturnType,
          typename FuncType,
          typename FuncOptType,
          typename ArgTupleType>
ReturnType InvokeNativeCallback_Call_Static( JSContext* cx,
                                             FuncType fn, FuncOptType fnWithOpt,
                                             const ArgTupleType& argTuple, size_t optArgCount )
{
    const auto getResolvedFn = [&] {
        if constexpr ( HasOptArg )
        {
            return fnWithOpt;
        }
        else
        {
            return fn;
        }
    };
    const auto getNativeCallArgs = [&] {
        if constexpr ( HasOptArg )
        {
            return std::tuple_cat( std::make_tuple( cx, optArgCount ), argTuple );
        }
        else
        {
            return std::tuple_cat( std::make_tuple( cx ), argTuple );
        }
    };

    return std::apply( getResolvedFn(), getNativeCallArgs() );
}

template <size_t OptArgCount,
          bool HasSelfArg,
          typename BaseClass,
          typename ReturnType,
          typename FuncType,
          typename FuncOptType,
          typename... ArgTypes>
void InvokeNativeCallback_Member( JSContext* cx,
                                  FuncType fn,
                                  FuncOptType fnWithOpt,
                                  unsigned argc, JS::Value* vp )
{
    constexpr size_t maxArgCount = sizeof...( ArgTypes ) - !!HasSelfArg;
    static_assert( OptArgCount <= maxArgCount );

    JS::CallArgs jsArgs = JS::CallArgsFromVp( argc, vp );
    if ( jsArgs.length() < ( maxArgCount - OptArgCount ) )
    {
        throw qwr::QwrException( "Invalid number of arguments" );
    }

    // TODO: cleanup `this` handling
    JS::RootedValue jsThisValue( cx, jsArgs.thisv() );
    JS::RootedObject jsSelf( cx );
    if ( jsThisValue.isObjectOrNull() )
    {
        jsSelf = jsThisValue.toObjectOrNull();
    }
    BaseClass* baseClass = InvokeNativeCallback_GetThisObject<BaseClass>( cx, jsThisValue );
    if ( !baseClass )
    {
        throw qwr::QwrException( "Invalid `this` context" );
    }

    JS::RootedValueVector handleValueVector( cx );
    auto callbackArguments = InvokeNativeCallback_ParseArguments_RemoveSelfArg<HasSelfArg, ArgTypes...>(
        cx, &handleValueVector, jsArgs );

    // May return raw JS pointer! (see below)
    const auto invokeNative = [&]() {
        return InvokeNativeCallback_Call_Member<!!OptArgCount, HasSelfArg, ReturnType>(
            jsSelf,
            baseClass,
            fn,
            fnWithOpt,
            callbackArguments,
            ( maxArgCount > jsArgs.length() ? maxArgCount - jsArgs.length() : 0 ) );
    };

    // Return value
    if constexpr ( std::is_same_v<ReturnType, JSObject*> )
    { // A raw JS pointer! Be careful when editing this code!
        jsArgs.rval().setObjectOrNull( invokeNative() );
    }
    else if constexpr ( std::is_same_v<ReturnType, JS::Value> )
    { // Unrooted JS::Value! Be careful when editing this code!
        jsArgs.rval().set( invokeNative() );
    }
    else if constexpr ( std::is_same_v<ReturnType, void> )
    {
        invokeNative();
        jsArgs.rval().setUndefined();
    }
    else
    {
        convert::to_js::ToValue( cx, invokeNative(), jsArgs.rval() );
    }
}

template <size_t OptArgCount,
          typename ReturnType,
          typename FuncType,
          typename FuncOptType,
          typename... ArgTypes>
void InvokeNativeCallback_Static( JSContext* cx,
                                  FuncType fn,
                                  FuncOptType fnWithOpt,
                                  unsigned argc, JS::Value* vp )
{
    constexpr size_t maxArgCount = sizeof...( ArgTypes );
    static_assert( OptArgCount <= maxArgCount );

    JS::CallArgs jsArgs = JS::CallArgsFromVp( argc, vp );
    if ( jsArgs.length() < ( maxArgCount - OptArgCount ) )
    {
        throw qwr::QwrException( "Invalid number of arguments" );
    }

    JS::RootedValueVector handleValueVector( cx );
    auto callbackArguments = InvokeNativeCallback_ParseArguments<ArgTypes...>( cx, &handleValueVector, jsArgs );

    // May return raw JS pointer! (see below)
    const auto invokeNative = [&]() {
        return InvokeNativeCallback_Call_Static<!!OptArgCount, ReturnType>(
            cx,
            fn,
            fnWithOpt,
            callbackArguments,
            ( maxArgCount > jsArgs.length() ? maxArgCount - jsArgs.length() : 0 ) );
    };

    // Return value
    if constexpr ( std::is_same_v<ReturnType, JSObject*> )
    { // A raw JS pointer! Be careful when editing this code!
        jsArgs.rval().setObjectOrNull( invokeNative() );
    }
    else if constexpr ( std::is_same_v<ReturnType, JS::Value> )
    { // Unrooted JS::Value! Be careful when editing this code!
        jsArgs.rval().set( invokeNative() );
    }
    else if constexpr ( std::is_same_v<ReturnType, void> )
    {
        invokeNative();
        jsArgs.rval().setUndefined();
    }
    else
    {
        convert::to_js::ToValue( cx, invokeNative(), jsArgs.rval() );
    }
}

} // namespace mozjs::internal

namespace mozjs
{

template <size_t OptArgCount = 0, bool HasSelfArg = false,
          typename BaseClass, typename ReturnType, typename FuncOptType, typename... ArgTypes>
void InvokeNativeCallback( JSContext* cx,
                           ReturnType ( BaseClass::*fn )( ArgTypes... ),
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    if ( !HasHostPanelForCurrentGlobal( cx ) )
    {
        return;
    }

    mozjs::internal::InvokeNativeCallback_Member<
        OptArgCount,
        HasSelfArg,
        BaseClass,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}

template <size_t OptArgCount = 0, bool HasSelfArg = false,
          typename BaseClass, typename ReturnType, typename FuncOptType, typename... ArgTypes>
void InvokeNativeCallback( JSContext* cx,
                           ReturnType ( BaseClass::*fn )( ArgTypes... ) const,
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    if ( !HasHostPanelForCurrentGlobal( cx ) )
    {
        return;
    }

    mozjs::internal::InvokeNativeCallback_Member<
        OptArgCount,
        HasSelfArg,
        BaseClass,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}

template <size_t OptArgCount = 0, bool HasSelfArg = false,
          typename ReturnType, typename FuncOptType, typename... ArgTypes>
void InvokeNativeCallback( JSContext* cx,
                           ReturnType( __cdecl* fn )( JSContext*, ArgTypes... ),
                           FuncOptType fnWithOpt,
                           unsigned argc, JS::Value* vp )
{
    if ( !HasHostPanelForCurrentGlobal( cx ) )
    {
        return;
    }

    mozjs::internal::InvokeNativeCallback_Static<
        OptArgCount,
        ReturnType,
        decltype( fn ),
        FuncOptType,
        ArgTypes...>( cx, fn, fnWithOpt, argc, vp );
}

} // namespace mozjs

/// @brief Defines a function named `functionName`, which executes `functionImpl` in a safe way:
///        traps C++ exceptions and converts them to JS exceptions,
///        while adding `logName` to error report.
#define MJS_DEFINE_JS_FN_FULL( functionName, logName, functionImpl )                \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )                \
    {                                                                               \
        return mozjs::error::Execute_JsSafe( cx, logName, functionImpl, argc, vp ); \
    }

/// @brief Same as MJS_DEFINE_JS_FN_FULL, but uses `functionName` for logging.
#define MJS_DEFINE_JS_FN( functionName, functionImpl ) \
    MJS_DEFINE_JS_FN_FULL( functionName, #functionName, functionImpl )

// TODO: document HasSelfArg

/// @brief Defines a function named `functionName`, which converts all JS arguments to corresponding arguments
///        of `functionImpl` prototype and executes it in a safe way (see MJS_DEFINE_JS_FN_FULL).
///        Allows for execution with less arguments than in `functionImpl` prototype
///        (`optArgCount` is the maximum amount of optional arguments),
///        in that case `functionImplWithOpt` will be called.
#define MJS_DEFINE_JS_FN_FROM_NATIVE_FULL( functionName, logName, functionImpl, functionImplWithOpt, optArgCount, hasSelfArg ) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )                                                           \
    {                                                                                                                          \
        const auto wrappedFunc = []( JSContext* cx, unsigned argc, JS::Value* vp ) {                                           \
            InvokeNativeCallback<optArgCount, hasSelfArg>( cx, &functionImpl, &functionImplWithOpt, argc, vp );                \
        };                                                                                                                     \
        return mozjs::error::Execute_JsSafe( cx, logName, wrappedFunc, argc, vp );                                             \
    }

#define MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( functionName, functionImpl, functionImplWithOpt, optArgCount ) \
    MJS_DEFINE_JS_FN_FROM_NATIVE_FULL( functionName, #functionName, functionImpl, functionImplWithOpt, optArgCount, false )

#define MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT_AND_SELF( functionName, functionImpl, functionImplWithOpt, optArgCount ) \
    MJS_DEFINE_JS_FN_FROM_NATIVE_FULL( functionName, #functionName, functionImpl, functionImplWithOpt, optArgCount, true )

#define MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_LOG( functionName, logName, functionImpl ) \
    MJS_DEFINE_JS_FN_FROM_NATIVE_FULL( functionName, logName, functionImpl, functionImpl, 0, false )

// TODO: document
#define MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( functionName, functionImpl ) \
    MJS_DEFINE_JS_FN_FROM_NATIVE_FULL( functionName, #functionName, functionImpl, functionImpl, 0, true )

#define MJS_DEFINE_JS_FN_FROM_NATIVE( functionName, functionImpl ) \
    MJS_DEFINE_JS_FN_FROM_NATIVE_FULL( functionName, #functionName, functionImpl, functionImpl, 0, false )
