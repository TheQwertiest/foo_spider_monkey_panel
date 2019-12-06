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
    void DrawThemeBackground( JsGdiGraphics* gr,
                              int32_t x, int32_t y, uint32_t w, uint32_t h,
                              int32_t clip_x = 0, int32_t clip_y = 0, uint32_t clip_w = 0, uint32_t clip_h = 0 );
    void DrawThemeBackgroundWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                     int32_t x, int32_t y, uint32_t w, uint32_t h,
                                     int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h );
    bool IsThemePartDefined( int32_t partid, int32_t stateId );
    void SetPartAndStateID( int32_t partid, int32_t stateId = 0 );
    void SetPartAndStateIDWithOpt( size_t optArgCount, int32_t partid, int32_t stateId );

private:
    JsThemeManager( JSContext* cx, HTHEME hTheme );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    HTHEME hTheme_ = nullptr;
    int32_t partId_ = 0;
    int32_t stateId_ = 0;
};

} // namespace mozjs
