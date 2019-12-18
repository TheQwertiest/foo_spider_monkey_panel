#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsGdiUtils
    : public JsObjectBase<JsGdiUtils>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsGdiUtils() override = default;

    static std::unique_ptr<JsGdiUtils> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    JSObject* CreateImage( uint32_t w, uint32_t h );
    JSObject* Font( const std::wstring& fontName, uint32_t pxSize, uint32_t style = 0 );
    JSObject* FontWithOpt( size_t optArgCount, const std::wstring& fontName, uint32_t pxSize, uint32_t style );
    JSObject* Image( const std::wstring& path );
    std::uint32_t LoadImageAsync( uint32_t hWnd, const std::wstring& path );
    JSObject* LoadImageAsyncV2( uint32_t hWnd, const std::wstring& path );

private:
    JsGdiUtils( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
