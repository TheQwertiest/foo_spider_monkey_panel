#include <stdafx.h>
#include "utils.h"

#include <ui/ui_input_box.h>

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/html_window.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/art_helper.h>
#include <js_utils/file_helpers.h>
#include <js_utils/scope_helper.h>

#include <helpers.h>

// StringCchCopy, StringCchCopyN
#include <StrSafe.h>

#include <io.h>
#include <fcntl.h>

#include <filesystem>

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
    JsUtils::FinalizeJsObject,
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

MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, CheckComponent, CheckComponentWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, CheckFont );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, ColourPicker );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, CreateHtmlWindow, CreateHtmlWindowWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, FileTest );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, FormatDuration );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, FormatFileSize );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, GetAlbumArtAsync, GetAlbumArtAsyncWithOpt, 4 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, GetAlbumArtEmbedded, GetAlbumArtEmbeddedWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, GetAlbumArtV2, GetAlbumArtV2WithOpt, 2 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetSysColour );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, GetSystemMetrics );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, Glob, GlobWithOpt, 2 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, InputBox, InputBoxWithOpt, 2 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, IsKeyPressed );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, MapString );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, PathWildcardMatch );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, ReadINI, ReadINIWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, ReadTextFile, ReadTextFileWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsUtils, WriteINI );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsUtils, WriteTextFile, WriteTextFileWithOpt, 1 );

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "CheckComponent", CheckComponent, 1, DefaultPropsFlags() ),
    JS_FN( "CheckFont", CheckFont, 1, DefaultPropsFlags() ),
    JS_FN( "ColourPicker", ColourPicker, 2, DefaultPropsFlags() ),
    JS_FN( "CreateHtmlWindow", CreateHtmlWindow, 1, DefaultPropsFlags() ),
    JS_FN( "FileTest", FileTest, 2, DefaultPropsFlags() ),
    JS_FN( "FormatDuration", FormatDuration, 1, DefaultPropsFlags() ),
    JS_FN( "FormatFileSize", FormatFileSize, 1, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtAsync", GetAlbumArtAsync, 2, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtEmbedded", GetAlbumArtEmbedded, 1, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtV2", GetAlbumArtV2, 1, DefaultPropsFlags() ),
    JS_FN( "GetSysColour", GetSysColour, 1, DefaultPropsFlags() ),
    JS_FN( "GetSystemMetrics", GetSystemMetrics, 1, DefaultPropsFlags() ),
    JS_FN( "Glob", Glob, 1, DefaultPropsFlags() ),
    JS_FN( "InputBox", InputBox, 3, DefaultPropsFlags() ),
    JS_FN( "IsKeyPressed", IsKeyPressed, 1, DefaultPropsFlags() ),
    JS_FN( "MapString", MapString, 3, DefaultPropsFlags() ),
    JS_FN( "PathWildcardMatch", PathWildcardMatch, 2, DefaultPropsFlags() ),
    JS_FN( "ReadINI", ReadINI, 3, DefaultPropsFlags() ),
    JS_FN( "ReadTextFile", ReadTextFile, 1, DefaultPropsFlags() ),
    JS_FN( "WriteINI", WriteINI, 4, DefaultPropsFlags() ),
    JS_FN( "WriteTextFile", WriteTextFile, 2, DefaultPropsFlags() ),
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

const JSClass JsUtils::JsClass = jsClass;
const JSFunctionSpec* JsUtils::JsFunctions = jsFunctions;
const JSPropertySpec* JsUtils::JsProperties = jsProperties;

JsUtils::JsUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsUtils::~JsUtils()
{
}

std::unique_ptr<JsUtils>
JsUtils::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsUtils>( new JsUtils( cx ) );
}

size_t JsUtils::GetInternalSize()
{
    return 0;
}

std::optional<bool>
JsUtils::CheckComponent( const pfc::string8_fast& name, bool is_dll )
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

        if ( temp == name )
        {
            return true;
        }
    }

    return false;
}

