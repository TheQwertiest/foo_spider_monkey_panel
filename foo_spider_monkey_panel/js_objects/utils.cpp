#include <stdafx.h>
#include "utils.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/art_helper.h>

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

MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, CheckComponent );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, CheckFont );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, ColourPicker );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, FileTest );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, FormatDuration );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, FormatFileSize );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetAlbumArtAsync );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetAlbumArtEmbedded );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetAlbumArtV2 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetSysColour );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetSystemMetrics );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, Glob );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, IsKeyPressed );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, MapString );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, PathWildcardMatch );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, ReadINI );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, ReadTextFile );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, WriteINI );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, WriteTextFile );

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "CheckComponent", CheckComponent, 0, DefaultPropsFlags() ),
    JS_FN( "CheckFont", CheckFont, 0, DefaultPropsFlags() ),
    JS_FN( "ColourPicker", ColourPicker, 0, DefaultPropsFlags() ),
    JS_FN( "FileTest", FileTest, 0, DefaultPropsFlags() ),
    JS_FN( "FormatDuration", FormatDuration, 0, DefaultPropsFlags() ),
    JS_FN( "FormatFileSize", FormatFileSize, 0, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtAsync", GetAlbumArtAsync, 0, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtEmbedded", GetAlbumArtEmbedded, 0, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtV2", GetAlbumArtV2, 0, DefaultPropsFlags() ),
    JS_FN( "GetSysColour", GetSysColour, 0, DefaultPropsFlags() ),
    JS_FN( "GetSystemMetrics", GetSystemMetrics, 0, DefaultPropsFlags() ),
    JS_FN( "Glob", Glob, 0, DefaultPropsFlags() ),
    JS_FN( "IsKeyPressed", IsKeyPressed, 0, DefaultPropsFlags() ),
    JS_FN( "MapString", MapString, 0, DefaultPropsFlags() ),
    JS_FN( "PathWildcardMatch", PathWildcardMatch, 0, DefaultPropsFlags() ),
    JS_FN( "ReadINI", ReadINI, 0, DefaultPropsFlags() ),
    JS_FN( "ReadTextFile", ReadTextFile, 0, DefaultPropsFlags() ),
    JS_FN( "WriteINI", WriteINI, 0, DefaultPropsFlags() ),
    JS_FN( "WriteTextFile", WriteTextFile, 0, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, get_Version )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Version", get_Version, DefaultPropsFlags() ),
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
    COLORREF color = helpers::convert_argb_to_colorref( default_colour );
    COLORREF colors[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uChooseColor( &color, (HWND)hWindow, &colors[0] );

    return helpers::convert_colorref_to_argb( color );
}

std::optional<JS::HandleValue>
JsUtils::FileTest( const std::wstring& path, const std::string& mode )
{
    if ( "e" == mode ) // exists
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setBoolean( PathFileExists( path.c_str() ) );
        return jsValue;
    }

    if ( "s" == mode )
    {
        HANDLE fh = CreateFile( path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
        IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, fh != INVALID_HANDLE_VALUE, std::nullopt, CreateFile );

        LARGE_INTEGER size = { 0 };
        GetFileSizeEx( fh, &size );
        CloseHandle( fh );

        JS::RootedValue jsValue( pJsCtx_ );
        // TODO: change this (should be uint64_t)
        jsValue.setNumber( static_cast<double>( size.QuadPart ) );
        return jsValue;
    }

    if ( "d" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setBoolean( PathIsDirectory( path.c_str() ) );
        return jsValue;
    }

    if ( "split" == mode )
    {
        const wchar_t* fn = PathFindFileName( path.c_str() );
        const wchar_t* ext = PathFindExtension( fn );
        wchar_t dir[MAX_PATH] = { 0 };

        std::wstring out[3];

        if ( PathIsFileSpec( fn ) )
        {
            StringCchCopyN( dir, _countof( dir ), path.c_str(), fn - path.c_str() );
            PathAddBackslash( dir );

            out[0].assign( dir );
            out[1].assign( fn, ext - fn );
            out[2].assign( ext );
        }
        else
        {
            StringCchCopy( dir, _countof( dir ), path.c_str() );
            PathAddBackslash( dir );

            out[0].assign( dir );
        }

        JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, _countof( out ) ) );
        if ( !jsArray )
        {
            JS_ReportOutOfMemory( pJsCtx_ );
            return std::nullopt;
        }

        JS::RootedValue jsValue( pJsCtx_ );
        JS::RootedObject jsObject( pJsCtx_ );
        for ( size_t i = 0; i < _countof( out ); ++i )
        {
            if ( !convert::to_js::ToValue( pJsCtx_, out[i], &jsValue ) )
            {
                JS_ReportErrorASCII( pJsCtx_, "Internal error: cast to JSString failed" );
                return std::nullopt;
            }

            jsValue.set( JS::ObjectValue( *jsObject ) );
            if ( !JS_SetElement( pJsCtx_, jsArray, i, jsValue ) )
            {
                JS_ReportErrorASCII( pJsCtx_, "Internal error: JS_SetElement failed" );
                return std::nullopt;
            }
        }

        jsValue.set( JS::ObjectValue( *jsArray ) );
        return jsValue;
    }

    if ( "chardet" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setNumber(
            static_cast<uint32_t>(
                helpers::detect_charset( pfc::stringcvt::string_utf8_from_wide( path.c_str(), path.length() ) )
                )
        );
        return jsValue;
    }

    JS_ReportErrorASCII( pJsCtx_, "Invalid value of mode argument" );
    return std::nullopt;
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

