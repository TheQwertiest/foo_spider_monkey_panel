#include <stdafx.h>

#include "native_to_js.h"

#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_playback_queue_item.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/global_object.h>

namespace mozjs::convert::to_js
{

template <>
void ToValue( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> inValue, JS::MutableHandleValue wrappedValue )
{
    if ( !inValue )
    { // Not an error
        wrappedValue.setNull();
        return;
    }

    wrappedValue.setObjectOrNull( JsGdiBitmap::CreateJs( cx, std::move( inValue ) ) );
}

template <>
void ToValue( JSContext*, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( inValue );
}

template <>
void ToValue( JSContext*, JS::HandleValue inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.set( inValue );
}

template <>
void ToValue( JSContext*, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
}

template <>
void ToValue( JSContext*, const int8_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( static_cast<int32_t>( inValue ) );
}

template <>
void ToValue( JSContext*, const uint8_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( static_cast<uint32_t>( inValue ) );
}

template <>
void ToValue( JSContext*, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
}

template <>
void ToValue( JSContext*, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
}

template <>
void ToValue( JSContext*, const int64_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setDouble( static_cast<double>( inValue ) );
}

template <>
void ToValue( JSContext*, const uint64_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setDouble( static_cast<double>( inValue ) );
}

template <>
void ToValue( JSContext*, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
}

template <>
void ToValue( JSContext*, const float& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
}

template <>
void ToValue( JSContext* cx, const pfc::string8_fast& inValue, JS::MutableHandleValue wrappedValue )
{
    ToValue<std::wstring_view>( cx, qwr::unicode::ToWide( qwr::u8string_view{ inValue.c_str(), inValue.length() } ), wrappedValue );
}

template <>
void ToValue( JSContext* cx, const qwr::u8string& inValue, JS::MutableHandleValue wrappedValue )
{
    ToValue<std::wstring_view>( cx, qwr::unicode::ToWide( inValue ), wrappedValue );
}

template <>
void ToValue( JSContext* cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedString jsString( cx, JS_NewUCStringCopyN( cx, reinterpret_cast<const char16_t*>( inValue.data() ), inValue.length() ) );
    if ( !jsString )
    {
        throw smp::JsException();
    }

    wrappedValue.setString( jsString );
}

template <>
void ToValue( JSContext* cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue )
{
    ToValue<std::wstring_view>( cx, inValue, wrappedValue );
}

template <>
void ToValue( JSContext* /*cx*/, const std::nullptr_t& /*inValue*/, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
}

template <>
void ToValue( JSContext* cx, const metadb_handle_ptr& inValue, JS::MutableHandleValue wrappedValue )
{
    if ( inValue.is_empty() )
    { // Not an error
        wrappedValue.setNull();
        return;
    }

    wrappedValue.setObjectOrNull( JsFbMetadbHandle::CreateJs( cx, inValue ) );
}

template <>
void ToValue( JSContext* cx, const metadb_handle_list& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( JsFbMetadbHandleList::CreateJs( cx, inValue ) );
}

template <>
void ToValue( JSContext* cx, const metadb_handle_list_cref& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( JsFbMetadbHandleList::CreateJs( cx, inValue ) );
}

template <>
void ToValue( JSContext* cx, const t_playback_queue_item& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( JsFbPlaybackQueueItem::CreateJs( cx, inValue ) );
}

} // namespace mozjs::convert::to_js
