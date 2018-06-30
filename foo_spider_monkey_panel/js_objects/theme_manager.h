#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsGdiGraphics;

class JsThemeManager
{
public:
    ~JsThemeManager();

    static JSObject* Create( JSContext* cx, HWND hwnd, std::wstring classlist );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> DrawThemeBackground( JsGdiGraphics* gr, int32_t x, int32_t y, uint32_t w, uint32_t h, int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h );
    // TODO: document changes for these two methods
    std::optional<bool> IsThemePartDefined( int32_t partid );
    std::optional<std::nullptr_t> SetPartID( int32_t partid );

private:
    JsThemeManager( JSContext* cx, HWND hwnd, std::wstring classlist );
    JsThemeManager( const JsThemeManager& ) = delete;
    JsThemeManager& operator=( const JsThemeManager& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;

    HTHEME hTheme_;
    int32_t partId_;    
}
