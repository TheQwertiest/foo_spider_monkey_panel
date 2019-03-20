#include <stdafx.h>
#include "js_to_native.h"

#include <js/Conversions.h>

namespace mozjs::convert::to_native::internal
{

template <>
bool ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    (void)cx;
    return JS::ToBoolean( jsValue );
}

template <>
int8_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    int8_t val;
    if ( !JS::ToInt8( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
int32_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    int32_t val;
    if ( !JS::ToInt32( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
uint8_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    uint8_t val;
    if ( !JS::ToUint8( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
uint32_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    uint32_t val;
    if ( !JS::ToUint32( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
int64_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    int64_t val;
    if ( !JS::ToInt64( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
uint64_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    uint64_t val;
    if ( !JS::ToUint64( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
float ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    double val;
    if ( !JS::ToNumber( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return static_cast<float>( val );
}

template <>
double ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    double val;
    if ( !JS::ToNumber( cx, jsValue, &val ) )
    {
        throw smp::JsException();
    }
    return val;
}

template <>
pfc::string8_fast ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    JS::RootedString jsString( cx, JS::ToString( cx, jsValue ) );
    return ToValue<pfc::string8_fast>( cx, jsString );
}

template <>
std::wstring ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    JS::RootedString jsString( cx, JS::ToString( cx, jsValue ) );
    return ToValue<std::wstring>( cx, jsString );
}

template <>
std::nullptr_t ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    (void)cx;
    (void)jsValue;
    return nullptr;
}

} // namespace mozjs::convert::to_native::internal

namespace mozjs::convert::to_native
{

template <>
pfc::string8_fast ToValue( JSContext* cx, const JS::HandleString& jsString )
{
    const auto& wStr = ToValue<std::wstring>( cx, jsString );
    return pfc::string8_fast{ pfc::stringcvt::string_utf8_from_wide( wStr.c_str(), wStr.size() ) };
}

template <>
std::wstring ToValue( JSContext* cx, const JS::HandleString& jsString )
{
    std::wstring wStr( JS_GetStringLength( jsString ), '\0' );
    mozilla::Range<char16_t> wCharStr( reinterpret_cast<char16_t*>( wStr.data() ), wStr.size() );
    if ( !JS_CopyStringChars( cx, wCharStr, jsString ) )
    {
        throw smp::JsException();
    }

    return wStr;
}

} // namespace mozjs::convert::to_native
