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
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaybackQueueItem, get_Handle )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaybackQueueItem, get_PlaylistIndex )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaybackQueueItem, get_PlaylistItemIndex )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Handle", get_Handle, 0 ),
    JS_PSG( "PlaylistIndex", get_PlaylistIndex, 0 ),
    JS_PSG( "PlaylistItemIndex", get_PlaylistItemIndex, 0 ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{


JsFbPlaybackQueueItem::JsFbPlaybackQueueItem( JSContext* cx, const t_playback_queue_item& playbackQueueItem )
    : pJsCtx_( cx )
    , playbackQueueItem_( playbackQueueItem )
{
}


JsFbPlaybackQueueItem::~JsFbPlaybackQueueItem()
{
}

JSObject* JsFbPlaybackQueueItem::Create( JSContext* cx, const t_playback_queue_item& playbackQueueItem )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsFbPlaybackQueueItem( cx, playbackQueueItem ) );

    return jsObj;
}

const JSClass& JsFbPlaybackQueueItem::GetClass()
{
    return jsClass;
}

std::optional<JSObject*> 
JsFbPlaybackQueueItem::get_Handle()
{
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::Create( pJsCtx_, playbackQueueItem_.m_handle ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
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
