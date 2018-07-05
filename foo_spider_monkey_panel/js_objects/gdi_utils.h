#pragma once

#include <string>
#include <optional>

class JSObject;
struct JSContext;
struct JSClass;


namespace mozjs
{

class JsGdiUtils
{
public:
    ~JsGdiUtils();
    
    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public: 
    std::optional<JSObject*> CreateImage( uint32_t w, uint32_t h );
    std::optional<JSObject*> Font( const std::wstring& fontName, float pxSize, uint32_t style = 0 );
    std::optional<JSObject*> FontWithOpt( size_t optArgCount, const std::wstring& fontName, float pxSize, uint32_t style );
    std::optional<JSObject*> Image( const std::wstring& path );
    std::optional<std::uint32_t> LoadImageAsync( uint32_t hWnd, const std::wstring& path );

private:
    JsGdiUtils( JSContext* cx );
    JsGdiUtils( const JsGdiUtils& ) = delete;
    JsGdiUtils& operator=( const JsGdiUtils& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;;
};

}