std::optional<bool> 
JsUtils::CheckComponentWithOpt( size_t optArgCount, const pfc::string8_fast& name, bool is_dll )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return CheckComponent( name );
    }

    return CheckComponent( name, is_dll );
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
JsUtils::ColourPicker( uint32_t hWindow, uint32_t default_colour )
{
    COLORREF color = helpers::convert_argb_to_colorref( default_colour );
    COLORREF colors[16] = { 0 };
    // Such cast will work only on x86
    uChooseColor( &color, (HWND)hWindow, &colors[0] );

    return helpers::convert_colorref_to_argb( color );
}

std::optional<JSObject*> 
JsUtils::CreateHtmlWindow( const std::wstring& htmlCode, JS::HandleValue options )
{
    JS::RootedObject jsObject( pJsCtx_, JsHtmlWindow::CreateJs( pJsCtx_, htmlCode, options ) );
    if ( !jsObject )
    {// reports
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsUtils::CreateHtmlWindowWithOpt( size_t optArgCount, const std::wstring& htmlCode, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 1:
        return CreateHtmlWindow( htmlCode );
    case 0:
        return CreateHtmlWindow( htmlCode, options );
    default:
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }
}

std::optional<JS::Value>
JsUtils::FileTest( const std::wstring& path, const std::wstring& mode )
{
    const std::wstring cleanedPath = file::CleanPath( path );

    if ( L"e" == mode ) // exists
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setBoolean( PathFileExists( path.c_str() ) );
        return jsValue;
    }
    else if ( L"s" == mode )
    {
        HANDLE fh = CreateFile( cleanedPath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
        IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, fh != INVALID_HANDLE_VALUE, std::nullopt, CreateFile );

        LARGE_INTEGER size = { 0 };
        GetFileSizeEx( fh, &size );
        CloseHandle( fh );

        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setNumber( static_cast<double>(size.QuadPart) );
        return jsValue;
    }
    else if ( L"d" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setBoolean( PathIsDirectory( cleanedPath.c_str() ) );
        return jsValue;
    }
    else if ( L"split" == mode )
    {
        const wchar_t* fn = PathFindFileName( cleanedPath.c_str() );
        const wchar_t* ext = PathFindExtension( fn );
        wchar_t dir[MAX_PATH] = { 0 };

        std::wstring out[3];

        if ( PathIsFileSpec( fn ) )
        {
            StringCchCopyN( dir, _countof( dir ), cleanedPath.c_str(), fn - cleanedPath.c_str() );
            PathAddBackslash( dir );

            out[0].assign( dir );
            out[1].assign( fn, ext - fn );
            out[2].assign( ext );
        }
        else
        {
            StringCchCopy( dir, _countof( dir ), cleanedPath.c_str() );
            PathAddBackslash( dir );

            out[0].assign( dir );
        }

        JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, _countof( out ) ) );
        if ( !jsArray )
        {// reports
            return std::nullopt;
        }

        JS::RootedValue jsValue( pJsCtx_ );        
        for ( size_t i = 0; i < _countof( out ); ++i )
        {
            if ( !convert::to_js::ToValue( pJsCtx_, out[i], &jsValue ) )
            {
                JS_ReportErrorUTF8( pJsCtx_, "Internal error: cast to JSString failed" );
                return std::nullopt;
            }

            if ( !JS_SetElement( pJsCtx_, jsArray, i, jsValue ) )
            {// report in JS_SetElement
                return std::nullopt;
            }
        }        

        return JS::ObjectValue( *jsArray );
    }
    else if ( L"chardet" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setNumber(
            static_cast<uint32_t>(
                helpers::detect_charset( pfc::stringcvt::string_utf8_from_wide( cleanedPath.c_str(), cleanedPath.length() ) )
                )
        );
        return jsValue;
    }

    JS_ReportErrorUTF8( pJsCtx_, "Invalid value of mode argument" );
    return std::nullopt;
}

std::optional<pfc::string8_fast>
JsUtils::FormatDuration( double p )
{
    pfc::string8_fast str( pfc::format_time_ex( p, 0 ) );
    return pfc::string8_fast( str.c_str(), str.length() );
}

