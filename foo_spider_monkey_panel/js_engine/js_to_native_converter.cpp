#include <stdafx.h>
#include "js_to_native_converter.h"

namespace mozjs::convert::to_native
{

template <>
bool IsValue<bool>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isBoolean();
}
template <>
bool ToValue<bool>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<bool>( jsValue.toBoolean() );
}

template <>
bool IsValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isInt32();
}
template <>
int32_t ToValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<int32_t>( jsValue.isInt32() );
}

template <>
bool IsValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isNumber();
}
template <>
uint8_t ToValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<uint8_t>( static_cast<uint8_t>(jsValue.toNumber()) );
}

template <>
bool IsValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isNumber();
}
template <>
uint32_t ToValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<uint32_t>( static_cast<uint32_t>( jsValue.toNumber() ) );
}

template <>
bool IsValue<float>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isNumber();
}
template <>
float ToValue<float>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<float>( static_cast<float>( jsValue.toNumber() ) );
}

template <>
bool IsValue<double>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isDouble();
}
template <>
double ToValue<double>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<double>( jsValue.toDouble() );
}

template <>
bool IsValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isString();
}
template <>
std::string ToValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue )
{
    // TODO: add error checking somewhere....
    JS::RootedString jsString( cx, jsValue.toString() );    
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
bool IsValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isString();
}
template <>
std::wstring ToValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue )
{
    JS::RootedString rStr(cx, jsValue.toString() );
    size_t strLen = JS_GetStringLength( rStr );
    std::wstring wStr( strLen + 1, '\0' );
    mozilla::Range<char16_t> wCharStr( reinterpret_cast<char16_t*>(wStr.data()), strLen) ;
    if ( !JS_CopyStringChars( cx, wCharStr, rStr ) )
    {
        JS_ReportOutOfMemory( cx );
        return std::forward<std::wstring>( std::wstring() );
    }

    return std::forward<std::wstring>( wStr );
}

template <>
bool IsValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return true;
}
template <>
std::nullptr_t ToValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue )
{
    return nullptr;
}

}
