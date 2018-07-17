#include <stdafx.h>
#include "js_to_native_converter.h"

#include <js/Conversions.h>

namespace mozjs::convert::to_native
{

std::wstring ToValue( JSContext * cx, const JS::HandleString& jsString, bool& isValid )
{
    isValid = true;

    size_t strLen = JS_GetStringLength( jsString );
    std::wstring wStr( strLen, '\0' );
    mozilla::Range<char16_t> wCharStr( reinterpret_cast<char16_t*>( wStr.data() ), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, jsString ) )
    {
        JS_ReportOutOfMemory( cx );
        return std::wstring();
    }

    return wStr;
}

template <>
bool ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    (void)cx;

    isValid = true;
    return JS::ToBoolean( jsValue );
}

template <>
int8_t ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    int8_t val;
    isValid = JS::ToInt8( cx, jsValue, &val );
    return val;
}

template <>
int32_t ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    int32_t val;
    isValid = JS::ToInt32( cx, jsValue, &val );
    return val;
}

template <>
uint8_t ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    uint8_t val;
    isValid = JS::ToUint8( cx, jsValue, &val );
    return val;
}

template <>
uint32_t ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    uint32_t val;
    isValid = JS::ToUint32( cx, jsValue, &val );
    return val;
}

template <>
uint64_t ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    uint64_t val;
    isValid = JS::ToUint64( cx, jsValue, &val );
    return val;
}

template <>
float ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    double val;
    isValid = JS::ToNumber( cx, jsValue, &val );
    return static_cast<float>(val);
}

template <>
double ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    double val;
    isValid = JS::ToNumber( cx, jsValue, &val );
    return val;
}

template <>
pfc::string8_fast ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    std::wstring wValue (ToValue<std::wstring>( cx, jsValue, isValid ));
    if ( !isValid )
    {
        return pfc::string8_fast();
    }

    return pfc::string8_fast(pfc::stringcvt::string_utf8_from_wide( wValue.c_str(), wValue.length() ));
}

template <>
std::wstring ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    JS::RootedString jsString( cx, JS::ToString( cx, jsValue ) );
    isValid = !!jsString;
    if ( !isValid )
    {
        return std::wstring();
    }

    return ToValue(cx, jsString, isValid);
}

template <>
std::nullptr_t ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    (void)cx;
    ( void )jsValue;

    isValid = true;
    return nullptr;
}

}
