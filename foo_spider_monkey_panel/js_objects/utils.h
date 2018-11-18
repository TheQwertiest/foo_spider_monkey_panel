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
    ~JsUtils() override;

    static std::unique_ptr<JsUtils> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    bool CheckComponent( const pfc::string8_fast& name, bool is_dll = true );
    bool CheckComponentWithOpt( size_t optArgCount, const pfc::string8_fast& name, bool is_dll );
    bool CheckFont( const std::wstring& name );
    uint32_t ColourPicker( uint32_t hWindow, uint32_t default_colour );
    JS::Value FileTest( const std::wstring& path, const std::wstring& mode );
    pfc::string8_fast FormatDuration( double p );
    pfc::string8_fast FormatFileSize( uint64_t p );
    void GetAlbumArtAsync( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true, bool only_embed = false, bool no_load = false );
    void GetAlbumArtAsyncWithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );
    JSObject* GetAlbumArtAsyncV2( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true, bool only_embed = false, bool no_load = false );
    JSObject* GetAlbumArtAsyncV2WithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );
    JSObject* GetAlbumArtEmbedded( const pfc::string8_fast& rawpath, uint32_t art_id = 0 );
    JSObject* GetAlbumArtEmbeddedWithOpt( size_t optArgCount, const pfc::string8_fast& rawpath, uint32_t art_id );
    JSObject* GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true );
    JSObject* GetAlbumArtV2WithOpt( size_t optArgCount, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub );
    uint32_t GetSysColour( uint32_t index );
    uint32_t GetSystemMetrics( uint32_t index );
    JSObject* Glob( const pfc::string8_fast& pattern, uint32_t exc_mask = FILE_ATTRIBUTE_DIRECTORY, uint32_t inc_mask = 0xFFFFFFFF );
    JSObject* GlobWithOpt( size_t optArgCount, const pfc::string8_fast& pattern, uint32_t exc_mask, uint32_t inc_mask );
    pfc::string8_fast InputBox( uint32_t hWnd, const pfc::string8_fast& prompt, const pfc::string8_fast& caption, const pfc::string8_fast& def = "", bool error_on_cancel = false );
    pfc::string8_fast InputBoxWithOpt( size_t optArgCount, uint32_t hWnd, const pfc::string8_fast& prompt, const pfc::string8_fast& caption, const pfc::string8_fast& def, bool error_on_cancel );
    bool IsKeyPressed( uint32_t vkey );
    std::wstring MapString( const std::wstring& str, uint32_t lcid, uint32_t flags );
    bool PathWildcardMatch( const std::wstring& pattern, const std::wstring& str );
    std::wstring ReadINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval = L"" );
    std::wstring ReadINIWithOpt( size_t optArgCount, const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval );
    std::wstring ReadTextFile( const pfc::string8_fast& filePath, uint32_t codepage = 0 );
    std::wstring ReadTextFileWithOpt( size_t optArgCount, const pfc::string8_fast& filePath, uint32_t codepage );
    JS::Value ShowHtmlDialog( uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options = JS::UndefinedHandleValue );
    JS::Value ShowHtmlDialogWithOpt( size_t optArgCount, uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options );
    bool WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val );
    bool WriteTextFile( const pfc::string8_fast& filename, const pfc::string8_fast& content, bool write_bom = true );
    bool WriteTextFileWithOpt( size_t optArgCount, const pfc::string8_fast& filename, const pfc::string8_fast& content, bool write_bom );

public:
    pfc::string8_fast get_Version();

private:
    JsUtils( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
