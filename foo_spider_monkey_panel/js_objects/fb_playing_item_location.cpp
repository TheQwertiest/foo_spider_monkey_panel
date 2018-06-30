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
    JsFinalizeOp<JsPlayingItemLocation>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlayingItemLocation",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsPlayingItemLocation, get_IsValid )
MJS_DEFINE_JS_TO_NATIVE_FN( JsPlayingItemLocation, get_PlaylistIndex )
MJS_DEFINE_JS_TO_NATIVE_FN( JsPlayingItemLocation, get_PlaylistItemIndex )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "IsValid", get_IsValid, 0 ),
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


JsPlayingItemLocation::JsPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex )
    : pJsCtx_( cx )
    , isValid_(isValid)
    , playlistIndex_(playlistIndex)
    , playlistItemIndex_(playlistItemIndex)
{
}


JsPlayingItemLocation::~JsPlayingItemLocation()
{
}

JSObject* JsPlayingItemLocation::Create( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex )
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

    JS_SetPrivate( jsObj, new JsPlayingItemLocation( cx, isValid, playlistIndex, playlistItemIndex ) );

    return jsObj;
}

const JSClass& JsPlayingItemLocation::GetClass()
{
    return jsClass;
}

std::optional<bool> 
JsPlayingItemLocation::get_IsValid()
{    
    return isValid_;
}

std::optional<uint32_t> 
JsPlayingItemLocation::get_PlaylistIndex()
{    
    return playlistIndex_;
}

std::optional<uint32_t> 
JsPlayingItemLocation::get_PlaylistItemIndex()
{    
    return playlistItemIndex_;
}

}
