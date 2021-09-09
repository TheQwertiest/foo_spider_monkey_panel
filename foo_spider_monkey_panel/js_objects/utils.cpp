#include <stdafx.h>

#include "utils.h"

#include <config/package_utils.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_art_helpers.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <ui/ui_html.h>
#include <ui/ui_input_box.h>
#include <utils/art_helpers.h>
#include <utils/colour_helpers.h>
#include <utils/edit_text.h>
#include <utils/gdi_error_helpers.h>

#include <qwr/file_helpers.h>
#include <qwr/winapi_error_helpers.h>

// StringCchCopy, StringCchCopyN
#include <StrSafe.h>
#include <fcntl.h>
#include <io.h>

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
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CheckComponent, JsUtils::CheckComponent, JsUtils::CheckComponentWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( CheckFont, JsUtils::CheckFont );
MJS_DEFINE_JS_FN_FROM_NATIVE( ColourPicker, JsUtils::ColourPicker );
MJS_DEFINE_JS_FN_FROM_NATIVE( DetectCharset, JsUtils::DetectCharset );
MJS_DEFINE_JS_FN_FROM_NATIVE( EditTextFile, JsUtils::EditTextFile );
MJS_DEFINE_JS_FN_FROM_NATIVE( FileExists, JsUtils::FileExists );
MJS_DEFINE_JS_FN_FROM_NATIVE( FileTest, JsUtils::FileTest );
MJS_DEFINE_JS_FN_FROM_NATIVE( FormatDuration, JsUtils::FormatDuration );
MJS_DEFINE_JS_FN_FROM_NATIVE( FormatFileSize, JsUtils::FormatFileSize );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtAsync, JsUtils::GetAlbumArtAsync, JsUtils::GetAlbumArtAsyncWithOpt, 4 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtAsyncV2, JsUtils::GetAlbumArtAsyncV2, JsUtils::GetAlbumArtAsyncV2WithOpt, 4 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtEmbedded, JsUtils::GetAlbumArtEmbedded, JsUtils::GetAlbumArtEmbeddedWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetAlbumArtV2, JsUtils::GetAlbumArtV2, JsUtils::GetAlbumArtV2WithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetFileSize, JsUtils::GetFileSize );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPackageInfo, JsUtils::GetPackageInfo );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPackagePath, JsUtils::GetPackagePath );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSysColour, JsUtils::GetSysColour );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSystemMetrics, JsUtils::GetSystemMetrics );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Glob, JsUtils::Glob, JsUtils::GlobWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( InputBox, JsUtils::InputBox, JsUtils::InputBoxWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsDirectory, JsUtils::IsDirectory );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsFile, JsUtils::IsFile );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsKeyPressed, JsUtils::IsKeyPressed );
MJS_DEFINE_JS_FN_FROM_NATIVE( MapString, JsUtils::MapString );
MJS_DEFINE_JS_FN_FROM_NATIVE( PathWildcardMatch, JsUtils::PathWildcardMatch );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ReadINI, JsUtils::ReadINI, JsUtils::ReadINIWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ReadTextFile, JsUtils::ReadTextFile, JsUtils::ReadTextFileWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ShowHtmlDialog, JsUtils::ShowHtmlDialog, JsUtils::ShowHtmlDialogWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( SplitFilePath, JsUtils::SplitFilePath );
MJS_DEFINE_JS_FN_FROM_NATIVE( WriteINI, JsUtils::WriteINI );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( WriteTextFile, JsUtils::WriteTextFile, JsUtils::WriteTextFileWithOpt, 1 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "CheckComponent", CheckComponent, 1, kDefaultPropsFlags ),
        JS_FN( "CheckFont", CheckFont, 1, kDefaultPropsFlags ),
        JS_FN( "ColourPicker", ColourPicker, 2, kDefaultPropsFlags ),
        JS_FN( "DetectCharset", DetectCharset, 1, kDefaultPropsFlags ),
        JS_FN( "EditTextFile", ::EditTextFile, 2, kDefaultPropsFlags ),
        JS_FN( "FileExists", FileExists, 1, kDefaultPropsFlags ),
        JS_FN( "FileTest", FileTest, 2, kDefaultPropsFlags ),
        JS_FN( "FormatDuration", FormatDuration, 1, kDefaultPropsFlags ),
        JS_FN( "FormatFileSize", FormatFileSize, 1, kDefaultPropsFlags ),
        JS_FN( "GetAlbumArtAsync", GetAlbumArtAsync, 2, kDefaultPropsFlags ),
        JS_FN( "GetAlbumArtAsyncV2", GetAlbumArtAsyncV2, 2, kDefaultPropsFlags ),
        JS_FN( "GetAlbumArtEmbedded", GetAlbumArtEmbedded, 1, kDefaultPropsFlags ),
        JS_FN( "GetAlbumArtV2", GetAlbumArtV2, 1, kDefaultPropsFlags ),
        JS_FN( "GetFileSize", GetFileSize, 1, kDefaultPropsFlags ),
        JS_FN( "GetPackageInfo", GetPackageInfo, 1, kDefaultPropsFlags ),
        JS_FN( "GetPackagePath", GetPackagePath, 1, kDefaultPropsFlags ),
        JS_FN( "GetSysColour", GetSysColour, 1, kDefaultPropsFlags ),
        JS_FN( "GetSystemMetrics", GetSystemMetrics, 1, kDefaultPropsFlags ),
        JS_FN( "Glob", Glob, 1, kDefaultPropsFlags ),
        JS_FN( "InputBox", InputBox, 3, kDefaultPropsFlags ),
        JS_FN( "IsDirectory", IsDirectory, 1, kDefaultPropsFlags ),
        JS_FN( "IsFile", IsFile, 1, kDefaultPropsFlags ),
        JS_FN( "IsKeyPressed", IsKeyPressed, 1, kDefaultPropsFlags ),
        JS_FN( "MapString", MapString, 3, kDefaultPropsFlags ),
        JS_FN( "PathWildcardMatch", PathWildcardMatch, 2, kDefaultPropsFlags ),
        JS_FN( "ReadINI", ReadINI, 3, kDefaultPropsFlags ),
        JS_FN( "ReadTextFile", ReadTextFile, 1, kDefaultPropsFlags ),
        JS_FN( "ShowHtmlDialog", ShowHtmlDialog, 3, kDefaultPropsFlags ),
        JS_FN( "SplitFilePath", SplitFilePath, 1, kDefaultPropsFlags ),
        JS_FN( "WriteINI", WriteINI, 4, kDefaultPropsFlags ),
        JS_FN( "WriteTextFile", WriteTextFile, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Version, JsUtils::get_Version )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Version", get_Version, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsUtils::JsClass = jsClass;
