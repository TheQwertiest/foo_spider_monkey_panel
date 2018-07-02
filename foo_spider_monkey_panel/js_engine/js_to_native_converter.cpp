#include <stdafx.h>
#include "js_to_native_converter.h"

#include <js/Conversions.h>

namespace mozjs::convert::to_native
{

template <>
bool ToValue<bool>( [[maybe_unused]]JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    isValid = true;
    return JS::ToBoolean( jsValue );
}

template <>
int8_t ToValue<int8_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    int8_t val;
    isValid = JS::ToInt8( cx, jsValue, &val );
    return val;
}

template <>
int32_t ToValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    int32_t val;
    isValid = JS::ToInt32( cx, jsValue, &val );
    return val;
}

template <>
uint8_t ToValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    uint8_t val;
    isValid = JS::ToUint8( cx, jsValue, &val );
    return val;
}

template <>
uint32_t ToValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    uint32_t val;
    isValid = JS::ToUint32( cx, jsValue, &val );
    return val;
}

template <>
uint64_t ToValue<uint64_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    uint64_t val;
    isValid = JS::ToUint64( cx, jsValue, &val );
    return val;
}

template <>
float ToValue<float>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    double val;
    isValid = JS::ToNumber( cx, jsValue, &val );
    return static_cast<float>(val);
}

template <>
double ToValue<double>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    double val;
    isValid = JS::ToNumber( cx, jsValue, &val );
    return val;
}

template <>
std::string ToValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    JS::RootedString jsString( cx, JS::ToString( cx, jsValue ));
    isValid = !!jsString;
    if ( !isValid )
    {
        return std::forward<std::string>( std::string() );
    }

    std::unique_ptr<
        const char, std::function<void( const char* )>
    > encodedString(
        JS_EncodeStringToUTF8( cx, jsString ), 
        [&]( const char* str )
        {
            JS_free( cx, (void*)str );
        }
    );

    if ( !encodedString )
    {
        JS_ReportOutOfMemory( cx );
        return std::forward<std::string>( std::string() );
    }    

    return std::forward<std::string>( std::string( encodedString.get()) );
}

template <>
std::wstring ToValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    JS::RootedString jsString( cx, JS::ToString( cx, jsValue ) );
    isValid = !!jsString;
    if ( !isValid )
    {
        return std::forward<std::wstring>( std::wstring() );
    }

    size_t strLen = JS_GetStringLength( jsString );
    std::wstring wStr( strLen + 1, '\0' );
    mozilla::Range<char16_t> wCharStr( reinterpret_cast<char16_t*>(wStr.data()), strLen) ;
    if ( !JS_CopyStringChars( cx, wCharStr, jsString ) )
    {
        JS_ReportOutOfMemory( cx );
        return std::forward<std::wstring>( std::wstring() );
    }

    return std::forward<std::wstring>( wStr );
}

template <>
std::nullptr_t ToValue<std::nullptr_t>( [[maybe_unused]]JSContext * cx, [[maybe_unused]]const JS::HandleValue& jsValue, bool& isValid )
{
    isValid = true;
    return nullptr;
}

}
