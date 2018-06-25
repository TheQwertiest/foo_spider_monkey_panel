#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>
#include <memory>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Bitmap;
}

namespace mozjs
{

/*
class GdiBitmap : public GdiObj<IGdiBitmap, Gdiplus::Bitmap>
{
protected:
GdiBitmap(Gdiplus::Bitmap* p);

public:
STDMETHODIMP ApplyAlpha(BYTE alpha, IGdiBitmap** pp);
STDMETHODIMP ApplyMask(IGdiBitmap* mask, VARIANT_BOOL* p);
STDMETHODIMP Clone(float x, float y, float w, float h, IGdiBitmap** pp);
STDMETHODIMP CreateRawBitmap(IGdiRawBitmap** pp);
STDMETHODIMP GetColourScheme(UINT count, VARIANT* outArray);
STDMETHODIMP GetColourSchemeJSON(UINT count, BSTR* outJson);
STDMETHODIMP GetGraphics(IGdiGraphics** pp);
STDMETHODIMP ReleaseGraphics(IGdiGraphics* p);
STDMETHODIMP Resize(UINT w, UINT h, int interpolationMode, IGdiBitmap** pp);
STDMETHODIMP RotateFlip(UINT mode);
STDMETHODIMP SaveAs(BSTR path, BSTR format, VARIANT_BOOL* p);
STDMETHODIMP StackBlur(int radius);
STDMETHODIMP get_Height(UINT* p);
STDMETHODIMP get_Width(UINT* p);
};

*/

class JsGdiBitmap
{
public:
    ~JsGdiBitmap();
    
    static JSObject* Create( JSContext* cx, Gdiplus::Bitmap* pGdiBitmap );

    static const JSClass& GetClass();

public: 
    Gdiplus::Bitmap* GdiBitmap() const;

public: // props
    std::optional<std::uint32_t> Height();
    std::optional<std::uint32_t> Width();

public: //methods
    std::optional<JSObject*> ApplyAlpha( uint8_t alpha );
    //std::optional<std::nullptr_t> ApplyMask( JS::HandleValue mask, VARIANT_BOOL* p );
    std::optional<JSObject*> Clone( float x, float y, float w, float h );
    std::optional<JSObject*> CreateRawBitmap();
    //std::optional<std::nullptr_t> GetColourScheme( uint32_t count, VARIANT* outArray );
    //std::optional<std::nullptr_t> GetColourSchemeJSON( uint32_t count, BSTR* outJson );
    std::optional<JSObject*> GetGraphics();
    std::optional<std::nullptr_t> ReleaseGraphics( JS::HandleValue p );
    std::optional<JSObject*> Resize( uint32_t w, uint32_t h, uint32_t interpolationMode );
    std::optional<JSObject*> ResizeWithOpt( size_t optArgCount, uint32_t w, uint32_t h, uint32_t interpolationMode );
    std::optional<std::nullptr_t> RotateFlip( uint32_t mode );
    //std::optional<std::nullptr_t> SaveAs( BSTR path, BSTR format, VARIANT_BOOL* p );
    std::optional<std::nullptr_t> StackBlur( uint32_t radius );

private:
    JsGdiBitmap( JSContext* cx, Gdiplus::Bitmap* pGdiBitmap );
    JsGdiBitmap( const JsGdiBitmap& ) = delete;

private:
    JSContext * pJsCtx_;
    std::unique_ptr<Gdiplus::Bitmap> pGdi_;
};

}
