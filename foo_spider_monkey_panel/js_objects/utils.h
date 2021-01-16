#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle;

class JsUtils
    : public JsObjectBase<JsUtils>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    static std::unique_ptr<JsUtils> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    bool CheckComponent( const std::u8string& name, bool is_dll = true );
    bool CheckComponentWithOpt( size_t optArgCount, const std::u8string& name, bool is_dll );
    bool CheckFont( const std::wstring& name );
    uint32_t ColourPicker( uint32_t hWnd, uint32_t default_colour );
    void EditTextFile( const std::wstring& path );
    JS::Value FileTest( const std::wstring& path, const std::wstring& mode );
    std::u8string FormatDuration( double p );
    std::u8string FormatFileSize( uint64_t p );
    void GetAlbumArtAsync( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true, bool only_embed = false, bool no_load = false );
    void GetAlbumArtAsyncWithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );
    JSObject* GetAlbumArtAsyncV2( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true, bool only_embed = false, bool no_load = false );
    JSObject* GetAlbumArtAsyncV2WithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );
    JSObject* GetAlbumArtEmbedded( const std::u8string& rawpath, uint32_t art_id = 0 );
    JSObject* GetAlbumArtEmbeddedWithOpt( size_t optArgCount, const std::u8string& rawpath, uint32_t art_id );
    JSObject* GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true );
    JSObject* GetAlbumArtV2WithOpt( size_t optArgCount, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub );
    std::u8string GetPackagePath( const std::u8string& packageId );
    uint32_t GetSysColour( uint32_t index );
    uint32_t GetSystemMetrics( uint32_t index );
    JSObject* Glob( const std::u8string& pattern, uint32_t exc_mask = FILE_ATTRIBUTE_DIRECTORY, uint32_t inc_mask = 0xFFFFFFFF );
    JSObject* GlobWithOpt( size_t optArgCount, const std::u8string& pattern, uint32_t exc_mask, uint32_t inc_mask );
    std::u8string InputBox( uint32_t hWnd, const std::u8string& prompt, const std::u8string& caption, const std::u8string& def = "", bool error_on_cancel = false );
    std::u8string InputBoxWithOpt( size_t optArgCount, uint32_t hWnd, const std::u8string& prompt, const std::u8string& caption, const std::u8string& def, bool error_on_cancel );
    bool IsKeyPressed( uint32_t vkey );
    std::wstring MapString( const std::wstring& str, uint32_t lcid, uint32_t flags );
    bool PathWildcardMatch( const std::wstring& pattern, const std::wstring& str );
    std::wstring ReadINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval = L"" );
    std::wstring ReadINIWithOpt( size_t optArgCount, const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval );
    std::wstring ReadTextFile( const std::wstring& filePath, uint32_t codepage = CP_ACP );
    std::wstring ReadTextFileWithOpt( size_t optArgCount, const std::wstring& filePath, uint32_t codepage );
    JS::Value ShowHtmlDialog( uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options = JS::UndefinedHandleValue );
    JS::Value ShowHtmlDialogWithOpt( size_t optArgCount, uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options );
    bool WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val );
    void WriteTextFile( const std::wstring& filename, const std::u8string& content, bool write_bom = true );
    void WriteTextFileWithOpt( size_t optArgCount, const std::wstring& filename, const std::u8string& content, bool write_bom );

public:
    std::u8string get_Version();

private:
    JsUtils( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
