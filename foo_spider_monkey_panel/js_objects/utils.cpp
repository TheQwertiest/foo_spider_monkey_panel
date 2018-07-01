#include <stdafx.h>
#include "utils.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/winapi_error_helper.h>
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
    JsFinalizeOp<JsUtils>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Utils",
    DefaultClassFlags(),
    &jsOps
};

// TODO: add methods

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

JsUtils::JsUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsUtils::~JsUtils()
{
}

JSObject* JsUtils::Create( JSContext* cx )
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

    JS_SetPrivate( jsObj, new JsUtils( cx ) );

    return jsObj;
}

const JSClass& JsUtils::GetClass()
{
    return jsClass;
}

std::optional<bool>
JsUtils::CheckComponent( const std::string& name, bool is_dll )
{
    service_enum_t<componentversion> e;
    componentversion::ptr ptr;
    pfc::string8_fast temp;

     while ( e.next( ptr ) )
    {
        if ( is_dll )
        {
            ptr->get_file_name( temp );
        }
        else
        {
            ptr->get_component_name( temp );
        }

        if ( !_stricmp( temp, name.c_str() ) )
        {
            return true;
        }
    }

    return false;
}

std::optional<bool>
JsUtils::CheckFont( const std::wstring& name )
{
    WCHAR family_name_eng[LF_FACESIZE] = { 0 };
    WCHAR family_name_loc[LF_FACESIZE] = { 0 };
    Gdiplus::InstalledFontCollection font_collection;
    int count = font_collection.GetFamilyCount();
    std::unique_ptr<Gdiplus::FontFamily[]> font_families( new Gdiplus::FontFamily[count] );
    int recv;
    
    Gdiplus::Status gdiRet = font_collection.GetFamilies( count, font_families.get(), &recv );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamilies );

    if ( recv == count )
    {// Find
        for ( int i = 0; i < count; ++i )
        {
            font_families[i].GetFamilyName( family_name_eng, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ) );
            font_families[i].GetFamilyName( family_name_loc );

            if ( !_wcsicmp( name.c_str(), family_name_loc )
                 || !_wcsicmp( name.c_str(), family_name_eng ) )
            {
                return true;                
            }
        }
    }

    return false;
}

std::optional<uint32_t>
JsUtils::ColourPicker( uint64_t hWindow, uint32_t default_colour )
{
    COLORREF COLOR = helpers::convert_argb_to_colorref( default_colour );
    COLORREF COLORS[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uChooseColor( &COLOR, (HWND)hWindow, &COLORS[0] );

    return helpers::convert_colorref_to_argb( COLOR );
}

std::optional<std::string>
JsUtils::FormatDuration( double p )
{
    pfc::string8_fast str( pfc::format_time_ex( p, 0 ) );
    return std::string( str.c_str(), str.length() );
}

std::optional<std::string>
JsUtils::FormatFileSize( uint64_t p )
{
    pfc::string8_fast str = pfc::format_file_size_short( p );
    return std::string( str.c_str(), str.length() );
}

std::optional<std::uint64_t>
JsUtils::GetAlbumArtAsync( uint64_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "font argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr ptr = handle->GetHandle();
    assert( ptr.is_valid() );

    try
    {
        std::unique_ptr<helpers::album_art_async> task( new helpers::album_art_async( (HWND)hWnd, ptr.get_ptr(), art_id, need_stub, only_embed, no_load ) );

        if ( !simple_thread_pool::instance().enqueue( task.get() ) )
        {
            return 0;
            
        }
        return reinterpret_cast<uint64_t>(task.release());
    }
    catch ( ... )
    {
        return 0;
    }
}

std::optional<JSObject*>
JsUtils::GetAlbumArtEmbedded( const std::string& rawpath, uint32_t art_id )
{// TODO: rewrite
    service_enum_t<album_art_extractor> e;
    album_art_extractor::ptr ptr;
    pfc::string_extension ext( rawpath.c_str() );
    abort_callback_dummy abort;

    while ( e.next( ptr ) )
    {
        if ( ptr->is_our_path( rawpath.c_str(), ext ) )
        {
            album_art_extractor_instance_ptr aaep;
            GUID what = helpers::convert_artid_to_guid( art_id );

            try
            {
                aaep = ptr->open( nullptr, rawpath.c_str(), abort );

                Gdiplus::Bitmap* bitmap = nullptr;
                album_art_data_ptr data = aaep->query( what, abort );
                // TODO: rewrite read_album_art_into_bitmap
                if ( helpers::read_album_art_into_bitmap( data, &bitmap ) )
                {
                    std::unique_ptr<Gdiplus::Bitmap> autoBitmap( bitmap );
                    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, bitmap ) );
                    if ( !jsObject )
                    {
                        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
                        return std::nullopt;
                    }

                    autoBitmap.release();
                    return jsObject;                    
                }
            }
            catch ( ... )
            {
            }
        }
    }
    
    return nullptr;
}

