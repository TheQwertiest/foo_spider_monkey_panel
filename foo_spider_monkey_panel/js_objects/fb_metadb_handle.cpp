#include <stdafx.h>

#include "fb_metadb_handle.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_file_info.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <stats.h>

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
    JsFinalizeOp<JsFbMetadbHandle>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbMetadbHandle",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, ClearStats )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, Compare )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, GetFileInfo )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, RefreshStats )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetFirstPlayed )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetLastPlayed )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetLoved )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetPlaycount )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetRating )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "ClearStats",    ClearStats, 0, DefaultPropsFlags() ),
    JS_FN( "Compare",       Compare, 1, DefaultPropsFlags() ),
    JS_FN( "GetFileInfo",   GetFileInfo, 0, DefaultPropsFlags() ),
    JS_FN( "RefreshStats",  RefreshStats, 0, DefaultPropsFlags() ),
    JS_FN( "SetFirstPlayed",SetFirstPlayed, 1, DefaultPropsFlags() ),
    JS_FN( "SetLastPlayed", SetLastPlayed, 1, DefaultPropsFlags() ),
    JS_FN( "SetLoved",      SetLoved, 1, DefaultPropsFlags() ),
    JS_FN( "SetPlaycount",  SetPlaycount, 1, DefaultPropsFlags() ),
    JS_FN( "SetRating",     SetRating, 1, DefaultPropsFlags() ),
    JS_FS_END
};


MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_FileSize )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_Length )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_Path )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_RawPath )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_SubSong )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "FileSize", get_FileSize, DefaultPropsFlags() ),
    JS_PSG( "Length",   get_Length, DefaultPropsFlags() ),
    JS_PSG( "Path",     get_Path, DefaultPropsFlags() ),
    JS_PSG( "RawPath",  get_RawPath, DefaultPropsFlags() ),
    JS_PSG( "SubSong",  get_SubSong, DefaultPropsFlags() ),
    JS_PS_END
};


}

namespace mozjs
{

const JSClass JsFbMetadbHandle::JsClass = jsClass;
const JSFunctionSpec* JsFbMetadbHandle::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbMetadbHandle::JsProperties = jsProperties;
const JsPrototypeId JsFbMetadbHandle::PrototypeId = JsPrototypeId::FbMetadbHandle;

JsFbMetadbHandle::JsFbMetadbHandle( JSContext* cx, const metadb_handle_ptr& handle )
    : pJsCtx_( cx )
    , metadbHandle_( handle )
{
}

JsFbMetadbHandle::~JsFbMetadbHandle()
{
}

std::unique_ptr<mozjs::JsFbMetadbHandle> 
JsFbMetadbHandle::CreateNative( JSContext* cx, const metadb_handle_ptr& handle )
{
    if ( !handle.is_valid() )
    {
        JS_ReportErrorUTF8( cx, "Internal error: metadb_handle_ptr is null" );
        return nullptr;
    }

    return std::unique_ptr<JsFbMetadbHandle>( new JsFbMetadbHandle( cx, handle ) );
}

size_t JsFbMetadbHandle::GetInternalSize( const metadb_handle_ptr& handle )
{
    return sizeof( metadb_handle );
}

metadb_handle_ptr& JsFbMetadbHandle::GetHandle()
{
    assert( metadbHandle_.is_valid() );
    return metadbHandle_;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::ClearStats()
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( !stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::set( hash, stats::fields() );
    }

    return nullptr;
}

std::optional<bool> 
JsFbMetadbHandle::Compare( JsFbMetadbHandle* handle )
{
    assert( metadbHandle_.is_valid() );

    if ( !handle )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr otherHandle ( handle->GetHandle());
    if ( otherHandle.is_empty() )
    {
        return false;
    }

    return otherHandle == metadbHandle_;
}

std::optional<JSObject*> 
JsFbMetadbHandle::GetFileInfo()
{
    assert( metadbHandle_.is_valid() );

    std::unique_ptr<file_info_impl> pFileInfo(new file_info_impl);

    if ( !metadbHandle_->get_info( *pFileInfo ) )
    {// Not an error: info not loaded yet
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbFileInfo::Create( pJsCtx_, std::move(pFileInfo) ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::RefreshStats()
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::theAPI()->dispatch_refresh( g_guid_jsp_metadb_index, hash );
    }

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::SetFirstPlayed( const pfc::string8_fast& first_played )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( !tmp.first_played.equals( first_played.c_str() ) )
        {
            tmp.first_played.set_string_nc( first_played.c_str(), first_played.length() );
            stats::set( hash, tmp );
        }
    }

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::SetLastPlayed( const pfc::string8_fast& last_played )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( !tmp.last_played.equals( last_played.c_str() ) )
        {
            tmp.last_played.set_string_nc( last_played.c_str(), last_played.length() );
            stats::set( hash, tmp );
        }
    }

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::SetLoved( uint32_t loved )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.loved != loved )
        {
            tmp.loved = loved;
            stats::set( hash, tmp );
        }
    }

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::SetPlaycount( uint32_t playcount )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.playcount != playcount )
        {
            tmp.playcount = playcount;
            stats::set( hash, tmp );
        }
    }

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::SetRating( uint32_t rating )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.rating != rating )
        {
            tmp.rating = rating;
            stats::set( hash, tmp );
        }
    }

    return nullptr;
}

std::optional<std::uint64_t> 
JsFbMetadbHandle::get_FileSize()
{
    assert( metadbHandle_.is_valid() );
    return static_cast<uint64_t>(metadbHandle_->get_filesize());
}

std::optional<double> 
JsFbMetadbHandle::get_Length()
{
    assert( metadbHandle_.is_valid() );
    return metadbHandle_->get_length();
}

std::optional<pfc::string8_fast> 
JsFbMetadbHandle::get_Path()
{
    assert( metadbHandle_.is_valid() );
    return file_path_display( metadbHandle_->get_path() ).get_ptr();
}

std::optional<pfc::string8_fast> 
JsFbMetadbHandle::get_RawPath()
{
    assert( metadbHandle_.is_valid() );
    return metadbHandle_->get_path();
}

std::optional<std::uint32_t> 
JsFbMetadbHandle::get_SubSong()
{
    assert( metadbHandle_.is_valid() );
    return metadbHandle_->get_subsong_index();
}

}
