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
    JsFinalizeOp<JsFbPlayingItemLocation>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlayingItemLocation",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlayingItemLocation, get_IsValid )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlayingItemLocation, get_PlaylistIndex )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlayingItemLocation, get_PlaylistItemIndex )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "IsValid", get_IsValid, DefaultPropsFlags() ),
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


JsFbPlayingItemLocation::JsFbPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex )
    : pJsCtx_( cx )
    , isValid_(isValid)
    , playlistIndex_(playlistIndex)
    , playlistItemIndex_(playlistItemIndex)
{
}


JsFbPlayingItemLocation::~JsFbPlayingItemLocation()
{
}

JSObject* JsFbPlayingItemLocation::Create( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex )
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

    JS_SetPrivate( jsObj, new JsFbPlayingItemLocation( cx, isValid, playlistIndex, playlistItemIndex ) );

    return jsObj;
}

const JSClass& JsFbPlayingItemLocation::GetClass()
{
    return jsClass;
}

std::optional<bool> 
JsFbPlayingItemLocation::get_IsValid()
{    
    return isValid_;
}

std::optional<uint32_t> 
JsFbPlayingItemLocation::get_PlaylistIndex()
{    
    return playlistIndex_;
}

std::optional<uint32_t> 
JsFbPlayingItemLocation::get_PlaylistItemIndex()
{    
    return playlistItemIndex_;
}

}
