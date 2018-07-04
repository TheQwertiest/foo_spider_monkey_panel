#include <stdafx.h>
#include "native_to_js_converter.h"

#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/gdi_bitmap.h>

namespace mozjs::convert::to_js
{

bool ToValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    (void)cx;
    (void)inValue;
    wrappedValue.setObjectOrNull( inValue );
    return true;
}

template <>
bool ToValue<bool>( JSContext *, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
    return true;
}

template <>
bool ToValue<int8_t>( JSContext *, const int8_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( static_cast<int32_t>(inValue) );
    return true;
}

template <>
bool ToValue<int32_t>( JSContext *, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
    return true;
}

template <>
bool ToValue<uint32_t>( JSContext *, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue<uint64_t>( JSContext *, const uint64_t& inValue, JS::MutableHandleValue wrappedValue )
{    
    wrappedValue.setDouble( static_cast<double>(inValue) );
    return true;
}

template <>
bool ToValue<double>( JSContext *, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue<float>( JSContext *, const float& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedString jsString (cx, JS_NewStringCopyN( cx, inValue.data(), inValue.length() ));
    if ( !jsString )
    {
        return false;
    }

    wrappedValue.setString( jsString );
    return true;
}

template <>
bool ToValue<std::wstring_view>( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedString jsString( cx, JS_NewUCStringCopyN( cx, reinterpret_cast<const char16_t*>(inValue.data()), inValue.length() ) );
    if ( !jsString )
    {
        return false;
    }

    wrappedValue.setString( jsString );
    return true;
}

template <>
bool ToValue<std::string>( JSContext * cx, const std::string& inValue, JS::MutableHandleValue wrappedValue )
{
    return ToValue<std::string_view>( cx, inValue, wrappedValue );
}

template <>
bool ToValue<std::wstring>( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue )
{
    return ToValue<std::wstring_view>( cx, inValue, wrappedValue );
}

template <>
bool ToValue<std::nullptr_t>( JSContext *, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
    return true;
}

template <>
bool ToValue<metadb_handle_ptr>( JSContext * cx, const metadb_handle_ptr& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedObject jsObject( cx, JsFbMetadbHandle::Create( cx, inValue ) );
    if ( !jsObject )
    {
        return false;
    }

    wrappedValue.setObjectOrNull( jsObject );
    return true;
}

template <>
bool ToValue<metadb_handle_list>( JSContext * cx, const metadb_handle_list& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedObject jsObject( cx, JsFbMetadbHandleList::Create( cx, inValue ) );
    if ( !jsObject )
    {
        return false;
    }

    wrappedValue.setObjectOrNull( jsObject );
    return true;
}

template<>
bool ToValue<Gdiplus::Bitmap*>( JSContext * cx, Gdiplus::Bitmap* const& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedObject jsObject( cx, JsGdiBitmap::Create( cx, inValue ) );
    if ( !jsObject )
    {
        return false;
    }

    wrappedValue.setObjectOrNull( jsObject );
    return true;
}

}
