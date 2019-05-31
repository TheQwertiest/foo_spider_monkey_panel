#include <stdafx.h>
#include "utils.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_art_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/winapi_error_helpers.h>
#include <utils/art_helpers.h>
#include <utils/file_helpers.h>
#include <utils/scope_helpers.h>
#include <utils/colour_helpers.h>

#include <ui/ui_input_box.h>
#include <ui/ui_html.h>

// StringCchCopy, StringCchCopyN
#include <StrSafe.h>

#include <io.h>
#include <fcntl.h>

#include <filesystem>

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

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CheckComponent, JsUtils::CheckComponent, JsUtils::CheckComponentWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( CheckFont, JsUtils::CheckFont );
MJS_DEFINE_JS_FN_FROM_NATIVE( ColourPicker, JsUtils::ColourPicker );
MJS_DEFINE_JS_FN_FROM_NATIVE( FileTest, JsUtils::FileTest );
MJS_DEFINE_JS_FN_FROM_NATIVE( FormatDuration, JsUtils::FormatDuration );
MJS_DEFINE_JS_FN_FROM_NATIVE( FormatFileSize, JsUtils::FormatFileSize );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtAsync, JsUtils::GetAlbumArtAsync, JsUtils::GetAlbumArtAsyncWithOpt, 4 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtAsyncV2, JsUtils::GetAlbumArtAsyncV2, JsUtils::GetAlbumArtAsyncV2WithOpt, 4 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtEmbedded, JsUtils::GetAlbumArtEmbedded, JsUtils::GetAlbumArtEmbeddedWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtV2, JsUtils::GetAlbumArtV2, JsUtils::GetAlbumArtV2WithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSysColour, JsUtils::GetSysColour );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSystemMetrics, JsUtils::GetSystemMetrics );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Glob, JsUtils::Glob, JsUtils::GlobWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( InputBox, JsUtils::InputBox, JsUtils::InputBoxWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsKeyPressed, JsUtils::IsKeyPressed );
MJS_DEFINE_JS_FN_FROM_NATIVE( MapString, JsUtils::MapString );
MJS_DEFINE_JS_FN_FROM_NATIVE( PathWildcardMatch, JsUtils::PathWildcardMatch );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ReadINI, JsUtils::ReadINI, JsUtils::ReadINIWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ReadTextFile, JsUtils::ReadTextFile, JsUtils::ReadTextFileWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ShowHtmlDialog, JsUtils::ShowHtmlDialog, JsUtils::ShowHtmlDialogWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( WriteINI, JsUtils::WriteINI );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( WriteTextFile, JsUtils::WriteTextFile, JsUtils::WriteTextFileWithOpt, 1 );

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "CheckComponent", CheckComponent, 1, DefaultPropsFlags() ),
    JS_FN( "CheckFont", CheckFont, 1, DefaultPropsFlags() ),
    JS_FN( "ColourPicker", ColourPicker, 2, DefaultPropsFlags() ),
    JS_FN( "FileTest", FileTest, 2, DefaultPropsFlags() ),
    JS_FN( "FormatDuration", FormatDuration, 1, DefaultPropsFlags() ),
    JS_FN( "FormatFileSize", FormatFileSize, 1, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtAsync", GetAlbumArtAsync, 2, DefaultPropsFlags() ),
    JS_FN( "GetAlbumArtAsyncV2", GetAlbumArtAsyncV2, 2, DefaultPropsFlags() ),
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
    JS_FN( "ShowHtmlDialog", ShowHtmlDialog, 3, DefaultPropsFlags() ),
    JS_FN( "WriteINI", WriteINI, 4, DefaultPropsFlags() ),
    JS_FN( "WriteTextFile", WriteTextFile, 2, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Version, JsUtils::get_Version )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Version", get_Version, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsUtils::JsClass = jsClass;
const JSFunctionSpec* JsUtils::JsFunctions = jsFunctions;
const JSPropertySpec* JsUtils::JsProperties = jsProperties;

JsUtils::JsUtils( JSContext* cx )
    : pJsCtx_( cx )
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

bool JsUtils::CheckComponent( const pfc::string8_fast& name, bool is_dll )
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

bool JsUtils::CheckComponentWithOpt( size_t optArgCount, const pfc::string8_fast& name, bool is_dll )
{
    switch ( optArgCount )
    {
    case 0:
        return CheckComponent( name, is_dll );
    case 1:
        return CheckComponent( name );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

bool JsUtils::CheckFont( const std::wstring& name )
{
    Gdiplus::InstalledFontCollection font_collection;
    const int count = font_collection.GetFamilyCount();
    std::vector<Gdiplus::FontFamily> font_families( count );

    int recv;
    Gdiplus::Status gdiRet = font_collection.GetFamilies( count, font_families.data(), &recv );
    smp::error::CheckGdi( gdiRet, "GetFamilies" );
    SmpException::ExpectTrue( recv == count, "Internal error: GetFamilies numSought != numFound" );

    WCHAR family_name_eng[LF_FACESIZE] = { 0 };
    WCHAR family_name_loc[LF_FACESIZE] = { 0 };
    const auto it = ranges::find_if( font_families, [&family_name_eng, &family_name_loc, &name]( const auto& fontFamily ) {
        Gdiplus::Status gdiRet = fontFamily.GetFamilyName( family_name_eng, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ) );
        smp::error::CheckGdi( gdiRet, "GetFamilyName" );

        gdiRet = fontFamily.GetFamilyName( family_name_loc );
        smp::error::CheckGdi( gdiRet, "GetFamilyName" );

        return ( !_wcsicmp( name.c_str(), family_name_loc )
                 || !_wcsicmp( name.c_str(), family_name_eng ) );
    } );

    return ( it != font_families.cend() );
}

uint32_t JsUtils::ColourPicker( uint32_t hWindow, uint32_t default_colour )
{
    COLORREF colour{};
    COLORREF dummy[16] = { 0 };
    // Such cast will work only on x86
    if ( !uChooseColor( &colour, reinterpret_cast<HWND>( hWindow ), dummy ) )
    {
        colour = smp::colour::convert_argb_to_colorref( default_colour );
    }

    return smp::colour::convert_colorref_to_argb( colour );
}

JS::Value JsUtils::FileTest( const std::wstring& path, const std::wstring& mode )
{
    namespace fs = std::filesystem;
    const std::wstring cleanedPath = fs::path( path.c_str() ).lexically_normal().wstring();

    if ( L"e" == mode ) // exists
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setBoolean( PathFileExists( path.c_str() ) );
        return jsValue;
    }
    else if ( L"s" == mode )
    {
        WIN32_FILE_ATTRIBUTE_DATA fileData;
        if ( !GetFileAttributesEx( ( std::wstring{ L"\\\\?\\" } + cleanedPath ).c_str(), GetFileExInfoStandard, &fileData ) )
        {
            smp::error::CheckWinApi( false, "GetFileAttributesEx" );
        }

        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setNumber( static_cast<double>( static_cast<uint64_t>( fileData.nFileSizeHigh ) << 32 | fileData.nFileSizeLow ) );
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
        const wchar_t* cPath = cleanedPath.c_str();
        const wchar_t* fn = PathFindFileName( cPath );
        const wchar_t* ext = PathFindExtension( fn );
        wchar_t dir[MAX_PATH] = { 0 };

        std::vector<std::wstring> out( 3 );
        if ( PathIsFileSpec( fn ) )
        {
            StringCchCopyN( dir, _countof( dir ), cPath, fn - cPath );
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

        JS::RootedValue jsValue( pJsCtx_ );
        convert::to_js::ToArrayValue(
            pJsCtx_,
            out,
            []( const auto& vec, auto index ) {
                return vec[index];
            },
            &jsValue );

        return jsValue;
    }
    else if ( L"chardet" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        jsValue.setNumber(
            static_cast<uint32_t>(
                smp::file::DetectFileCharset( pfc::stringcvt::string_utf8_from_wide( cleanedPath.c_str(), cleanedPath.length() ) ) ) );
        return jsValue;
    }

    throw SmpException( "Invalid value of mode argument" );
}

pfc::string8_fast JsUtils::FormatDuration( double p )
{
    return pfc::string8_fast( pfc::format_time_ex( p, 0 ) );
}

pfc::string8_fast JsUtils::FormatFileSize( uint64_t p )
{
    return pfc::string8_fast( pfc::format_file_size_short( p ) );
}

void JsUtils::GetAlbumArtAsync( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    SmpException::ExpectTrue( hWnd, "Invalid hWnd argument" );
    SmpException::ExpectTrue( handle, "handle argument is null" );

    // Such cast will work only on x86
    smp::art::GetAlbumArtAsync( reinterpret_cast<HWND>( hWnd ), handle->GetHandle(), art_id, need_stub, only_embed, no_load );
}

void JsUtils::GetAlbumArtAsyncWithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    switch ( optArgCount )
    {
    case 0:
        return GetAlbumArtAsync( hWnd, handle, art_id, need_stub, only_embed, no_load );
    case 1:
        return GetAlbumArtAsync( hWnd, handle, art_id, need_stub, only_embed );
    case 2:
        return GetAlbumArtAsync( hWnd, handle, art_id, need_stub );
    case 3:
        return GetAlbumArtAsync( hWnd, handle, art_id );
    case 4:
        return GetAlbumArtAsync( hWnd, handle );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

JSObject* JsUtils::GetAlbumArtAsyncV2( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    SmpException::ExpectTrue( hWnd, "Invalid hWnd argument" );
    SmpException::ExpectTrue( handle, "handle argument is null" );

    // Such cast will work only on x86
    return mozjs::art::GetAlbumArtPromise( pJsCtx_, reinterpret_cast<HWND>( hWnd ), handle->GetHandle(), art_id, need_stub, only_embed, no_load );
}

JSObject* JsUtils::GetAlbumArtAsyncV2WithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    switch ( optArgCount )
    {
    case 0:
        return GetAlbumArtAsyncV2( hWnd, handle, art_id, need_stub, only_embed, no_load );
    case 1:
        return GetAlbumArtAsyncV2( hWnd, handle, art_id, need_stub, only_embed );
    case 2:
        return GetAlbumArtAsyncV2( hWnd, handle, art_id, need_stub );
    case 3:
        return GetAlbumArtAsyncV2( hWnd, handle, art_id );
    case 4:
        return GetAlbumArtAsyncV2( hWnd, handle );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

JSObject* JsUtils::GetAlbumArtEmbedded( const pfc::string8_fast& rawpath, uint32_t art_id )
{
    std::unique_ptr<Gdiplus::Bitmap> artImage( smp::art::GetBitmapFromEmbeddedData( rawpath, art_id ) );
    if ( !artImage )
    { // Not an error: no art found
        return nullptr;
    }

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( artImage ) );
}

JSObject* JsUtils::GetAlbumArtEmbeddedWithOpt( size_t optArgCount, const pfc::string8_fast& rawpath, uint32_t art_id )
{
    switch ( optArgCount )
    {
    case 0:
        return GetAlbumArtEmbedded( rawpath, art_id );
    case 1:
        return GetAlbumArtEmbedded( rawpath );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

JSObject* JsUtils::GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{
    SmpException::ExpectTrue( handle, "handle argument is null" );

    std::unique_ptr<Gdiplus::Bitmap> artImage( smp::art::GetBitmapFromMetadb( handle->GetHandle(), art_id, need_stub, false, nullptr ) );
    if ( !artImage )
    { // Not an error: no art found
        return nullptr;
    }

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( artImage ) );
}

JSObject* JsUtils::GetAlbumArtV2WithOpt( size_t optArgCount, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{
    switch ( optArgCount )
    {
    case 0:
        return GetAlbumArtV2( handle, art_id, need_stub );
    case 1:
        return GetAlbumArtV2( handle, art_id );
    case 2:
        return GetAlbumArtV2( handle );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

uint32_t JsUtils::GetSysColour( uint32_t index )
{
    const auto hBrush = ::GetSysColorBrush( index ); ///< no need to call DeleteObject here
    SmpException::ExpectTrue( hBrush, "Invalid color index: {}", index );

    return smp::colour::convert_colorref_to_argb( ::GetSysColor( index ) );
}

uint32_t JsUtils::GetSystemMetrics( uint32_t index )
{
    return ::GetSystemMetrics( index );
}

JSObject* JsUtils::Glob( const pfc::string8_fast& pattern, uint32_t exc_mask, uint32_t inc_mask )
{
    std::vector<pfc::string8_fast> files;
    {
        std::unique_ptr<uFindFile> ff( uFindFirstFile( pattern.c_str() ) );
        if ( ff )
        {
            const pfc::string8_fast dir( pattern.c_str(), pfc::scan_filename( pattern.c_str() ) );
            do
            {
                const DWORD attr = ff->GetAttributes();
                if ( ( attr & inc_mask ) && !( attr & exc_mask ) )
                {
                    files.emplace_back( dir + ff->GetFileName() );
                }
            } while ( ff->FindNext() );
        }
    }

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue(
        pJsCtx_,
        files,
        []( auto& vec, auto idx ) {
            return vec[idx];
        },
        &jsValue );

    return &jsValue.toObject();
}

JSObject* JsUtils::GlobWithOpt( size_t optArgCount, const pfc::string8_fast& pattern, uint32_t exc_mask, uint32_t inc_mask )
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
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

pfc::string8_fast JsUtils::InputBox( uint32_t hWnd, const pfc::string8_fast& prompt, const pfc::string8_fast& caption, const pfc::string8_fast& def, bool error_on_cancel )
{
    if ( modal_dialog_scope::can_create() )
    {
        modal_dialog_scope scope( reinterpret_cast<HWND>( hWnd ) );

        CInputBox dlg( prompt, caption, def );
        int status = dlg.DoModal( reinterpret_cast<HWND>( hWnd ) );
        if ( status == IDCANCEL && error_on_cancel )
        {
            throw SmpException( "Dialog window was closed" );
        }

        if ( status == IDOK )
        {
            pfc::string8_fast val;
            dlg.GetValue( val );
            return val;
        }
    }

    return def;
}

pfc::string8_fast JsUtils::InputBoxWithOpt( size_t optArgCount, uint32_t hWnd, const pfc::string8_fast& prompt, const pfc::string8_fast& caption, const pfc::string8_fast& def, bool error_on_cancel )
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
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

bool JsUtils::IsKeyPressed( uint32_t vkey )
{
    return ::IsKeyPressed( vkey );
}

std::wstring JsUtils::MapString( const std::wstring& str, uint32_t lcid, uint32_t flags )
{
    // WinAPI is weird: 0 - error (with LastError), > 0 - characters required
    int iRet = LCIDToLocaleName( lcid, nullptr, 0, LOCALE_ALLOW_NEUTRAL_NAMES );
    smp::error::CheckWinApi( iRet, "LCIDToLocaleName(nullptr)" );

    std::wstring localeName( iRet, '\0' );
    iRet = LCIDToLocaleName( lcid, localeName.data(), localeName.size(), LOCALE_ALLOW_NEUTRAL_NAMES );
    smp::error::CheckWinApi( iRet, "LCIDToLocaleName(data)" );

    std::optional<NLSVERSIONINFOEX> versionInfo;
    try
    {
        if ( _WIN32_WINNT_WIN7 > GetWindowsVersionCode() )
        {
            NLSVERSIONINFOEX tmpVersionInfo{};
            BOOL bRet = GetNLSVersionEx( COMPARE_STRING, localeName.c_str(), &tmpVersionInfo );
            smp::error::CheckWinApi( bRet, "GetNLSVersionEx" );

            versionInfo = tmpVersionInfo;
        }
    }
    catch ( const std::exception& )
    {
    }

    NLSVERSIONINFO* pVersionInfo = reinterpret_cast<NLSVERSIONINFO*>( versionInfo ? &versionInfo.value() : nullptr );

    iRet = LCMapStringEx( localeName.c_str(), flags, str.c_str(), str.length() + 1, nullptr, 0, pVersionInfo, nullptr, 0 );
    smp::error::CheckWinApi( iRet, "LCMapStringEx(nullptr)" );

    std::wstring dst( iRet, '\0' );
    iRet = LCMapStringEx( localeName.c_str(), flags, str.c_str(), str.length() + 1, dst.data(), dst.size(), pVersionInfo, nullptr, 0 );
    smp::error::CheckWinApi( iRet, "LCMapStringEx(data)" );

    dst.resize( wcslen( dst.c_str() ) );
    return dst;
}

bool JsUtils::PathWildcardMatch( const std::wstring& pattern, const std::wstring& str )
{
    return PathMatchSpec( str.c_str(), pattern.c_str() );
}

std::wstring JsUtils::ReadINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval )
{
    // WinAPI is weird: 0 - error (with LastError), > 0 - characters required
    std::wstring dst( MAX_PATH, '\0' );
    int iRet = GetPrivateProfileString( section.c_str(), key.c_str(), defaultval.c_str(), dst.data(), dst.size(), filename.c_str() );
    // TODO: Uncomment error checking in v2.x
    // smp::error::CheckWinApi( ( iRet || ( NO_ERROR == GetLastError() ) ), "GetPrivateProfileString(nullptr)" );

    if ( !iRet && ( NO_ERROR != GetLastError() ) )
    {
        dst = defaultval;
    }
    else
    {
        dst.resize( wcslen( dst.c_str() ) );
    }

    return dst;
}

std::wstring JsUtils::ReadINIWithOpt( size_t optArgCount, const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval )
{
    switch ( optArgCount )
    {
    case 0:
        return ReadINI( filename, section, key, defaultval );
    case 1:
        return ReadINI( filename, section, key );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

std::wstring JsUtils::ReadTextFile( const pfc::string8_fast& filePath, uint32_t codepage )
{
    return smp::file::ReadFileW( filePath, codepage );
}

std::wstring JsUtils::ReadTextFileWithOpt( size_t optArgCount, const pfc::string8_fast& filePath, uint32_t codepage )
{
    switch ( optArgCount )
    {
    case 0:
        return ReadTextFile( filePath, codepage );
    case 1:
        return ReadTextFile( filePath );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

JS::Value JsUtils::ShowHtmlDialog( uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options )
{
    if ( modal_dialog_scope::can_create() )
    {
        modal_dialog_scope scope( reinterpret_cast<HWND>( hWnd ) );

        smp::ui::CDialogHtml dlg( pJsCtx_, htmlCode, options );
        int iRet = (int)dlg.DoModal( reinterpret_cast<HWND>( hWnd ) );
        if ( -1 == iRet || IDABORT == iRet )
        {
            if ( JS_IsExceptionPending( pJsCtx_ ) )
            {
                throw smp::JsException();
            }
            else
            {
                throw SmpException( fmt::format( "DoModal failed: {}", iRet ) );
            }
        }
    }

    // TODO: placeholder for modeless
    return JS::UndefinedValue();
}

JS::Value JsUtils::ShowHtmlDialogWithOpt( size_t optArgCount, uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return ShowHtmlDialog( hWnd, htmlCode, options );
    case 1:
        return ShowHtmlDialog( hWnd, htmlCode );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

bool JsUtils::WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val )
{
    return WritePrivateProfileString( section.c_str(), key.c_str(), val.c_str(), filename.c_str() );
}

bool JsUtils::WriteTextFile( const std::wstring& filename, const pfc::string8_fast& content, bool write_bom )
{ // TODO: inspect the code (replace with std::filesystem perhaps?)
    SmpException::ExpectTrue( !filename.empty(), "Invalid filename" );

    return smp::file::WriteFile( filename.c_str(), content, write_bom );
}

bool JsUtils::WriteTextFileWithOpt( size_t optArgCount, const std::wstring& filename, const pfc::string8_fast& content, bool write_bom )
{
    switch ( optArgCount )
    {
    case 0:
        return WriteTextFile( filename, content, write_bom );
    case 1:
        return WriteTextFile( filename, content );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

pfc::string8_fast JsUtils::get_Version()
{
    return SMP_VERSION;
}

} // namespace mozjs
