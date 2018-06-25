#pragma once

#include <string>
#include <optional>

class JSObject;
struct JSContext;


namespace mozjs
{

/*

class GdiUtils : public IDispatchImpl3<IGdiUtils>
{
protected:
GdiUtils();
virtual ~GdiUtils();

public:
STDMETHODIMP CreateImage(int w, int h, IGdiBitmap** pp);

STDMETHODIMP Image(BSTR path, IGdiBitmap** pp);
STDMETHODIMP LoadImageAsync(UINT window_id, BSTR path, UINT* p);
};

*/

class JsGdiUtils
{
public:
    ~JsGdiUtils();
    
    static JSObject* Create( JSContext* cx );

public: 
    std::optional<JSObject*> Font( std::wstring fontName, float pxSize, uint32_t style );
    std::optional<JSObject*> FontWithOpt( size_t optArgCount, std::wstring fontName, float pxSize, uint32_t style );

private:
    JsGdiUtils( JSContext* cx );
    JsGdiUtils( const JsGdiUtils& ) = delete;

private:
    JSContext * pJsCtx_;
};

}