std::optional<pfc::string8_fast>
JsUtils::FormatFileSize( uint64_t p )
{
    pfc::string8_fast str = pfc::format_file_size_short( p );
    return pfc::string8_fast( str.c_str(), str.length() );
}

std::optional<std::uint32_t>
JsUtils::GetAlbumArtAsync( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    if ( !hWnd )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Invalid hWnd argument" );
        return std::nullopt;
    }

    if ( !handle )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr ptr = handle->GetHandle();
    assert( ptr.is_valid() );

    // Such cast will work only on x86
    return art::GetAlbumArtAsync( (HWND)hWnd, ptr, art_id, need_stub, only_embed, no_load );
}

std::optional<std::uint32_t> 
JsUtils::GetAlbumArtAsyncWithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    if ( optArgCount > 4 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 4 )
    {
        return GetAlbumArtAsync( hWnd, handle );
    }
    else if ( optArgCount == 3 )
    {
        return GetAlbumArtAsync( hWnd, handle, art_id );
    }
    else if ( optArgCount == 2 )
    {
        return GetAlbumArtAsync( hWnd, handle, art_id, need_stub );
    }
    else if ( optArgCount == 1 )
    {
        return GetAlbumArtAsync( hWnd, handle, art_id, need_stub, only_embed );
    }

    return GetAlbumArtAsync( hWnd, handle, art_id, need_stub, only_embed, no_load );
}

std::optional<JSObject*>
JsUtils::GetAlbumArtEmbedded( const pfc::string8_fast& rawpath, uint32_t art_id )
{
    std::unique_ptr<Gdiplus::Bitmap> artImage( art::GetBitmapFromEmbeddedData( rawpath, art_id ) );
    if ( !artImage )
    {// Not an error: no art found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::CreateJs( pJsCtx_, std::move( artImage ) ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*> 
JsUtils::GetAlbumArtEmbeddedWithOpt( size_t optArgCount, const pfc::string8_fast& rawpath, uint32_t art_id )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return GetAlbumArtEmbedded( rawpath );
    }

    return GetAlbumArtEmbedded( rawpath, art_id );
}

