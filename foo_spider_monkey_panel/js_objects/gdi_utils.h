#pragma once

#include <js_objects/object_base.h>

#include <string>
#include <optional>

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

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsGdiUtils();

    static std::unique_ptr<JsGdiUtils> CreateNative( JSContext* cx );

public:
    std::optional<JSObject*> CreateImage( uint32_t w, uint32_t h );
    std::optional<JSObject*> Font( const std::wstring& fontName, float pxSize, uint32_t style = 0 );
    std::optional<JSObject*> FontWithOpt( size_t optArgCount, const std::wstring& fontName, float pxSize, uint32_t style );
    std::optional<JSObject*> Image( const std::wstring& path );
    std::optional<std::uint32_t> LoadImageAsync( uint32_t hWnd, const std::wstring& path );

private:
    JsGdiUtils( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;;
};

}
