#include <stdafx.h>
#include "js_to_native_converter.h"


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
        unwrappedValue = nullptr;
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

bool JsToNative<bool>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isBoolean();
}
bool JsToNative<bool>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<bool>( jsValue.toBoolean() );
}

bool JsToNative<int32_t>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isInt32();
}
int32_t JsToNative<int32_t>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<int32_t>( jsValue.isInt32() );
}

bool JsToNative<uint8_t>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isNumber();
}
uint8_t JsToNative<uint8_t>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<uint8_t>( static_cast<uint8_t>( jsValue.toNumber() ) );
}

bool JsToNative<uint32_t>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isNumber();
}
uint32_t JsToNative<uint32_t>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<uint32_t>( static_cast<uint32_t>( jsValue.toNumber() ) );
}

bool JsToNative<float>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isNumber();
}
float JsToNative<float>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<float>( static_cast<float>( jsValue.toNumber() ) );
}

bool JsToNative<double>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isDouble();
}
double JsToNative<double>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return std::forward<double>( jsValue.toDouble() );
}

bool JsToNative<std::string>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isString();
}
std::string JsToNative<std::string>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
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

bool JsToNative<std::wstring>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return jsValue.isString();
}
std::wstring JsToNative<std::wstring>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    mozilla::Range<char16_t> wCharStr;
    if ( !JS_CopyStringChars( cx, wCharStr, jsValue.toString() ) )
    {
        JS_ReportOutOfMemory( cx );
        return std::forward<std::wstring>( std::wstring() );
    }

    return std::forward<std::wstring>( std::wstring((wchar_t*)wCharStr.begin().get(), wCharStr.length()) );
}

bool JsToNative<std::nullptr_t>::IsValid( JSContext * cx, const JS::HandleValue& jsValue )
{
    return true;
}
std::nullptr_t JsToNative<std::nullptr_t>::Convert( JSContext * cx, const JS::HandleValue& jsValue )
{
    return nullptr;
}

}
