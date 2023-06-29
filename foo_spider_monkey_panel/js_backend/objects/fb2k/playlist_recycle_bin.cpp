#include <stdafx.h>

#include "playlist_recycle_bin.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track_list.h>

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
    PlaylistRecycleBin::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaylistRecycleBin",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( getContent, PlaylistRecycleBin::GetContent )
MJS_DEFINE_JS_FN_FROM_NATIVE( getName, PlaylistRecycleBin::GetName )
MJS_DEFINE_JS_FN_FROM_NATIVE( purge, PlaylistRecycleBin::Purge )
MJS_DEFINE_JS_FN_FROM_NATIVE( restore, PlaylistRecycleBin::Restore )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getContent", getContent, 1, kDefaultPropsFlags ),
        JS_FN( "getName", getName, 1, kDefaultPropsFlags ),
        JS_FN( "purge", purge, 1, kDefaultPropsFlags ),
        JS_FN( "restore", restore, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_length, PlaylistRecycleBin::get_Length )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "length", get_length, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaylistRecycleBin>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<PlaylistRecycleBin>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<PlaylistRecycleBin>::JsProperties = jsProperties.data();

PlaylistRecycleBin::PlaylistRecycleBin( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<PlaylistRecycleBin>
PlaylistRecycleBin::CreateNative( JSContext* cx )
{
    return std::unique_ptr<PlaylistRecycleBin>( new PlaylistRecycleBin( cx ) );
}

size_t PlaylistRecycleBin::GetInternalSize()
{
    return 0;
}

JSObject* PlaylistRecycleBin::GetContent( uint32_t index )
{
    auto api = playlist_manager_v3::get();

    const t_size count = api->recycler_get_count();
    qwr::QwrException::ExpectTrue( index < count, "Index is out of bounds" );

    metadb_handle_list handles;
    playlist_manager_v3::get()->recycler_get_content( index, handles );

    return TrackList::CreateJs( pJsCtx_, handles );
}

pfc::string8_fast PlaylistRecycleBin::GetName( uint32_t index )
{
    auto api = playlist_manager_v3::get();

    const t_size count = api->recycler_get_count();
    qwr::QwrException::ExpectTrue( index < count, "Index is out of bounds" );

    pfc::string8_fast name;
    playlist_manager_v3::get()->recycler_get_name( index, name );
    return name;
}

void PlaylistRecycleBin::Purge( const std::vector<uint32_t>& indices )
{
    auto api = playlist_manager_v3::get();

    pfc::bit_array_bittable affected;
    for ( auto i: indices )
    {
        affected.set( i, true );
    }

    api->recycler_purge( affected );
}

void PlaylistRecycleBin::Restore( uint32_t index )
{
    auto api = playlist_manager_v3::get();

    const t_size count = api->recycler_get_count();
    qwr::QwrException::ExpectTrue( index < count, "Index is out of bounds" );

    api->recycler_restore( index );
}

uint32_t PlaylistRecycleBin::get_Length()
{
    return playlist_manager_v3::get()->recycler_get_count();
}

} // namespace mozjs
