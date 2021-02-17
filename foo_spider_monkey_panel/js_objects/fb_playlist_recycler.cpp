#include <stdafx.h>

#include "fb_playlist_recycler.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <qwr/string_helpers.h>

using namespace smp;

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
    JsFbPlaylistRecycler::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbPlaylistRecycler",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( GetContent, JsFbPlaylistRecycler::GetContent )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetName, JsFbPlaylistRecycler::GetName )
MJS_DEFINE_JS_FN_FROM_NATIVE( Purge, JsFbPlaylistRecycler::Purge )
MJS_DEFINE_JS_FN_FROM_NATIVE( Restore, JsFbPlaylistRecycler::Restore )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "GetContent", GetContent, 1, kDefaultPropsFlags ),
        JS_FN( "GetName", GetName, 1, kDefaultPropsFlags ),
        JS_FN( "Purge", Purge, 1, kDefaultPropsFlags ),
        JS_FN( "Restore", Restore, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Count, JsFbPlaylistRecycler::get_Count )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Count", get_Count, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbPlaylistRecycler::JsClass = jsClass;
const JSFunctionSpec* JsFbPlaylistRecycler::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbPlaylistRecycler::JsProperties = jsProperties.data();

JsFbPlaylistRecycler::JsFbPlaylistRecycler( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<JsFbPlaylistRecycler>
JsFbPlaylistRecycler::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsFbPlaylistRecycler>( new JsFbPlaylistRecycler( cx ) );
}

size_t JsFbPlaylistRecycler::GetInternalSize()
{
    return 0;
}

JSObject* JsFbPlaylistRecycler::GetContent( uint32_t index )
{
    auto api = playlist_manager_v3::get();

    const t_size count = api->recycler_get_count();
    qwr::QwrException::ExpectTrue( index < count, "Index is out of bounds" );

    metadb_handle_list handles;
    playlist_manager_v3::get()->recycler_get_content( index, handles );

    return JsFbMetadbHandleList::CreateJs( pJsCtx_, handles );
}

pfc::string8_fast JsFbPlaylistRecycler::GetName( uint32_t index )
{
    auto api = playlist_manager_v3::get();

    const t_size count = api->recycler_get_count();
    qwr::QwrException::ExpectTrue( index < count, "Index is out of bounds" );

    pfc::string8_fast name;
    playlist_manager_v3::get()->recycler_get_name( index, name );
    return name;
}

void JsFbPlaylistRecycler::Purge( JS::HandleValue affectedItems )
{
    auto api = playlist_manager_v3::get();
    pfc::bit_array_bittable affected( api->recycler_get_count() );

    convert::to_native::ProcessArray<uint32_t>( pJsCtx_, affectedItems, [&affected]( uint32_t index ) { affected.set( index, true ); } );

    api->recycler_purge( affected );
}

void JsFbPlaylistRecycler::Restore( uint32_t index )
{
    auto api = playlist_manager_v3::get();

    const t_size count = api->recycler_get_count();
    qwr::QwrException::ExpectTrue( index < count, "Index is out of bounds" );

    api->recycler_restore( index );
}

uint32_t JsFbPlaylistRecycler::get_Count()
{
    return playlist_manager_v3::get()->recycler_get_count();
}

} // namespace mozjs
