#include <stdafx.h>

#include "fb_playback_queue_item.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
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
    JsFbPlaybackQueueItem::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbPlaybackQueueItem",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Handle, JsFbPlaybackQueueItem::get_Handle )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaylistIndex, JsFbPlaybackQueueItem::get_PlaylistIndex )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaylistItemIndex, JsFbPlaybackQueueItem::get_PlaylistItemIndex )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Handle", get_Handle, kDefaultPropsFlags ),
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

const JSClass JsFbPlaybackQueueItem::JsClass = jsClass;
const JSFunctionSpec* JsFbPlaybackQueueItem::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbPlaybackQueueItem::JsProperties = jsProperties.data();
const JsPrototypeId JsFbPlaybackQueueItem::PrototypeId = JsPrototypeId::FbPlaybackQueueItem;

JsFbPlaybackQueueItem::JsFbPlaybackQueueItem( JSContext* cx, const t_playback_queue_item& playbackQueueItem )
    : pJsCtx_( cx )
    , playbackQueueItem_( playbackQueueItem )
{
}

std::unique_ptr<JsFbPlaybackQueueItem>
JsFbPlaybackQueueItem::CreateNative( JSContext* cx, const t_playback_queue_item& playbackQueueItem )
{
    return std::unique_ptr<JsFbPlaybackQueueItem>( new JsFbPlaybackQueueItem( cx, playbackQueueItem ) );
}

size_t JsFbPlaybackQueueItem::GetInternalSize( const t_playback_queue_item& /*playbackQueueItem*/ )
{
    return 0;
}

JSObject* JsFbPlaybackQueueItem::get_Handle()
{
    return JsFbMetadbHandle::CreateJs( pJsCtx_, playbackQueueItem_.m_handle );
}

uint32_t JsFbPlaybackQueueItem::get_PlaylistIndex()
{
    return playbackQueueItem_.m_playlist;
}

uint32_t JsFbPlaybackQueueItem::get_PlaylistItemIndex()
{
    return playbackQueueItem_.m_item;
}

} // namespace mozjs