const JSFunctionSpec* JsUtils::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsUtils::JsProperties = jsProperties.data();

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

bool JsUtils::CheckComponent( const qwr::u8string& name, bool is_dll ) const
{
    pfc::string8_fast temp;
    for ( service_enum_t<componentversion> e; !e.finished(); ++e )
    {
        auto cv = e.get();
        if ( is_dll )
        {
            cv->get_file_name( temp );
        }
        else
        {
            cv->get_component_name( temp );
        }

        if ( temp.c_str() == name )
        {
            return true;
        }
    }

    return false;
}

bool JsUtils::CheckComponentWithOpt( size_t optArgCount, const qwr::u8string& name, bool is_dll ) const
{
    switch ( optArgCount )
    {
    case 0:
        return CheckComponent( name, is_dll );
    case 1:
        return CheckComponent( name );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool JsUtils::CheckFont( const std::wstring& name ) const
{
    Gdiplus::InstalledFontCollection font_collection;
    const int count = font_collection.GetFamilyCount();
    std::vector<Gdiplus::FontFamily> font_families( count );

    int recv;
    Gdiplus::Status gdiRet = font_collection.GetFamilies( count, font_families.data(), &recv );
    qwr::error::CheckGdi( gdiRet, "GetFamilies" );
    qwr::QwrException::ExpectTrue( recv == count, "Internal error: GetFamilies numSought != numFound" );

    std::array<wchar_t, LF_FACESIZE> family_name_eng{};
    std::array<wchar_t, LF_FACESIZE> family_name_loc{};
    const auto it = ranges::find_if( font_families, [&family_name_eng, &family_name_loc, &name]( const auto& fontFamily ) {
        Gdiplus::Status gdiRet = fontFamily.GetFamilyName( family_name_eng.data(), MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ) );
        qwr::error::CheckGdi( gdiRet, "GetFamilyName" );

        gdiRet = fontFamily.GetFamilyName( family_name_loc.data() );
        qwr::error::CheckGdi( gdiRet, "GetFamilyName" );

        return ( !_wcsicmp( name.c_str(), family_name_loc.data() )
                 || !_wcsicmp( name.c_str(), family_name_eng.data() ) );
    } );

    return ( it != font_families.cend() );
}

uint32_t JsUtils::ColourPicker( uint32_t hWnd, uint32_t default_colour )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    COLORREF colour{};
    std::array<COLORREF, 16> dummy{};
    if ( !uChooseColor( &colour, hPanel, dummy.data() ) )
    {
        colour = smp::colour::ArgbToColorref( default_colour );
    }

    return smp::colour::ColorrefToArgb( colour );
}

