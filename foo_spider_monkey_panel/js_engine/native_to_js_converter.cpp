#include <stdafx.h>
#include "native_to_js_converter.h"

#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/gdi_bitmap.h>

namespace mozjs::convert::to_js
{

template <>
bool ToValue( JSContext *, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( inValue );
    return true;
}

template <>
bool ToValue( JSContext *, JS::HandleValue inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.set( inValue );
    return true;
}

template <>
bool ToValue( JSContext *, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
    return true;
}

template <>
bool ToValue( JSContext *, const int8_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( static_cast<int32_t>(inValue) );
    return true;
}

template <>
bool ToValue( JSContext *, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
    return true;
}

template <>
bool ToValue( JSContext *, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue( JSContext *, const uint64_t& inValue, JS::MutableHandleValue wrappedValue )
{    
    wrappedValue.setDouble( static_cast<double>(inValue) );
    return true;
}

template <>
bool ToValue( JSContext *, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue( JSContext *, const float& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue( JSContext * cx, const pfc::string8_fast& inValue, JS::MutableHandleValue wrappedValue )
{
    pfc::stringcvt::string_wide_from_utf8 inValue8( inValue );
    return ToValue<std::wstring_view>( cx, inValue8.get_ptr(), wrappedValue );
}

template <>
bool ToValue( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue )
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
bool ToValue( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue )
{
    return ToValue<std::wstring_view>( cx, inValue, wrappedValue );
}

template <>
bool ToValue( JSContext *, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
    return true;
}

template <>
bool ToValue( JSContext * cx, const metadb_handle_ptr& inValue, JS::MutableHandleValue wrappedValue )
{
    if ( inValue.is_empty() )
    {// Not an error
        wrappedValue.setNull();
        return true;
    }

    JS::RootedObject jsObject( cx, JsFbMetadbHandle::Create( cx, inValue ) );
    if ( !jsObject )
    {
        return false;
    }

    wrappedValue.setObjectOrNull( jsObject );
    return true;
}

template <>
bool ToValue( JSContext * cx, const metadb_handle_list& inValue, JS::MutableHandleValue wrappedValue )
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
bool ToValue( JSContext * cx, Gdiplus::Bitmap* const& inValue, JS::MutableHandleValue wrappedValue )
{
    if ( !inValue )
    {// Not an error
        wrappedValue.setNull();
        return true;
    }

    JS::RootedObject jsObject( cx, JsGdiBitmap::Create( cx, inValue ) );
    if ( !jsObject )
    {
        return false;
    }

    wrappedValue.setObjectOrNull( jsObject );
    return true;
}

}
