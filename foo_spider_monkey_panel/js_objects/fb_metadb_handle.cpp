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
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_Length )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_Path )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_RawPath )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, get_SubSong )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Length",  get_Length, 0 ),
    JS_PSG( "Path",    get_Path, 0 ),
    JS_PSG( "RawPath", get_RawPath, 0 ),
    JS_PSG( "SubSong", get_SubSong, 0 ),
    JS_PS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, ClearStats )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, Compare )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, RefreshStats )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetFirstPlayed )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetLastPlayed )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetLoved )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetPlaycount )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbMetadbHandle, SetRating )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "ClearStats",    ClearStats, 0, 0 ),
    JS_FN( "Compare",       Compare, 1, 0 ),
    JS_FN( "RefreshStats",  RefreshStats, 0, 0 ),
    JS_FN( "SetFirstPlayed",SetFirstPlayed, 1, 0 ),
    JS_FN( "SetLastPlayed", SetLastPlayed, 1, 0 ),
    JS_FN( "SetLoved",      SetLoved, 1, 0 ),
    JS_FN( "SetPlaycount",  SetPlaycount, 1, 0 ),
    JS_FN( "SetRating",     SetRating, 1, 0 ),
    JS_FS_END
};

}

namespace mozjs
{


JsFbMetadbHandle::JsFbMetadbHandle( JSContext* cx, const metadb_handle_ptr& handle )
    : pJsCtx_( cx )
    , metadbHandle_( handle )
{
}


JsFbMetadbHandle::~JsFbMetadbHandle()
{
}

JSObject* JsFbMetadbHandle::Create( JSContext* cx, const metadb_handle_ptr& handle )
{
    if ( !handle.is_valid() )
    {
        JS_ReportErrorASCII( cx, "Internal error: metadb_handle_ptr is null" );
        return nullptr;
    }    

    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties(cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsFbMetadbHandle( cx, handle ) );

    return jsObj;
}

const JSClass& JsFbMetadbHandle::GetClass()
{
    return jsClass;
}

metadb_handle_ptr& JsFbMetadbHandle::GetHandle()
{
    return metadbHandle_;
}

/*

STDMETHODIMP FbMetadbHandle::get_FileSize(LONGLONG* p)
{
    if (metadbHandle_.is_empty() || !p) return E_POINTER;

    *p = metadbHandle_->get_filesize();
    return nullptr;
}

*/

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
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
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

    JS::RootedObject jsObject( pJsCtx_, JsFbFileInfo::Create( pJsCtx_, pFileInfo.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    pFileInfo.release();
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
JsFbMetadbHandle::SetFirstPlayed( std::string first_played )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( !tmp.first_played.equals( first_played.c_str() ) )
        {
            tmp.first_played = first_played.c_str();
            stats::set( hash, tmp );
        }
    }

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandle::SetLastPlayed( std::string last_played )
{
    assert( metadbHandle_.is_valid() );

    metadb_index_hash hash;
    if ( stats::g_client->hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( !tmp.last_played.equals( last_played.c_str() ) )
        {
            tmp.last_played = last_played.c_str();
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

std::optional<double> 
JsFbMetadbHandle::get_Length()
{
    assert( metadbHandle_.is_valid() );
    return metadbHandle_->get_length();
}

std::optional<std::string> 
JsFbMetadbHandle::get_Path()
{
    assert( metadbHandle_.is_valid() );
    return file_path_display( metadbHandle_->get_path() ).get_ptr();
}

std::optional<std::string> 
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
