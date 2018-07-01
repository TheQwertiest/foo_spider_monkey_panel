#include <stdafx.h>

#include "fb_playlist_recycler_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <helpers.h>


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
    JsFinalizeOp<JsFbPlaylistRecyclerManager>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbPlaylistRecyclerManager",
    DefaultClassFlags(),
    &jsOps
};

//MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, Purge )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, Restore )

const JSFunctionSpec jsFunctions[] = {
//  JS_FN( "Purge", Purge, 1, DefaultPropsFlags() ),
    JS_FN( "Restore", Restore, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, get_Content )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, get_Count )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, get_Name )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Content", get_Content, DefaultPropsFlags() ),
    JS_PSG( "Count", get_Count, DefaultPropsFlags() ),
    JS_PSG( "Name", get_Name, DefaultPropsFlags() ),
    JS_PS_END
};

}

namespace mozjs
{


JsFbPlaylistRecyclerManager::JsFbPlaylistRecyclerManager( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsFbPlaylistRecyclerManager::~JsFbPlaylistRecyclerManager()
{
}

JSObject* JsFbPlaylistRecyclerManager::Create( JSContext* cx )
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

    JS_SetPrivate( jsObj, new JsFbPlaylistRecyclerManager( cx ) );

    return jsObj;
}

const JSClass& JsFbPlaylistRecyclerManager::GetClass()
{
    return jsClass;
}
/*
std::optional<std::nullptr_t> 
JsFbPlaylistRecyclerManager::Purge( JS::HandleValue affectedItems )
{
    //try
    //{
        auto api = playlist_manager_v3::get();
        pfc::bit_array_bittable affected( api->recycler_get_count() );
        bool ok;
        //if ( !helpers::com_array_to_bitarray::convert( affectedItems, affected, ok ) )
        //{
            //return E_INVALIDARG;
        //}
        if ( !ok )
        {
            //JS_ReportErrorASCII( pJsCtx_, "points[%d] is not an object", i );
            return std::nullopt;
        }
        api->recycler_purge( affected );
    //}
    //catch ( ... )
    //{
        //return E_INVALIDARG;
    //}

    //return nullptr;
}
*/
std::optional<std::nullptr_t> 
JsFbPlaylistRecyclerManager::Restore( uint32_t index )
{
    //try
    //{
    playlist_manager_v3::get()->recycler_restore( index );
    //}
    //catch ( ... )
    //{
        //return E_INVALIDARG;
    //}

    return nullptr;
}

std::optional<JSObject*> 
JsFbPlaylistRecyclerManager::get_Content( uint32_t index )
{
    //try
    //{
    metadb_handle_list handles;
    playlist_manager_v3::get()->recycler_get_content( index, handles );    

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, handles ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;

    //}
    //catch ( ... )
    //{
        //return E_INVALIDARG;
    //}

    //return nullptr;
}

std::optional<uint32_t> 
JsFbPlaylistRecyclerManager::get_Count()
{
    return playlist_manager_v3::get()->recycler_get_count();
}

std::optional<std::string>
JsFbPlaylistRecyclerManager::get_Name( uint32_t index )
{
    // TODO: test for exceptions

    //try
    //{
    pfc::string8_fast name;
    playlist_manager_v3::get()->recycler_get_name( index, name );
    return name.c_str();
    //}
    //catch ( ... )
    //{
        //return E_INVALIDARG;
    //}
}

}
