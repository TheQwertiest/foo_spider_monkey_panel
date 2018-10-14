#include <stdafx.h>
#include "js_to_native.h"

#include <js/Conversions.h>

namespace mozjs::convert::to_native
{

std::optional<std::wstring> ToValue( JSContext * cx, const JS::HandleString& jsString )
{
    size_t strLen = JS_GetStringLength( jsString );
    std::wstring wStr( strLen, '\0' );
    mozilla::Range<char16_t> wCharStr( reinterpret_cast<char16_t*>(wStr.data()), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, jsString ) )
    {// reports
        return std::nullopt;
    }

    return wStr;
}

template <>
std::optional<bool> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    (void)cx;
    return JS::ToBoolean( jsValue );
}

template <>
std::optional<int8_t> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    int8_t val;
    if ( !JS::ToInt8( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return val;
}

template <>
std::optional<int32_t> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    int32_t val;
    if ( !JS::ToInt32( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return val;
}

template <>
std::optional<uint8_t> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    uint8_t val;
    if ( !JS::ToUint8( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return val;
}

template <>
std::optional<uint32_t> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    uint32_t val;
    if ( !JS::ToUint32( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return val;
}

template <>
std::optional<uint64_t> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    uint64_t val;
    if ( !JS::ToUint64( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return val;
}

template <>
std::optional<float> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    double val;
    if ( !JS::ToNumber( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return static_cast<float>(val);
}

template <>
std::optional<double> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    double val;
    if ( !JS::ToNumber( cx, jsValue, &val ) )
    {// reports
        return std::nullopt;
    }
    return val;
}

template <>
std::optional<pfc::string8_fast> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    auto retValue = ToValue<std::wstring>( cx, jsValue );
    if ( !retValue )
    {// reports
        return std::nullopt;
    }

    return pfc::string8_fast( pfc::stringcvt::string_utf8_from_wide( retValue->c_str(), retValue->length() ) );
}

template <>
std::optional<std::wstring> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    JS::RootedString jsString( cx, JS::ToString( cx, jsValue ) );
    if ( !jsString )
    {// reports
        return std::nullopt;
    }

    return ToValue( cx, jsString );
}

template <>
std::optional<std::nullptr_t> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    (void)cx;
    (void)jsValue;
    return nullptr;
}

}
