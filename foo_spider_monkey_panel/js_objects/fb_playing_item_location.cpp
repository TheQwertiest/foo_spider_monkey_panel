#include <stdafx.h>

#include "fb_playing_item_location.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFbPlayingItemLocation::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlayingItemLocation",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsValid, JsFbPlayingItemLocation::get_IsValid )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaylistIndex, JsFbPlayingItemLocation::get_PlaylistIndex )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaylistItemIndex, JsFbPlayingItemLocation::get_PlaylistItemIndex )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "IsValid", get_IsValid, kDefaultPropsFlags ),
        JS_PSG( "PlaylistIndex", get_PlaylistIndex, kDefaultPropsFlags ),
        JS_PSG( "PlaylistItemIndex", get_PlaylistItemIndex, kDefaultPropsFlags ),
        JS_PS_END,
    } );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbPlayingItemLocation::JsClass = jsClass;
const JSFunctionSpec* JsFbPlayingItemLocation::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbPlayingItemLocation::JsProperties = jsProperties.data();
const JsPrototypeId JsFbPlayingItemLocation::PrototypeId = JsPrototypeId::FbPlayingItemLocation;

JsFbPlayingItemLocation::JsFbPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex )
    : pJsCtx_( cx )
    , isValid_( isValid )
    , playlistIndex_( playlistIndex )
    , playlistItemIndex_( playlistItemIndex )
{
}

std::unique_ptr<JsFbPlayingItemLocation>
JsFbPlayingItemLocation::CreateNative( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    return std::unique_ptr<JsFbPlayingItemLocation>( new JsFbPlayingItemLocation( cx, isValid, playlistIndex, playlistItemIndex ) );
}

size_t JsFbPlayingItemLocation::GetInternalSize( bool /*isValid*/, uint32_t /*playlistIndex*/, uint32_t /*playlistItemIndex*/ )
{
    return 0;
}

bool JsFbPlayingItemLocation::get_IsValid()
{
    return isValid_;
}

uint32_t JsFbPlayingItemLocation::get_PlaylistIndex()
{
    return playlistIndex_;
}

uint32_t JsFbPlayingItemLocation::get_PlaylistItemIndex()
{
    return playlistItemIndex_;
}

} // namespace mozjs
