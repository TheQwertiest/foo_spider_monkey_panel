#pragma once

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>

#include <optional>

namespace mozjs::internal
{

template <size_t ArgArraySize>
void NativeToJsArguments( [[maybe_unused]] JSContext* cx,
                          [[maybe_unused]] JS::AutoValueArray<ArgArraySize>& wrappedArgs,
                          [[maybe_unused]] uint8_t argIndex )
{
}

template <size_t ArgArraySize, typename ArgType, typename... ArgTypes>
void NativeToJsArguments( JSContext* cx,
                          JS::AutoValueArray<ArgArraySize>& wrappedArgs,
                          uint8_t argIndex, ArgType&& arg, ArgTypes&&... args )
{
    convert::to_js::ToValue( cx, std::forward<ArgType>( arg ), wrappedArgs[argIndex] );
    NativeToJsArguments( cx, wrappedArgs, argIndex + 1, std::forward<ArgTypes>( args )... );
}

bool InvokeJsCallback_Impl( JSContext* cx,
                            JS::HandleObject globalObject,
                            JS::HandleValue functionValue,
                            const JS::HandleValueArray& args,
                            JS::MutableHandleValue rval );

} // namespace mozjs::internal

namespace mozjs
{

template <typename ReturnType = std::nullptr_t, typename... ArgTypes>
std::optional<ReturnType> InvokeJsCallback( JSContext* cx,
                                            JS::HandleObject globalObject,
                                            std::string functionName,
                                            ArgTypes&&... args )
{
    assert( cx );
    assert( !!globalObject );
    assert( functionName.length() );

    JsAutoRealmWithErrorReport autoScope( cx, globalObject );

    JS::RootedValue funcValue( cx );
    if ( !JS_GetProperty( cx, globalObject, functionName.c_str(), &funcValue ) )
    { // Reports
        return std::nullopt;
    }

    if ( funcValue.isUndefined() )
    { // Not an error: user didn't define a callback
        return std::nullopt;
    }

    try
    {
        JS::RootedValue retVal( cx );
        if constexpr ( sizeof...( ArgTypes ) > 0 )
        {
            JS::AutoValueArray<sizeof...( ArgTypes )> wrappedArgs( cx );
            mozjs::internal::NativeToJsArguments( cx, wrappedArgs, 0, std::forward<ArgTypes>( args )... );

            if ( !mozjs::internal::InvokeJsCallback_Impl( cx, globalObject, funcValue, wrappedArgs, &retVal ) )
            {
                return std::nullopt;
            }
        }
        else
        {
            if ( !mozjs::internal::InvokeJsCallback_Impl( cx, globalObject, funcValue, JS::HandleValueArray::empty(), &retVal ) )
            {
                return std::nullopt;
            }
        }

        return convert::to_native::ToValue<ReturnType>( cx, retVal );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return std::nullopt;
    }
}

} // namespace mozjs
