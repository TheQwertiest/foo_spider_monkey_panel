#include <stdafx.h>

#include "track.h"

#include <fb2k/title_format_manager.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track_info_snapshot.h>

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
    Track::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Track",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( isEqual, Track::IsEqual )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( formatTitle, Track::FormatTitle, Track::FormatTitleWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "isEqual", isEqual, 1, kDefaultPropsFlags ),
        JS_FN( "formatTitle", formatTitle, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_displayPath, Track::get_DisplayPath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_infoSnapshot, Track::get_InfoSnapshot )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_path, Track::get_Path )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_subTrackIndex, Track::get_SubTrackIndex )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "displayPath", get_displayPath, kDefaultPropsFlags ),
        JS_PSG( "infoSnapshot", get_infoSnapshot, kDefaultPropsFlags ),
        JS_PSG( "path", get_path, kDefaultPropsFlags ),
        JS_PSG( "subTrackIndex", get_subTrackIndex, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Track_Constructor, Track::Constructor_2, Track::ConstructorWithOpt, 2 )

MJS_VERIFY_OBJECT( mozjs::Track );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Track>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<Track>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<Track>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<Track>::PrototypeId = JsPrototypeId::New_Track;
const JSNative JsObjectTraits<Track>::JsConstructor = Track_Constructor;

Track::Track( JSContext* cx, metadb_handle_ptr metadbHandle )
    : pJsCtx_( cx )
    , metadbHandle_( metadbHandle )
{
}

Track::~Track()
{
}

std::unique_ptr<Track>
Track::CreateNative( JSContext* cx, metadb_handle_ptr metadbHandle )
{
    return std::unique_ptr<Track>( new Track( cx, metadbHandle ) );
}

size_t Track::GetInternalSize() const
{
    return 0;
}

metadb_handle_ptr Track::GetHandle() const
{
    return metadbHandle_;
}

JSObject* Track::Constructor_1( JSContext* cx )
{
    // fake handle
    return CreateJs( cx, metadb::get()->handle_create( playable_location_impl{} ) );
}

JSObject* Track::Constructor_2( JSContext* cx, const qwr::u8string& path, uint32_t subTrackIndex )
{
    return CreateJs( cx, metadb::get()->handle_create( make_playable_location{ path.c_str(), subTrackIndex } ) );
}

JSObject* Track::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& path, uint32_t subTrackIndex )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor_2( cx, path, subTrackIndex );
    case 1:
        return Constructor_2( cx, path );
    case 2:
        return Constructor_1( cx );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool Track::IsEqual( smp::not_null<Track*> handle )
{
    return ( metadbHandle_ == handle->GetHandle() );
}

pfc::string8_fast Track::FormatTitle( const qwr::u8string& query, const qwr::u8string& fallback )
{
    auto pTitleFormat = smp::TitleFormatManager::Get().Load( query, fallback );

    pfc::string8_fast text;
    playback_control::get()->playback_format_title_ex( metadbHandle_,
                                                       nullptr,
                                                       text,
                                                       pTitleFormat,
                                                       nullptr,
                                                       playback_control::display_level_all );
    return text;
}

pfc::string8_fast Track::FormatTitleWithOpt( size_t optArgCount, const qwr::u8string& query, const qwr::u8string& fallback )
{
    switch ( optArgCount )
    {
    case 0:
        return FormatTitle( query, fallback );
    case 1:
        return FormatTitle( query );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

pfc::string8_fast Track::get_DisplayPath() const
{
    pfc::string8_fast path{ metadbHandle_->get_path() };
    if ( path.is_empty() )
    { // file_path_display crashes with empty strings
        return path;
    }
    return file_path_display( path );
}

JSObject* Track::get_InfoSnapshot() const
{
    return TrackInfoSnapshot::CreateJs( pJsCtx_, metadbHandle_->get_info_ref() );
}

qwr::u8string Track::get_Path() const
{
    return metadbHandle_->get_path();
}

uint32_t Track::get_SubTrackIndex() const
{
    return metadbHandle_->get_location().get_subsong();
}

} // namespace mozjs
