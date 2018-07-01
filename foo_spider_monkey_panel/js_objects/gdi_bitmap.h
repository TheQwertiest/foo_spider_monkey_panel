#pragma once

#include <optional>
#include <memory>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Bitmap;
}

// TODO: add default arguments everywhere

namespace mozjs
{

class JsGdiGraphics;

class JsGdiBitmap
{
public:
    ~JsGdiBitmap();
    
    static JSObject* Create( JSContext* cx, Gdiplus::Bitmap* pGdiBitmap );

    static const JSClass& GetClass();

public: 
    Gdiplus::Bitmap* GdiBitmap() const;

public: //methods
    std::optional<JSObject*> ApplyAlpha( uint8_t alpha );
    std::optional<bool> ApplyMask( JsGdiBitmap* mask );
    std::optional<JSObject*> Clone( float x, float y, float w, float h );
    std::optional<JSObject*> CreateRawBitmap();
    //std::optional<std::nullptr_t> GetColourScheme( uint32_t count, VARIANT* outArray );
    //std::optional<std::nullptr_t> GetColourSchemeJSON( uint32_t count, BSTR* outJson );
    std::optional<JSObject*> GetGraphics();
    std::optional<std::nullptr_t> ReleaseGraphics( JsGdiGraphics* graphics );
    std::optional<JSObject*> Resize( uint32_t w, uint32_t h, uint32_t interpolationMode );
    std::optional<JSObject*> ResizeWithOpt( size_t optArgCount, uint32_t w, uint32_t h, uint32_t interpolationMode );
    std::optional<std::nullptr_t> RotateFlip( uint32_t mode );
    //std::optional<std::nullptr_t> SaveAs( BSTR path, BSTR format, VARIANT_BOOL* p );
    std::optional<std::nullptr_t> StackBlur( uint32_t radius );

public: // props
    std::optional<std::uint32_t> get_Height();
    std::optional<std::uint32_t> get_Width();

private:
    JsGdiBitmap( JSContext* cx, Gdiplus::Bitmap* pGdiBitmap );
    JsGdiBitmap( const JsGdiBitmap& ) = delete;
    JsGdiBitmap& operator=( const JsGdiBitmap& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;;
    std::unique_ptr<Gdiplus::Bitmap> pGdi_;
};

}
