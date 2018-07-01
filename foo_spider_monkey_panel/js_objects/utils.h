#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle;

class JsUtils
{
public:
    ~JsUtils();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    std::optional<bool> CheckComponent( const std::string& name, bool is_dll );
    std::optional<bool> CheckFont( const std::wstring& name );
    std::optional<uint32_t> ColourPicker( uint64_t hWindow, uint32_t default_colour );
    //std::optional<std::nullptr_t> FileTest( const std::wstring& path, const std::wstring&  mode, VARIANT* p );
    std::optional<std::string> FormatDuration( double p );
    std::optional<std::string> FormatFileSize( uint64_t p );
    std::optional<std::uint64_t> GetAlbumArtAsync( uint64_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );
    std::optional<JSObject*> GetAlbumArtEmbedded( const std::string& rawpath, uint32_t art_id );
    std::optional<JSObject*> GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub );
    std::optional<uint32_t> GetSysColour( uint32_t index );
    std::optional<uint32_t> GetSystemMetrics( uint32_t index );
    //std::optional<std::nullptr_t> Glob( const std::string& pattern, uint32_t exc_mask, uint32_t inc_mask, VARIANT* p );
    std::optional<bool> IsKeyPressed( uint32_t vkey );
    std::optional<std::wstring> MapString( const std::wstring& str, uint32_t lcid, uint32_t flags );
    std::optional<bool> PathWildcardMatch( const std::wstring& pattern, const std::wstring& str );
    std::optional<std::wstring> ReadINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval );
    // TODO: think about removing wstring here 
    std::optional<std::wstring> ReadTextFile( const std::wstring& filename, uint32_t codepage );
    // TODO: and here
    std::optional<bool> WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val );
    std::optional<bool> WriteTextFile( const std::string& filename, const std::string& content, bool write_bom );
    std::optional<uint32_t> get_Version();

private:
    JsUtils( JSContext* cx );
    JsUtils( const JsUtils& ) = delete;
    JsUtils& operator=( const JsUtils& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
};

}