std::optional<JSObject*>
JsUtils::GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{// TODO: rewrite
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "font argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr ptr = handle->GetHandle();
    assert( ptr.is_valid() );    

    GUID what = helpers::convert_artid_to_guid( art_id );
    abort_callback_dummy abort;
    auto aamv2 = album_art_manager_v2::get();
    album_art_extractor_instance_v2::ptr aaeiv2;
    IGdiBitmap* ret = NULL;

    try
    {
        aaeiv2 = aamv2->open( pfc::list_single_ref_t<metadb_handle_ptr>( handle ), pfc::list_single_ref_t<GUID>( what ), abort );

        ret = helpers::query_album_art( aaeiv2, what, no_load, image_path_ptr );
    }
    catch ( ... )
    {
        if ( need_stub )
        {
            album_art_extractor_instance_v2::ptr aaeiv2_stub = aamv2->open_stub( abort );

            try
            {
                album_art_data_ptr data = aaeiv2_stub->query( what, abort );
                ret = helpers::query_album_art( aaeiv2_stub, what, no_load, image_path_ptr );
            }
            catch ( ... )
            {
            }
        }
    }

    *pp = ret;
    return S_OK;
}

std::optional<uint32_t>
JsUtils::GetSysColour( uint32_t index )
{
    if ( !::GetSysColorBrush( index ))
    {
        // invalid index
        return 0;
    }
   
    int col = ::GetSysColor( index );
    return helpers::convert_colorref_to_argb( col );
}

std::optional<uint32_t>
JsUtils::GetSystemMetrics( uint32_t index )
{
    return ::GetSystemMetrics( index );    
}

std::optional<bool>
JsUtils::IsKeyPressed( uint32_t vkey )
{
    return ::IsKeyPressed( vkey );    
}

std::optional<std::wstring>
JsUtils::MapString( const std::wstring& str, uint32_t lcid, uint32_t flags )
{   // WinAPI is weird: 0 - error (with LastError), > 0 - characters required
    int iRet = ::LCMapStringW( lcid, flags, str.c_str(), str.length() + 1, nullptr, 0 );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, iRet, std::nullopt, LCMapStringW );

    std::unique_ptr<wchar_t[]> dst(new wchar_t[iRet]);
    iRet = ::LCMapStringW( lcid, flags, str.c_str(), str.length() + 1, dst.get(), iRet );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, iRet, std::nullopt, LCMapStringW );
    
    return dst.get();
}

std::optional<bool>
JsUtils::PathWildcardMatch( const std::wstring& pattern, const std::wstring& str )
{
    return PathMatchSpec( str.c_str(), pattern.c_str() );
}

std::optional<std::wstring>
JsUtils::ReadINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval )
{// TODO: inspect the code (replace with std::filesystem perhaps?)
    WCHAR buff[255] = { 0 };
    GetPrivateProfileString( section.c_str(), key.c_str(), nullptr, buff, sizeof( buff ), filename.c_str() );
    return !buff[0] ? defaultval : buff;
}

std::optional<std::wstring>
JsUtils::ReadTextFile( const std::wstring& filename, uint32_t codepage )
{// TODO: inspect the code (replace with std::filesystem perhaps?)
    pfc::array_t<wchar_t> content;

    if ( !helpers::read_file_wide( codepage, filename.c_str(), content ) )
    {
        return std::wstring();
    }

    return std::wstring( content.get_ptr(), content.get_size() );
}

std::optional<bool>
JsUtils::WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val )
{// TODO: inspect the code (replace with std::filesystem perhaps?)
    return WritePrivateProfileString( section.c_str(), key.c_str(), val.c_str(), filename.c_str() );
}

std::optional<bool>
JsUtils::WriteTextFile( const std::string& filename, const std::string& content, bool write_bom )
{// TODO: inspect the code (replace with std::filesystem perhaps?)
    if ( filename.empty() || content.empty() )
    {
        return false;
    }
    
    pfc::string8_fast content8( content.c_str(), content.length() );
    return helpers::write_file( filename.c_str(), content8, write_bom );
}

std::optional<uint32_t>
JsUtils::get_Version()
{
    return JSP_VERSION_MAJOR*100 + JSP_VERSION_MINOR*10 + JSP_VERSION_PATCH;
}

}