std::optional<std::uint32_t>
JsUtils::GetAlbumArtAsync( uint64_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    if ( !hWnd )
    {
        JS_ReportErrorASCII( pJsCtx_, "Invalid hWnd argument" );
        return std::nullopt;
    }

    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr ptr = handle->GetHandle();
    assert( ptr.is_valid() );

    return art::GetAlbumArtAsync( (HWND)hWnd, ptr, art_id, need_stub, only_embed, no_load );
}

std::optional<JSObject*>
JsUtils::GetAlbumArtEmbedded( const std::string& rawpath, uint32_t art_id )
{
    std::unique_ptr<Gdiplus::Bitmap> artImage( art::GetBitmapFromEmbeddedData( rawpath, art_id ) );
    if ( !artImage )
    {// Not an error: no art found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, artImage.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    artImage.release();
    return jsObject;
}

std::optional<JSObject*>
JsUtils::GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    std::unique_ptr<Gdiplus::Bitmap> artImage( art::GetBitmapFromMetadb( handle->GetHandle(), art_id, need_stub, false, nullptr ) );
    if ( !artImage )
    {// Not an error: no art found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, artImage.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    artImage.release();
    return jsObject;
}

std::optional<uint32_t>
JsUtils::GetSysColour( uint32_t index )
{
    if ( !::GetSysColorBrush( index ) )
    {// invalid index
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

std::optional<JSObject*>
JsUtils::Glob( const std::string& pattern, uint32_t exc_mask, uint32_t inc_mask )
{
    const char* fn = pattern.c_str() + pfc::scan_filename( pattern.c_str() );
    std::string dir( pattern.c_str(), fn - pattern.c_str() );
    std::unique_ptr<uFindFile> ff( uFindFirstFile( pattern.c_str() ) );

    pfc::string_list_impl files;

    if ( ff )
    {
        do
        {
            DWORD attr = ff->GetAttributes();

            if ( ( attr & inc_mask ) && !( attr & exc_mask ) )
            {
                std::string fullpath( dir );
                fullpath.append( ff->GetFileName() );
                files.add_item( fullpath.c_str() );
            }
        } while ( ff->FindNext() );
    }

    ff.release();

    JS::RootedObject evalResult( pJsCtx_, JS_NewArrayObject( pJsCtx_, files.get_count() ) );
    if ( !evalResult )
    {
        JS_ReportOutOfMemory( pJsCtx_ );
        return std::nullopt;
    }

    JS::RootedValue jsValue( pJsCtx_ );
    for ( t_size i = 0; i < files.get_count(); ++i )
    {
        std::string tmpString( files[i] );
        if ( !convert::to_js::ToValue( pJsCtx_, tmpString, &jsValue ) )
        {
            JS_ReportErrorASCII( pJsCtx_, "Internal error: cast to JSString failed" );
            return std::nullopt;
        }

        if ( !JS_SetElement( pJsCtx_, evalResult, i, jsValue ) )
        {
            JS_ReportErrorASCII( pJsCtx_, "Internal error: JS_SetElement failed" );
            return std::nullopt;
        }
    }

    return evalResult;
}

std::optional<bool>
JsUtils::IsKeyPressed( uint32_t vkey )
{
    return ::IsKeyPressed( vkey );
}

std::optional<std::wstring>
JsUtils::MapString( const std::wstring& str, uint32_t lcid, uint32_t flags )
{// TODO: LCMapString is deprecated, replace with a new V2 method (based on LCMapStringEx)
    // WinAPI is weird: 0 - error (with LastError), > 0 - characters required
    int iRet = ::LCMapStringW( lcid, flags, str.c_str(), str.length() + 1, nullptr, 0 );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, iRet, std::nullopt, LCMapStringW );

    std::unique_ptr<wchar_t[]> dst( new wchar_t[iRet] );
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
    return JSP_VERSION_MAJOR * 100 + JSP_VERSION_MINOR * 10 + JSP_VERSION_PATCH;
}

}
