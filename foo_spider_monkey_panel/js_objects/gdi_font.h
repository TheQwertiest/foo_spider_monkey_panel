#pragma once

#include <js_engine/js_error_codes.h>


class JSObject;
struct JSContext;
struct JSClass;

namespace Gdi
{
class Font;
}

namespace mozjs
{

/*

class GdiFont : public GdiObj<IGdiFont, Gdiplus::Font>
{
protected:
HFONT m_hFont;
bool m_managed;

GdiFont(Gdiplus::Font* p, HFONT hFont, bool managed = true);
virtual ~GdiFont();
virtual void FinalRelease();

public:
STDMETHODIMP get_Height(UINT* p);
STDMETHODIMP get_Name(LANGID langId, BSTR* outName);
STDMETHODIMP get_Size(float* outSize);
STDMETHODIMP get_Style(INT* outStyle);
};

*/

class JsGdiFont
{
public:
    ~JsGdiFont();
    
    static JSObject* Create( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont );

    static const JSClass& GetClass();

public: 
    Gdiplus::Font* GetGdiFont() const;
    HFONT GetHFont() const;

private:
    JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont );
    JsGdiFont( const JsGdiFont& ) = delete;

private:
    JSContext * pJsCtx_;
    std::unique_ptr<Gdiplus::Font> gdiFont_;
    HFONT hFont_;
};

}
