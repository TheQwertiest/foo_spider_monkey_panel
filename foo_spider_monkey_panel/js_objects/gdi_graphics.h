#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>

class JSObject;
struct JSContext;

namespace Gdi
{
class Graphics;
}

namespace mozjs
{

class JsGdiFont;
class JsGdiBitmap;
class JsGdiRawBitmap;

class JsGdiGraphics
{
public:
    ~JsGdiGraphics();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    Gdiplus::Graphics* GetGraphicsObject() const;
    void SetGraphicsObject( Gdiplus::Graphics* graphics );

public:
    std::optional<uint32_t> CalcTextHeight( const std::wstring& str, JsGdiFont* font );
    std::optional<uint32_t> CalcTextWidth( const std::wstring& str, JsGdiFont* font );
    std::optional<std::nullptr_t> DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawImage( JsGdiBitmap* image, 
                                             float dstX, float dstY, float dstW, float dstH, 
                                             float srcX, float srcY, float srcW, float srcH, 
                                             float angle = 0, uint8_t alpha = 255 );
    std::optional<std::nullptr_t> DrawImageWithOpt( size_t optArgCount, JsGdiBitmap* image,
                                             float dstX, float dstY, float dstW, float dstH,
                                             float srcX, float srcY, float srcW, float srcH,
                                             float angle, uint8_t alpha );
    std::optional<std::nullptr_t> DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawPolygon( uint32_t colour, float line_width, JS::HandleValue points );
    std::optional<std::nullptr_t> DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawString( const std::wstring& str, JsGdiFont* font, uint32_t colour, 
                                              float x, float y, float w, float h, 
                                              uint32_t flags = 0 );
    std::optional<std::nullptr_t> DrawStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour, 
                                                     float x, float y, float w, float h, 
                                                     uint32_t flags );
    std::optional<JSObject*> EstimateLineWrap( const std::wstring& str, JsGdiFont* font, uint32_t max_width );
    std::optional<std::nullptr_t> FillEllipse( float x, float y, float w, float h, uint32_t colour );
    std::optional<std::nullptr_t> FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus );
    std::optional<std::nullptr_t> FillPolygon( uint32_t colour, uint32_t fillmode, JS::HandleValue points );
    std::optional<std::nullptr_t> FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour );
    std::optional<std::nullptr_t> FillSolidRect( float x, float y, float w, float h, uint32_t colour );
    std::optional<std::nullptr_t> GdiAlphaBlend( JsGdiRawBitmap* bitmap, 
                                                 int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstHGdiDrawText,
                                                 int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH, 
                                                 uint8_t alpha = 255);
    std::optional<std::nullptr_t> GdiAlphaBlendWithOpt( size_t optArgCount, JsGdiRawBitmap* bitmap,
                                                        int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                                        int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                                                        uint8_t alpha );
    std::optional<std::nullptr_t> GdiDrawBitmap( JsGdiRawBitmap* bitmap, int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH, int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH );
    std::optional<std::nullptr_t> GdiDrawText( const std::wstring& str, JsGdiFont* font, uint32_t colour, 
                                               int32_t x, int32_t y, uint32_t w, uint32_t h,
                                               uint32_t format = 0 );
    std::optional<std::nullptr_t> GdiDrawTextWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
                                               int32_t x, int32_t y, uint32_t w, uint32_t h,
                                               uint32_t format );
    std::optional<JSObject*> MeasureString( const std::wstring& str, JsGdiFont* font, 
                                            float x, float y, float w, float h, 
                                            uint32_t flags = 0);
    std::optional<JSObject*> MeasureStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, 
                                                   float x, float y, float w, float h, 
                                                   uint32_t flags );
    std::optional<std::nullptr_t> SetInterpolationMode( uint32_t mode = 0);
    std::optional<std::nullptr_t> SetInterpolationModeWithOpt( size_t optArgCount, uint32_t mode );
    std::optional<std::nullptr_t> SetSmoothingMode( uint32_t mode = 0);
    std::optional<std::nullptr_t> SetSmoothingModeWithOpt( size_t optArgCount, uint32_t mode );
    std::optional<std::nullptr_t> SetTextRenderingHint( uint32_t mode = 0);
    std::optional<std::nullptr_t> SetTextRenderingHintWithOpt( size_t optArgCount, uint32_t mode );

private:
    JsGdiGraphics( JSContext* cx );
    JsGdiGraphics( const JsGdiGraphics& ) = delete;
    JsGdiGraphics& operator=( const JsGdiGraphics& ) = delete;

    bool GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height );
    bool ParsePoints( JS::HandleValue jsValue, std::vector<Gdiplus::PointF> &gdiPoints );

private:
    JSContext * pJsCtx_ = nullptr;
    Gdiplus::Graphics* pGdi_ = nullptr;
};

}
