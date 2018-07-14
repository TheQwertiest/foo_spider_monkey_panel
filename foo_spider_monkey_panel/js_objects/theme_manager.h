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

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsThemeManager();

    static bool HasThemeData( HWND hwnd, const std::wstring& classlist );
    static std::unique_ptr<JsThemeManager> CreateNative( JSContext* cx, HWND hwnd, const std::wstring& classlist );
    static size_t GetInternalSize( HWND hwnd, const std::wstring& classlist );

public:
    std::optional<std::nullptr_t> DrawThemeBackground( JsGdiGraphics* gr, 
                                                       int32_t x, int32_t y, uint32_t w, uint32_t h, 
                                                       int32_t clip_x = 0, int32_t clip_y = 0, uint32_t clip_w = 0, uint32_t clip_h = 0 );
    std::optional<std::nullptr_t> DrawThemeBackgroundWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                                              int32_t x, int32_t y, uint32_t w, uint32_t h,
                                                              int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h );
    std::optional<bool> IsThemePartDefined( int32_t partid, int32_t stateId );
    std::optional<std::nullptr_t> SetPartAndStateID( int32_t partid, int32_t stateId = 0);
    std::optional<std::nullptr_t> SetPartAndStateIDWithOpt( size_t optArgCount, int32_t partid, int32_t stateId );

private:
    JsThemeManager( JSContext* cx, HWND hwnd, HTHEME hTheme );

private:
    JSContext * pJsCtx_ = nullptr;

    HTHEME hTheme_ = nullptr;
    int32_t partId_ = 0;
    int32_t stateId_ = 0;
};

}
