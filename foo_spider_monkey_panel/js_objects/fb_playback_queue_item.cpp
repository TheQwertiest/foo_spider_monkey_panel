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
    JsFinalizeOp<JsFbPlaybackQueueItem>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbPlaybackQueueItem",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaybackQueueItem, get_Handle )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaybackQueueItem, get_PlaylistIndex )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaybackQueueItem, get_PlaylistItemIndex )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Handle", get_Handle, DefaultPropsFlags() ),
    JS_PSG( "PlaylistIndex", get_PlaylistIndex, DefaultPropsFlags() ),
    JS_PSG( "PlaylistItemIndex", get_PlaylistItemIndex, DefaultPropsFlags() ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{

const JSClass JsFbPlaybackQueueItem::JsClass = jsClass;
const JSFunctionSpec* JsFbPlaybackQueueItem::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbPlaybackQueueItem::JsProperties = jsProperties;
const JsPrototypeId JsFbPlaybackQueueItem::PrototypeId = JsPrototypeId::FbPlaybackQueueItem;

JsFbPlaybackQueueItem::JsFbPlaybackQueueItem( JSContext* cx, const t_playback_queue_item& playbackQueueItem )
    : pJsCtx_( cx )
    , playbackQueueItem_( playbackQueueItem )
{
}


JsFbPlaybackQueueItem::~JsFbPlaybackQueueItem()
{
}

std::unique_ptr<JsFbPlaybackQueueItem> 
JsFbPlaybackQueueItem::CreateNative( JSContext* cx, const t_playback_queue_item& playbackQueueItem )
{
    return std::unique_ptr<JsFbPlaybackQueueItem>( new JsFbPlaybackQueueItem( cx, playbackQueueItem ) );
}

size_t JsFbPlaybackQueueItem::GetInternalSize( const t_playback_queue_item& playbackQueueItem )
{
    return 0;
}

std::optional<JSObject*> 
JsFbPlaybackQueueItem::get_Handle()
{
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::Create( pJsCtx_, playbackQueueItem_.m_handle ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    return jsObject;
}

std::optional<uint32_t> 
JsFbPlaybackQueueItem::get_PlaylistIndex()
{
    return playbackQueueItem_.m_playlist;
}

std::optional<uint32_t> 
JsFbPlaybackQueueItem::get_PlaylistItemIndex()
{
    return playbackQueueItem_.m_item;
}

}
