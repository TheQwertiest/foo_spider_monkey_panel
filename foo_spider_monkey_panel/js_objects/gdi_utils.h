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
    std::optional<JSObject*> Font( std::wstring fontName, float pxSize, uint32_t style );
    std::optional<JSObject*> FontWithOpt( size_t optArgCount, std::wstring fontName, float pxSize, uint32_t style );
    //STDMETHODIMP Image( BSTR path, IGdiBitmap** pp );
    //STDMETHODIMP LoadImageAsync( UINT window_id, BSTR path, UINT* p );

private:
    JsGdiUtils( JSContext* cx );
    JsGdiUtils( const JsGdiUtils& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;;
};

}
