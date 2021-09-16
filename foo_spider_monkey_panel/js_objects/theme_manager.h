#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsGdiGraphics;

class JsThemeManager
    : public JsObjectBase<JsThemeManager>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsThemeManager() override;

    static bool HasThemeData( HWND hwnd, const std::wstring& classId );
    static std::unique_ptr<JsThemeManager> CreateNative( JSContext* cx, HWND hwnd, const std::wstring& classId );
    static size_t GetInternalSize( HWND hwnd, const std::wstring& classId );

public:
    bool IsThemePartDefined( int32_t partId, int32_t stateId );

    void SetPartAndStateID( int32_t partId, int32_t stateId = 0 );
    void SetPartAndStateIDWithOpt( size_t optArgCount, int32_t partid, int32_t stateId );

    void DrawThemeBackground( JsGdiGraphics* gr,
                              int32_t x, int32_t y, uint32_t w, uint32_t h,
                              int32_t clip_x = 0, int32_t clip_y = 0, uint32_t clip_w = 0, uint32_t clip_h = 0 );
    void DrawThemeBackgroundWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                     int32_t x, int32_t y, uint32_t w, uint32_t h,
                                     int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h );

    void DrawThemeText( JsGdiGraphics* gr,
                        const std::wstring& str,
                        int32_t x, int32_t y, uint32_t w, uint32_t h,
                        uint32_t format = 0 );
    void DrawThemeTextWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                               const std::wstring& str,
                               int32_t x, int32_t y, uint32_t w, uint32_t h,
                               uint32_t format );

    int32_t GetThemePropertyOrigin( int32_t propId );
    int32_t GetThemeMetric( int32_t propId );

    bool GetThemeBool( int32_t propId );
    int32_t GetThemeInt( int32_t propId );
    uint32_t GetThemeColour( int32_t propId );
    JS::Value GetThemeMargins( int32_t propId );
    JS::Value GetThemePosition( int32_t propId );
    JS::Value GetThemePartSize( int32_t themeSize, JsGdiGraphics* gr = nullptr, int32_t x = 0, int32_t y = 0, uint32_t w = 0, uint32_t h = 0 );
    JS::Value GetThemePartSizeWithOpt( size_t optArgCount, int32_t themeSize, JsGdiGraphics* gr, int32_t x, int32_t y, uint32_t w, uint32_t h );
    int32_t GetThemeEnumValue( int32_t propId );

    // JS::Value GetThemeFilename( int32_t propId );
    // JS::Value GetThemeIntList( int32_t propId );
    // JS::Value GetThemeRect( int32_t propId );
    // JSObject* GetThemeStream( int32_t propId );
    // JS::Value GetThemeString( int32_t propId );

    JS::Value GetThemeBackgroundContentRect( int32_t x, int32_t y, uint32_t w, uint32_t h );

    int32_t GetThemeSysInt( int32_t propId );
    int32_t GetThemeSysSize( int32_t propId );
    uint32_t GetThemeSysColour( int32_t propId );

    JSObject* GetThemeFont( int32_t propId );
    JS::Value GetThemeFontArgs( int32_t propId );

    JSObject* GetThemeSysFont( int32_t propId );
    JS::Value GetThemeSysFontArgs( int32_t propId );

private:
    JsThemeManager( JSContext* cx, HTHEME hTheme );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    JSObject* MakeFont( HDC dc, LOGFONT* plf );
    JS::Value MakeFontArgs( HDC dc, LOGFONT* plf );

    HTHEME hTheme_ = nullptr;
    int32_t partId_ = 0;
    int32_t stateId_ = 0;
};

} // namespace mozjs
