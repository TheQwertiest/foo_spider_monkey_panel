#include <stdafx.h>

#include "fb_playlist_recycler_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/string_helpers.h>

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
    JsFbPlaylistRecyclerManager::FinalizeJsObject,
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

MJS_DEFINE_JS_FN_FROM_NATIVE( Purge, JsFbPlaylistRecyclerManager::Purge )
MJS_DEFINE_JS_FN_FROM_NATIVE( Restore, JsFbPlaylistRecyclerManager::Restore )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Purge", Purge, 1, DefaultPropsFlags() ),
    JS_FN( "Restore", Restore, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Content, JsFbPlaylistRecyclerManager::get_Content )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Count, JsFbPlaylistRecyclerManager::get_Count )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Name, JsFbPlaylistRecyclerManager::get_Name )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Content", get_Content, DefaultPropsFlags() ),
    JS_PSG( "Count", get_Count, DefaultPropsFlags() ),
    JS_PSG( "Name", get_Name, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsFbPlaylistRecyclerManager::JsClass = jsClass;
const JSFunctionSpec* JsFbPlaylistRecyclerManager::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbPlaylistRecyclerManager::JsProperties = jsProperties;

JsFbPlaylistRecyclerManager::JsFbPlaylistRecyclerManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsFbPlaylistRecyclerManager::~JsFbPlaylistRecyclerManager()
{
}

std::unique_ptr<JsFbPlaylistRecyclerManager>
JsFbPlaylistRecyclerManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsFbPlaylistRecyclerManager>( new JsFbPlaylistRecyclerManager( cx ) );
}

size_t JsFbPlaylistRecyclerManager::GetInternalSize()
{
    return 0;
}

void JsFbPlaylistRecyclerManager::Purge( JS::HandleValue affectedItems )
{
    JS::RootedObject jsObject( pJsCtx_, affectedItems.toObjectOrNull() );
    if ( !jsObject )
    {
        throw smp::SmpException( "affectedItems argument is not a JS object" );
    }

    bool is;
    if ( !JS_IsArrayObject( pJsCtx_, jsObject, &is ) && !is )
    {
        throw smp::SmpException( "affectedItems argument is not an array" );
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( pJsCtx_, jsObject, &arraySize ) )
    {
        throw smp::SmpException( "Failed to get affectedItems argument array length" );
    }

    auto api = playlist_manager_v3::get();
    pfc::bit_array_bittable affected( api->recycler_get_count() );

    JS::RootedValue arrayElement( pJsCtx_ );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( pJsCtx_, jsObject, i, &arrayElement ) )
        {
            throw smp::SmpException( smp::string::Formatter() << "Failed to get affectedItems[" << i << "]" );
        }

        auto retVal = convert::to_native::ToValue<uint32_t>( pJsCtx_, arrayElement );
        if ( !retVal )
        {
            throw smp::SmpException( smp::string::Formatter() << "affectedItems[" << i << "] can't be converted to number" );
        }

        affected.set( retVal.value(), true );
    }

    api->recycler_purge( affected );
}

void JsFbPlaylistRecyclerManager::Restore( uint32_t index )
{
    auto api = playlist_manager_v3::get();
    t_size count = api->recycler_get_count();
    if ( index >= count )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }

    api->recycler_restore( index );
}

JSObject* JsFbPlaylistRecyclerManager::get_Content( uint32_t index )
{
    auto api = playlist_manager_v3::get();
    t_size count = api->recycler_get_count();
    if ( index >= count )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }

    metadb_handle_list handles;
    playlist_manager_v3::get()->recycler_get_content( index, handles );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, handles ) );
    if ( !jsObject )
    { //TODO: remove
        throw smp::JsException();
    }

    return jsObject;
}

uint32_t JsFbPlaylistRecyclerManager::get_Count()
{
    return playlist_manager_v3::get()->recycler_get_count();
}

pfc::string8_fast JsFbPlaylistRecyclerManager::get_Name( uint32_t index )
{
    auto api = playlist_manager_v3::get();
    t_size count = api->recycler_get_count();
    if ( index >= count )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }

    pfc::string8_fast name;
    playlist_manager_v3::get()->recycler_get_name( index, name );
    return name.c_str();
}

} // namespace mozjs