std::optional<JSObject*>
JsUtils::GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{
    if ( !handle )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    std::unique_ptr<Gdiplus::Bitmap> artImage( art::GetBitmapFromMetadb( handle->GetHandle(), art_id, need_stub, false, nullptr ) );
    if ( !artImage )
    {// Not an error: no art found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::CreateJs( pJsCtx_, std::move(artImage) ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsUtils::GetAlbumArtV2WithOpt( size_t optArgCount, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{
    if ( optArgCount > 2 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 2 )
    {
        return GetAlbumArtV2( handle );
    }
    else if ( optArgCount == 1 )
    {
        return GetAlbumArtV2( handle, art_id );
    }

    return GetAlbumArtV2( handle, art_id, need_stub );
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
JsUtils::Glob( const pfc::string8_fast& pattern, uint32_t exc_mask, uint32_t inc_mask )
{
    pfc::string_list_impl files;
    {       
        std::unique_ptr<uFindFile> ff( uFindFirstFile( pattern.c_str() ) );
        if ( ff )
        {
            const char* fn = pattern.c_str() + pfc::scan_filename( pattern.c_str() );
            const pfc::string8_fast dir( pattern.c_str(), fn - pattern.c_str() );
            do
            {
                DWORD attr = ff->GetAttributes();
                if ( (attr & inc_mask) && !(attr & exc_mask) )
                {
                    const pfc::string8_fast fullPath = dir + ff->GetFileName();
                    files.add_item( fullPath.c_str() );
                }
            } while ( ff->FindNext() );
        }
    }    

    JS::RootedObject evalResult( pJsCtx_, JS_NewArrayObject( pJsCtx_, files.get_count() ) );
    if ( !evalResult )
    {// reports
        return std::nullopt;
    }

    JS::RootedValue jsValue( pJsCtx_ );
    for ( t_size i = 0; i < files.get_count(); ++i )
    {
        pfc::string8_fast tmpString( files[i] );
        if ( !convert::to_js::ToValue( pJsCtx_, tmpString, &jsValue ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Internal error: cast to JSString failed" );
            return std::nullopt;
        }

        if ( !JS_SetElement( pJsCtx_, evalResult, i, jsValue ) )
        {// report in JS_SetElement
            return std::nullopt;
        }
    }

    return evalResult;
}

std::optional<JSObject*>
JsUtils::GlobWithOpt( size_t optArgCount, const pfc::string8_fast& pattern, uint32_t exc_mask, uint32_t inc_mask )
{
    switch ( optArgCount )
    {
    case 0:
        return Glob( pattern, exc_mask, inc_mask );
    case 1:
        return Glob( pattern, exc_mask );
    case 2:
        return Glob( pattern );
    default:
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }
}

std::optional<pfc::string8_fast> 
JsUtils::InputBox( uint32_t hWnd, const pfc::string8_fast& prompt, const pfc::string8_fast& caption, const pfc::string8_fast& def, bool error_on_cancel )
{
    modal_dialog_scope scope;
    if ( scope.can_create() )
    {
        scope.initialize( HWND( hWnd ) );
        CInputBox dlg( prompt, caption, def );
        int status = dlg.DoModal( HWND( hWnd ) );
        if ( status == IDCANCEL && error_on_cancel )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Dialog window was closed" );
            return std::nullopt;
        }

        if ( status == IDOK )
        {
            pfc::string8_fast val;
            dlg.GetValue( val );
            return val;
        }        
    }

    return pfc::string8_fast();
}

std::optional<pfc::string8_fast>
JsUtils::InputBoxWithOpt( size_t optArgCount, uint32_t hWnd, const pfc::string8_fast& prompt, const pfc::string8_fast& caption, const pfc::string8_fast& def, bool error_on_cancel )
{
    switch ( optArgCount )
    {
    case 0:
        return InputBox( hWnd, prompt, caption, def, error_on_cancel );
    case 1:
        return InputBox( hWnd, prompt, caption, def );
    case 2:
        return InputBox( hWnd, prompt, caption );
    default:
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }   
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
JsUtils::ReadINIWithOpt( size_t optArgCount, const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return ReadINI( filename, section, key );
    }

    return ReadINI( filename, section, key, defaultval );
}

std::optional<std::wstring>
JsUtils::ReadTextFile( const pfc::string8_fast& filePath, uint32_t codepage )
{
    auto retVal = file::ReadFromFile( pJsCtx_, filePath );
    if ( !retVal )
    {// report in ReadFromFile;
        return std::nullopt;
    }

    return retVal.value();
}

std::optional<std::wstring> 
JsUtils::ReadTextFileWithOpt( size_t optArgCount, const pfc::string8_fast& filePath, uint32_t codepage )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return ReadTextFile( filePath );
    }

    return ReadTextFile( filePath, codepage );
}

std::optional<bool>
JsUtils::WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val )
{// TODO: inspect the code (replace with std::filesystem perhaps?)
    return WritePrivateProfileString( section.c_str(), key.c_str(), val.c_str(), filename.c_str() );
}

std::optional<bool>
JsUtils::WriteTextFile( const pfc::string8_fast& filename, const pfc::string8_fast& content, bool write_bom )
{// TODO: inspect the code (replace with std::filesystem perhaps?)
    if ( filename.is_empty() )
    {
        return false;
    }
    
    return helpers::write_file( filename.c_str(), content, write_bom );
}

std::optional<bool> 
JsUtils::WriteTextFileWithOpt( size_t optArgCount, const pfc::string8_fast& filename, const pfc::string8_fast& content, bool write_bom )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return WriteTextFile( filename, content );
    }

    return WriteTextFile( filename, content, write_bom );
}

std::optional<pfc::string8_fast>
JsUtils::get_Version()
{
    return SMP_VERSION;
}

}
