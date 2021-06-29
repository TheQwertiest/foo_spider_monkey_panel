#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;

namespace Gdiplus
{
class Graphics;
}

namespace mozjs
{

class JsGdiFont;
class JsGdiBitmap;
class JsGdiRawBitmap;

class JsGdiGraphics
    : public JsObjectBase<JsGdiGraphics>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsGdiGraphics() override = default;

    static std::unique_ptr<JsGdiGraphics> CreateNative( JSContext* cx );
    [[nodiscard]] static size_t GetInternalSize();

public:
    [[nodiscard]] Gdiplus::Graphics* GetGraphicsObject() const;
    void SetGraphicsObject( Gdiplus::Graphics* graphics );

public:
    uint32_t CalcTextHeight( const std::wstring& str, JsGdiFont* font );
    uint32_t CalcTextWidth( const std::wstring& str, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcTextWidthWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, boolean use_exact );
    void DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour );
    void DrawImage( JsGdiBitmap* image,
                    float dstX, float dstY, float dstW, float dstH,
                    float srcX, float srcY, float srcW, float srcH,
                    float angle = 0, uint8_t alpha = 255 );
    void DrawImageWithOpt( size_t optArgCount, JsGdiBitmap* image,
                           float dstX, float dstY, float dstW, float dstH,
                           float srcX, float srcY, float srcW, float srcH,
                           float angle, uint8_t alpha );
    void DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour );
    void DrawPolygon( uint32_t colour, float line_width, JS::HandleValue points );
    void DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour );
    void DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour );
    void DrawString( const std::wstring& str, JsGdiFont* font, uint32_t colour,
                     float x, float y, float w, float h,
                     uint32_t flags = 0 );
    void DrawStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
                            float x, float y, float w, float h,
                            uint32_t flags );
    JSObject* EstimateLineWrap( const std::wstring& str, JsGdiFont* font, uint32_t max_width );
    void FillEllipse( float x, float y, float w, float h, uint32_t colour );
    void FillGradRect( float x, float y, float w, float h,
                       float angle, uint32_t colour1, uint32_t colour2, float focus = 1 );
    void FillGradRectWithOpt( size_t optArgCount,
                              float x, float y, float w, float h,
                              float angle, uint32_t colour1, uint32_t colour2, float focus );
    void FillPolygon( uint32_t colour, uint32_t fillmode, JS::HandleValue points );
    void FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour );
    void FillSolidRect( float x, float y, float w, float h, uint32_t colour );
    void GdiAlphaBlend( JsGdiRawBitmap* bitmap,
                        int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstHGdiDrawText,
                        int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                        uint8_t alpha = 255 );
    void GdiAlphaBlendWithOpt( size_t optArgCount, JsGdiRawBitmap* bitmap,
                               int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                               int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                               uint8_t alpha );
    void GdiDrawBitmap( JsGdiRawBitmap* bitmap,
                        int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                        int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH );
    void GdiDrawText( const std::wstring& str, JsGdiFont* font, uint32_t colour,
                      int32_t x, int32_t y, uint32_t w, uint32_t h,
                      uint32_t format = 0 );
    void GdiDrawTextWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
                             int32_t x, int32_t y, uint32_t w, uint32_t h,
                             uint32_t format );
    JSObject* MeasureString( const std::wstring& str, JsGdiFont* font,
                             float x, float y, float w, float h,
                             uint32_t flags = 0 );
    JSObject* MeasureStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font,
                                    float x, float y, float w, float h,
                                    uint32_t flags );
    void SetInterpolationMode( uint32_t mode = 0 );
    void SetInterpolationModeWithOpt( size_t optArgCount, uint32_t mode );
    void SetSmoothingMode( uint32_t mode = 0 );
    void SetSmoothingModeWithOpt( size_t optArgCount, uint32_t mode );
    void SetTextRenderingHint( uint32_t mode = 0 );
    void SetTextRenderingHintWithOpt( size_t optArgCount, uint32_t mode );

private:
    JsGdiGraphics( JSContext* cx );

    void GetRoundRectPath( Gdiplus::GraphicsPath& gp, const Gdiplus::RectF& rect, float arc_width, float arc_height ) const;
    void ParsePoints( JS::HandleValue jsValue, std::vector<Gdiplus::PointF>& gdiPoints );

private:
    JSContext* pJsCtx_ = nullptr;
    Gdiplus::Graphics* pGdi_ = nullptr;
};

} // namespace mozjs
