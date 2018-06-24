#include <stdafx.h>

#include "js_value_converter.h"
#include <js_objects/gdi_font.h>

namespace
{

template <typename NativeObjectType>
bool JsToNativeObject( JSContext * cx, const JS::HandleValue& jsValue, NativeObjectType*& unwrappedValue )
{
    if ( !jsValue.isObjectOrNull() )
    {
        return false;
    }

    JS::RootedObject jsObject( cx, jsValue.toObjectOrNull() );
    if ( !jsObject )
    {
        unwrappedValue = NULL;
        return true;
    }

    if ( !JS_InstanceOf( cx, jsObject, &NativeObjectType::GetClass(), nullptr ) )
    {
        return false;
    }

    auto pJsFont = static_cast<NativeObjectType *>(JS_GetPrivate( jsObject ));
    if ( !pJsFont )
    {
        assert( 0 );
        return false;
    }

    unwrappedValue = pJsFont;
    return true;
}

}

namespace mozjs
{

bool NativeToJsValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( inValue );
    return true;
}

template <>
bool NativeToJsValue<bool>( JSContext *, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
    return true;
}

template <>
bool NativeToJsValue<int32_t>( JSContext *, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
    return true;
}

template <>
bool NativeToJsValue<uint32_t>( JSContext *, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool NativeToJsValue<double>( JSContext *, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool NativeToJsValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JSString* jsString = JS_NewStringCopyZ( cx, inValue.data() );
    if ( !jsString )
    {
        return false;
    }

    wrappedValue.setString( jsString );
    return true;
}

template <>
bool NativeToJsValue<std::nullptr_t>( JSContext *, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
    return true;
}

template <>
bool JsToNativeValue<bool>( JSContext * cx, const JS::HandleValue& jsValue, bool& unwrappedValue )
{
    if ( !jsValue.isBoolean() )
    {
        return false;
    }

    unwrappedValue = jsValue.toBoolean();
    return true;
}

template <>
bool JsToNativeValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue, int32_t& unwrappedValue )
{
    if ( !jsValue.isInt32() )
    {
        return false;
    }

    unwrappedValue = jsValue.toInt32();
    return true;
}

template <>
bool JsToNativeValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue, uint32_t& unwrappedValue )
{
    if ( !jsValue.isNumber() )
    {
        return false;
    }

    unwrappedValue = static_cast<uint32_t>(jsValue.toNumber());
    return true;
}

template <>
bool JsToNativeValue<float>( JSContext * cx, const JS::HandleValue& jsValue, float& unwrappedValue )
{
    if ( !jsValue.isNumber() )
    {
        return false;
    }

    unwrappedValue = static_cast<float>(jsValue.toNumber());
    return true;
}

template <>
bool JsToNativeValue<double>( JSContext * cx, const JS::HandleValue& jsValue, double& unwrappedValue )
{
    if ( !jsValue.isNumber() )
    {
        return false;
    }

    unwrappedValue = jsValue.toNumber();
    return true;
}

template <>
bool JsToNativeValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue, std::string& unwrappedValue )
{
    if ( !jsValue.isString() )
    {
        return false;
    }

    JS::RootedString jsString( cx, jsValue.toString() );
    const char* encodedString = JS_EncodeStringToUTF8( cx, jsString );

    unwrappedValue = encodedString;

    JS_free( cx, (void*)encodedString );
    return true;
}

template <>
bool JsToNativeValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue, std::wstring& unwrappedValue )
{
    std::string tmpString;
    if ( !JsToNativeValue( cx, jsValue, tmpString ) )
    {
        return false;
    }
    // <codecvt> is deprecated in C++17...
    unwrappedValue = pfc::stringcvt::string_wide_from_utf8( tmpString.c_str() );
    return true;
}

template <>
bool JsToNativeValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue, std::nullptr_t& unwrappedValue )
{
    return true;
}

template <>
bool JsToNativeValue<JsGdiFont*>( JSContext * cx, const JS::HandleValue& jsValue, JsGdiFont*& unwrappedValue )
{
    return JsToNativeObject( cx, jsValue, unwrappedValue );
}

}
