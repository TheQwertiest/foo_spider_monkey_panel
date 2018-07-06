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

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, Purge )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistRecyclerManager, Restore )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Purge", Purge, 1, DefaultPropsFlags() ),
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

std::optional<std::nullptr_t> 
JsFbPlaylistRecyclerManager::Purge( JS::HandleValue affectedItems )
{
    JS::RootedObject jsObject( pJsCtx_, affectedItems.toObjectOrNull() );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "affectedItems argument is not a JS object" );
        return std::nullopt;
    }

    bool is;
    if ( !JS_IsArrayObject( pJsCtx_, jsObject, &is ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "affectedItems argument is an array" );
        return std::nullopt;
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( pJsCtx_, jsObject, &arraySize ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Failed to get affectedItems argument array length" );
        return std::nullopt;
    }

    auto api = playlist_manager_v3::get();
    pfc::bit_array_bittable affected( api->recycler_get_count() );

    JS::RootedValue arrayElement( pJsCtx_ );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( pJsCtx_, jsObject, i, &arrayElement ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Failed to get affectedItems[%ud]", i );
            return std::nullopt;
        }

        bool isValid;
        uint32_t affectedIdx( convert::to_native::ToValue<uint32_t>( pJsCtx_, arrayElement, isValid ) );
        if ( !isValid )
        {
            JS_ReportErrorUTF8( pJsCtx_, "affectedItems[%ud] can't be converted to number" );
            return std::nullopt;
        }

        affected.set( affectedIdx, true );
    }

    api->recycler_purge( affected );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistRecyclerManager::Restore( uint32_t index )
{
    auto api = playlist_manager_v3::get();
    t_size count = api->recycler_get_count();
    if ( index >= count )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    api->recycler_restore( index );
    return nullptr;
}

std::optional<JSObject*> 
JsFbPlaylistRecyclerManager::get_Content( uint32_t index )
{
    auto api = playlist_manager_v3::get();
    t_size count = api->recycler_get_count();
    if ( index >= count )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    metadb_handle_list handles;
    playlist_manager_v3::get()->recycler_get_content( index, handles );    

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, handles ) );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<uint32_t> 
JsFbPlaylistRecyclerManager::get_Count()
{
    return playlist_manager_v3::get()->recycler_get_count();
}

std::optional<pfc::string8_fast>
JsFbPlaylistRecyclerManager::get_Name( uint32_t index )
{
    auto api = playlist_manager_v3::get();
    t_size count = api->recycler_get_count();
    if ( index >= count )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    pfc::string8_fast name;
    playlist_manager_v3::get()->recycler_get_name( index, name );
    return name.c_str();
}

}