uint32_t JsUtils::DetectCharset( const std::wstring& path ) const
{
    namespace fs = std::filesystem;
    const auto cleanedPath = fs::path( path ).lexically_normal();

    return static_cast<uint32_t>( qwr::file::DetectFileCharset( cleanedPath ) );
}

void JsUtils::EditTextFile( const std::wstring& path )
{
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    if ( !modal_dialog_scope::can_create() )
    {
        return;
    }

    modal_dialog_scope scope( hPanel );

    // TODO: add options - editor_path, is_modal
    smp::EditTextFile( hPanel, std::filesystem::path{ path }, false, false );
}

bool JsUtils::FileExists( const std::wstring& path ) const
{
    namespace fs = std::filesystem;
    try
    {
        return fs::exists( path );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

JS::Value JsUtils::FileTest( const std::wstring& path, const std::wstring& mode )
{
    if ( L"e" == mode ) // exists
    {
        JS::RootedValue jsValue( pJsCtx_ );
        convert::to_js::ToValue( pJsCtx_, FileExists( path ), &jsValue );
        return jsValue;
    }
    else if ( L"s" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        convert::to_js::ToValue( pJsCtx_, GetFileSize( path ), &jsValue );
        return jsValue;
    }
    else if ( L"d" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        convert::to_js::ToValue( pJsCtx_, IsDirectory( path ), &jsValue );
        return jsValue;
    }
    else if ( L"split" == mode )
    {
        return SplitFilePath( path );
    }
    else if ( L"chardet" == mode )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        convert::to_js::ToValue( pJsCtx_, DetectCharset( path ), &jsValue );
        return jsValue;
    }
    else
    {
        throw qwr::QwrException( "Invalid value of mode argument: '{}'", qwr::unicode::ToU8( mode ) );
    }
}

qwr::u8string JsUtils::FormatDuration( double p ) const
{
    return qwr::u8string( pfc::format_time_ex( p, 0 ) );
}

qwr::u8string JsUtils::FormatFileSize( uint64_t p ) const
{
    return qwr::u8string( pfc::format_file_size_short( p ) );
}

void JsUtils::GetAlbumArtAsync( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    smp::art::GetAlbumArtAsync( hPanel, handle->GetHandle(), smp::art::LoadingOptions{ art_id, need_stub, only_embed, no_load } );
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* JsUtils::GetAlbumArtAsyncV2( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    return mozjs::art::GetAlbumArtPromise( pJsCtx_, hPanel, handle->GetHandle(), smp::art::LoadingOptions{ art_id, need_stub, only_embed, no_load } );
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* JsUtils::GetAlbumArtEmbedded( const qwr::u8string& rawpath, uint32_t art_id )
{
    std::unique_ptr<Gdiplus::Bitmap> artImage( smp::art::GetBitmapFromEmbeddedData( rawpath, art_id ) );
    if ( !artImage )
    { // Not an error: no art found
        return nullptr;
    }

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( artImage ) );
}

JSObject* JsUtils::GetAlbumArtEmbeddedWithOpt( size_t optArgCount, const qwr::u8string& rawpath, uint32_t art_id )
{
    switch ( optArgCount )
    {
    case 0:
        return GetAlbumArtEmbedded( rawpath, art_id );
    case 1:
        return GetAlbumArtEmbedded( rawpath );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* JsUtils::GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    std::unique_ptr<Gdiplus::Bitmap> artImage( smp::art::GetBitmapFromMetadb( handle->GetHandle(), smp::art::LoadingOptions{ art_id, need_stub, false, false }, nullptr ) );
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint64_t JsUtils::GetFileSize( const std::wstring& path ) const
{
    namespace fs = std::filesystem;
    try
    {
        qwr::QwrException::ExpectTrue( fs::exists( path ), L"Path does not point to a file: {}", path );
        return fs::file_size( path );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

JSObject* JsUtils::GetPackageInfo( const qwr::u8string& packageId ) const
{
    const auto packagePathOpt = config::FindPackage( packageId );
    if ( !packagePathOpt )
    {
        return nullptr;
    }

    const auto settings = config::GetPackageSettingsFromPath( *packagePathOpt );

    JS::RootedObject jsDirs( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    AddProperty( pJsCtx_, jsDirs, "Root", config::GetPackagePath( settings ).wstring() );
    AddProperty( pJsCtx_, jsDirs, "Assets", config::GetPackageAssetsDir( settings ).wstring() );
    AddProperty( pJsCtx_, jsDirs, "Scripts", config::GetPackageScriptsDir( settings ).wstring() );
    AddProperty( pJsCtx_, jsDirs, "Storage", config::GetPackageStorageDir( settings ).wstring() );

    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    AddProperty( pJsCtx_, jsObject, "Directories", static_cast<JS::HandleObject>( jsDirs ) );
    AddProperty( pJsCtx_, jsObject, "Version", settings.scriptVersion );

    return jsObject;
}

qwr::u8string JsUtils::GetPackagePath( const qwr::u8string& packageId ) const
{
    const auto packagePathOpt = config::FindPackage( packageId );
    qwr::QwrException::ExpectTrue( packagePathOpt.has_value(), "Unknown package: {}", packageId );

    return packagePathOpt->u8string();
}

uint32_t JsUtils::GetSysColour( uint32_t index ) const
{
    const auto hBrush = ::GetSysColorBrush( index ); ///< no need to call DeleteObject here
    qwr::QwrException::ExpectTrue( hBrush, "Invalid color index: {}", index );

    return smp::colour::ColorrefToArgb( ::GetSysColor( index ) );
}

uint32_t JsUtils::GetSystemMetrics( uint32_t index ) const
{
    return ::GetSystemMetrics( index );
}

JS::Value JsUtils::Glob( const qwr::u8string& pattern, uint32_t exc_mask, uint32_t inc_mask )
{
    std::vector<qwr::u8string> files;
    {
        const auto wPattern = qwr::unicode::ToWide( pattern );

        std::unique_ptr<uFindFile> ff( uFindFirstFile( pattern.c_str() ) );
        if ( ff )
        {
            const qwr::u8string dir( pattern.c_str(), pfc::scan_filename( pattern.c_str() ) );
            do
            {
                const DWORD attr = ff->GetAttributes();
                if ( ( attr & inc_mask ) && !( attr & exc_mask ) )
                {
                    const auto wPath = qwr::unicode::ToWide( dir + ff->GetFileName() );
                    if ( !PathMatchSpec( wPath.c_str(), wPattern.c_str() ) )
                    {
                        // workaround for FindFirstFile() bug:
                        // https://stackoverflow.com/a/44933735
                        continue;
                    }

                    files.emplace_back( dir + ff->GetFileName() );
                }
            } while ( ff->FindNext() );
        }
    }

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, files, &jsValue );
    return jsValue;
}

JS::Value JsUtils::GlobWithOpt( size_t optArgCount, const qwr::u8string& pattern, uint32_t exc_mask, uint32_t inc_mask )
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

qwr::u8string JsUtils::InputBox( uint32_t hWnd, const qwr::u8string& prompt, const qwr::u8string& caption, const qwr::u8string& def, bool error_on_cancel )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    if ( modal_dialog_scope::can_create() )
    {
        modal_dialog_scope scope( hPanel );

        smp::ui::CInputBox dlg( prompt.c_str(), caption.c_str(), def.c_str() );
        int status = dlg.DoModal( hPanel );
        if ( status == IDCANCEL && error_on_cancel )
        {
            throw qwr::QwrException( "Dialog window was closed" );
        }

        if ( status == IDOK )
        {
            return dlg.GetValue();
        }
    }

    return def;
}

qwr::u8string JsUtils::InputBoxWithOpt( size_t optArgCount, uint32_t hWnd, const qwr::u8string& prompt, const qwr::u8string& caption, const qwr::u8string& def, bool error_on_cancel )
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool JsUtils::IsDirectory( const std::wstring& path ) const
{
    namespace fs = std::filesystem;
    try
    {
        return fs::is_directory( path );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

bool JsUtils::IsFile( const std::wstring& path ) const
{
    namespace fs = std::filesystem;
    try
    {
        return fs::is_regular_file( path );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

bool JsUtils::IsKeyPressed( uint32_t vkey ) const
{
    return ::IsKeyPressed( vkey );
}

std::wstring JsUtils::MapString( const std::wstring& str, uint32_t lcid, uint32_t flags )
{
    // WinAPI is weird: 0 - error (with LastError), > 0 - characters required
    int iRet = LCIDToLocaleName( lcid, nullptr, 0, LOCALE_ALLOW_NEUTRAL_NAMES );
    qwr::error::CheckWinApi( iRet, "LCIDToLocaleName(nullptr)" );

    std::wstring localeName( iRet, '\0' );
    iRet = LCIDToLocaleName( lcid, localeName.data(), localeName.size(), LOCALE_ALLOW_NEUTRAL_NAMES );
    qwr::error::CheckWinApi( iRet, "LCIDToLocaleName(data)" );

    std::optional<NLSVERSIONINFOEX> versionInfo;
    try
    {
        if ( _WIN32_WINNT_WIN7 > GetWindowsVersionCode() )
        {
            NLSVERSIONINFOEX tmpVersionInfo{};
            BOOL bRet = GetNLSVersionEx( COMPARE_STRING, localeName.c_str(), &tmpVersionInfo );
            qwr::error::CheckWinApi( bRet, "GetNLSVersionEx" );

            versionInfo = tmpVersionInfo;
        }
    }
    catch ( const std::exception& )
    {
    }

    auto* pVersionInfo = reinterpret_cast<NLSVERSIONINFO*>( versionInfo ? &( *versionInfo ) : nullptr );

    iRet = LCMapStringEx( localeName.c_str(), flags, str.c_str(), str.length() + 1, nullptr, 0, pVersionInfo, nullptr, 0 );
    qwr::error::CheckWinApi( iRet, "LCMapStringEx(nullptr)" );

    std::wstring dst( iRet, '\0' );
    iRet = LCMapStringEx( localeName.c_str(), flags, str.c_str(), str.length() + 1, dst.data(), dst.size(), pVersionInfo, nullptr, 0 );
    qwr::error::CheckWinApi( iRet, "LCMapStringEx(data)" );

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
    // TODO v2: Uncomment error checking
    // qwr::error::CheckWinApi( ( iRet || ( NO_ERROR == GetLastError() ) ), "GetPrivateProfileString(nullptr)" );

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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

std::wstring JsUtils::ReadTextFile( const std::wstring& filePath, uint32_t codepage )
{
    return qwr::file::ReadFileW( filePath, codepage );
}

std::wstring JsUtils::ReadTextFileWithOpt( size_t optArgCount, const std::wstring& filePath, uint32_t codepage )
{
    switch ( optArgCount )
    {
    case 0:
        return ReadTextFile( filePath, codepage );
    case 1:
        return ReadTextFile( filePath );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsUtils::ShowHtmlDialog( uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    if ( modal_dialog_scope::can_create() )
    {
        modal_dialog_scope scope( hPanel );

        smp::ui::CDialogHtml dlg( pJsCtx_, htmlCode, options );
        int iRet = dlg.DoModal( hPanel );
        if ( -1 == iRet || IDABORT == iRet )
        {
            if ( JS_IsExceptionPending( pJsCtx_ ) )
            {
                throw JsException();
            }
            else
            {
                throw qwr::QwrException( "DoModal failed: {}", iRet );
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsUtils::SplitFilePath( const std::wstring& path )
{
    const auto cleanedPath = std::filesystem::path( path ).lexically_normal();

    std::vector<std::wstring> out( 3 );
    if ( PathIsFileSpec( cleanedPath.filename().c_str() ) )
    {
        out[0] = cleanedPath.parent_path() / "";
        out[1] = cleanedPath.stem();
        out[2] = cleanedPath.extension();
    }
    else
    {
        out[0] = cleanedPath / "";
    }

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, out, &jsValue );

    return jsValue;
}

bool JsUtils::WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val )
{
    return WritePrivateProfileString( section.c_str(), key.c_str(), val.c_str(), filename.c_str() );
}

bool JsUtils::WriteTextFile( const std::wstring& filename, const qwr::u8string& content, bool write_bom )
{
    qwr::QwrException::ExpectTrue( !filename.empty(), "Invalid filename" );

    try
    {
        qwr::file::WriteFile( filename, content, write_bom );
        return true;
    }
    catch ( const qwr::QwrException& )
    {
        return false;
    }
}

bool JsUtils::WriteTextFileWithOpt( size_t optArgCount, const std::wstring& filename, const qwr::u8string& content, bool write_bom )
{
    switch ( optArgCount )
    {
    case 0:
        return WriteTextFile( filename, content, write_bom );
    case 1:
        return WriteTextFile( filename, content );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

qwr::u8string JsUtils::get_Version() const
{
    return SMP_VERSION;
}

} // namespace mozjs
