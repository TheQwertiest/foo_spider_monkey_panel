#pragma once

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>

#include <optional>

class JSFreeOp;
struct JSContext;
class JSObject;

namespace mozjs::utils
{

template <typename T>
std::optional<T> GetOptionalProperty( JSContext* cx, JS::HandleObject jsObject, const std::string& propName )
{
    bool hasProp;
    if ( !JS_HasProperty( cx, jsObject, propName.c_str(), &hasProp ) )
    {
        throw smp::JsException();
    }

    if ( !hasProp )
    {
        return std::nullopt;
    }

    JS::RootedValue jsValue( cx );
    if ( !JS_GetProperty( cx, jsObject, propName.c_str(), &jsValue ) )
    {
        throw smp::JsException();
    }

    return convert::to_native::ToValue<T>( cx, jsValue );
};

template <typename T>
void OptionalPropertyTo( JSContext* cx, JS::HandleObject jsObject, const std::string& propName, T& dst )
{
    if ( auto propOpt = GetOptionalProperty<T>( cx, jsObject, propName ) )
    {
        dst = std::move( *propOpt );
    }
}

template <typename T>
void AddProperty( JSContext* cx, JS::HandleObject jsObject, const std::string& propName, const T& propValue )
{
    if constexpr ( std::is_same_v<T, JS::RootedValue> )
    {
        if ( !JS_DefineProperty( cx, jsObject, propName.c_str(), propValue, kDefaultPropsFlags ) )
        {
            throw smp::JsException();
        }
    }
    else
    {
        JS::RootedValue jsProperty( cx );
        convert::to_js::ToValue( cx, propValue, &jsProperty );

        if ( !JS_DefineProperty( cx, jsObject, propName.c_str(), jsProperty, kDefaultPropsFlags ) )
        {
            throw smp::JsException();
        }
    }
};

template <typename T>
void SetProperty( JSContext* cx, JS::HandleObject jsObject, const std::string& propName, const T& propValue )
{
    JS::RootedValue jsProperty( cx );
    convert::to_js::ToValue( cx, propValue, &jsProperty );

    if ( !JS_SetProperty( cx, jsObject, propName.c_str(), jsProperty ) )
    {
        throw smp::JsException();
    }
};

} // namespace mozjs::utils
