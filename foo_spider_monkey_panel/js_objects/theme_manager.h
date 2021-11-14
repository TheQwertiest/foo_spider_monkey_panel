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

    static bool HasThemeData( HWND wnd, const std::wstring& themeClass );
    static std::unique_ptr<JsThemeManager> CreateNative( JSContext* ctx, HWND wnd, const std::wstring& themeClass );
    static size_t GetInternalSize( HWND wnd, const std::wstring& themeClass );

    // static void GetClientMetrics();

public:
    void DrawThemeBackground( JsGdiGraphics* gr,
                              int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                              int32_t clipX = 0, int32_t clipY = 0, uint32_t clipW = 0, uint32_t clipH = 0 );
    void DrawThemeBackgroundWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                     int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                     int32_t clipX, int32_t clipY, uint32_t clipW, uint32_t clipH );
    JS::Value DrawThemeEdge( JsGdiGraphics* gr,
                             int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                             uint32_t edge, uint32_t flags );
    void DrawThemeText( JsGdiGraphics* gr, const std::wstring& text,
                        int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                        uint32_t format = 0, uint32_t fontprop = 0 );
    void DrawThemeTextWithOpt( size_t optArgCount, JsGdiGraphics* gr, const std::wstring& text,
                               int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                               uint32_t format, uint32_t fontprop );
    JS::Value GetThemeBackgroundContentRect( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH );
    bool GetThemeBool( int32_t propId );
    uint32_t GetThemeColour( int32_t propId );
    int32_t GetThemeEnumValue( int32_t propId );
    // JS::Value GetThemeFilename( int32_t propId );
    JSObject* GetThemeFont( int32_t propId );
    JS::Value GetThemeFontArgs( int32_t propId );
    int32_t GetThemeInt( int32_t propId );
    JS::Value GetThemeIntList( int32_t propId );
    JS::Value GetThemeMargins( int32_t propId );
    int32_t GetThemeMetric( int32_t propId );
    JS::Value GetThemePartSize( int32_t themeSize,
                                int32_t rectX = 0, int32_t rectY = 0, uint32_t rectW = 0, uint32_t rectH = 0 );
    JS::Value GetThemePartSizeWithOpt( size_t optArgCount, int32_t themeSize,
                                       int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH );
    JS::Value GetThemePosition( int32_t propId );
    int32_t GetThemePropertyOrigin( int32_t propId );
    JS::Value GetThemeRect( int32_t propId );
    // JSObject* GetThemeStream( int32_t propId );
    // JS::Value GetThemeString( int32_t propId );
    BOOL GetThemeSysBool( int32_t boolId );
    uint32_t GetThemeSysColour( int32_t colourId );
    JSObject* GetThemeSysFont( int32_t fontId );
    JS::Value GetThemeSysFontArgs( int32_t fontId );
    int32_t GetThemeSysInt( int32_t intId );
    int32_t GetThemeSysSize( int32_t sizeId );

    bool IsThemeFontDefined( int32_t propId );
    bool IsThemePartDefined( int32_t partId, int32_t stateId );

    void SetPartAndStateID( int32_t partId, int32_t stateId = 0 );
    void SetPartAndStateIDWithOpt( size_t optArgCount, int32_t partId, int32_t stateId );

private:
    JsThemeManager( JSContext* cx, HTHEME hTheme );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    HTHEME theme = nullptr;
    int32_t partID = 0;
    int32_t stateID = 0;


private:
    JS::Value MakeFontArgs( LOGFONTW* plf );
};
} // namespace mozjs
