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

    void WrapGdiCall( std::function<void( HDC dc )> const& function );

public:
    uint32_t BeginContainer( float dst_x = 0, float dst_y = 0, float dst_w = 0, float dst_h = 0, float src_x = 0, float src_y = 0, float src_w = 0, float src_h = 0 );
    uint32_t BeginContainerWithOpt( size_t optArgCount, float dst_x, float dst_y, float dst_w, float dst_h, float src_x, float src_y, float src_w, float src_h );

    uint32_t CalcTextHeight( const std::wstring& text, JsGdiFont* font );
    uint32_t CalcTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcTextWidthWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, boolean use_exact );
    uint32_t CalcWriteTextHeight( const std::wstring& text, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcWriteTextHeightWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, boolean use_exact );
    uint32_t CalcWriteTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcWriteTextWidthWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, boolean use_exact );

    void Clear( uint32_t colour );

    JS::Value DrawEdge( int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t edge, uint32_t flags );
    void DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour );
    void DrawFocusRect( int32_t x, int32_t y, uint32_t w, uint32_t h );

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

    void DrawString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                     float x, float y, float w, float h,
                     uint32_t flags = 0 );
    void DrawStringWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, uint32_t colour,
                            float x, float y, float w, float h,
                            uint32_t flags );

    void EndContainer( uint32_t state );

    JSObject* EstimateLineWrap( const std::wstring& text, JsGdiFont* font, uint32_t max_width );

    void FillEllipse( float x, float y, float w, float h, uint32_t colour );
    void FillGradRect( float x, float y, float w, float h,
                       float angle, uint32_t colour1, uint32_t colour2, 
                       float focus = 1.0f, float scale = 1.0f );
    void FillGradRectWithOpt( size_t optArgCount,
                              float x, float y, float w, float h,
                              float angle, uint32_t colour1, uint32_t colour2, 
                              float focus, float scale );
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

    JS::Value GetClip();

    // uint32_t GetCompositingMode();
    uint32_t GetCompositingQuality();

    float GetDpiX();
    float GetDpiY();

    uint32_t GetInterpolationMode();

#ifdef DEBUG_FONT_METRICS
    JS::Value GetMetrics( const std::wstring& text, JsGdiFont* font );
#endif

    uint32_t GetPixelOffsetMode();
    uint32_t GetSmoothingMode();
    uint32_t GetTextContrast();
    uint32_t GetTextRenderingHint();

    JS::Value GetTransform();

    bool IsClipEmpty();
    bool IsPointVisible( float x, float y );
    bool IsRectVisible( float x, float y, float w, float h );
    bool IsVisibleClipEmpty();

    JSObject* MeasureString( const std::wstring& str, JsGdiFont* font,
                             float x, float y, float w, float h,
                             uint32_t flags = 0 );
    JSObject* MeasureStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font,
                                    float x, float y, float w, float h,
                                    uint32_t flags );

    void MultiplyTransform( float m11, float m12, float m21, float m22, float dx, float dy, uint32_t matrixOrder = 0 );
    void MultiplyTransformWithOpt( size_t optArgCount, float m11, float m12, float m21, float m22, float dx, float dy, uint32_t matrixOrder );

    void ResetClip();
    void ResetTransform();

    void Restore( uint32_t state );

    void RotateTransform( float angle, uint32_t matrixOrder = 0 );
    void RotateTransformWithOpt( size_t optArgCount, float angle, uint32_t matrixOrder );

    uint32_t Save();

    void ScaleTransform( float sx, float sy, uint32_t matrixOrder = 0 );
    void ScaleTransformWithOpt( size_t optArgCount, float sx, float sy, uint32_t matrixOrder );

    void SetClip( float x, float y, float w, float h );

    //void SetCompositingMode( uint32_t mode = 0 );
    //void SetCompositingModeWithOpt( size_t optArgCount, uint32_t mode );
    void SetCompositingQuality( uint32_t mode = 0 );
    void SetCompositingQualityWithOpt( size_t optArgCount, uint32_t mode );
    void SetInterpolationMode( uint32_t mode = 0 );
    void SetInterpolationModeWithOpt( size_t optArgCount, uint32_t mode );
    void SetPixelOffsetMode( uint32_t mode = 0 );
    void SetPixelOffsetModeWithOpt( size_t optArgCount, uint32_t mode );
    void SetSmoothingMode( uint32_t mode = 0 );
    void SetSmoothingModeWithOpt( size_t optArgCount, uint32_t mode );
    void SetTextContrast( uint32_t contrast );
    void SetTextRenderingHint( uint32_t mode = 0 );
    void SetTextRenderingHintWithOpt( size_t optArgCount, uint32_t mode );

    void SetTransform( float m11, float m12, float m21, float m22, float dx, float dy );

    JS::Value TransformPoint( float x, float y );
    JS::Value TransformRect( float x, float y, float w, float h );

    void TranslateTransform( float dx, float dy, uint32_t matrixOrder = 0 );
    void TranslateTransformWithOpt( size_t optArgCount, float dx, float dy, uint32_t matrixOrder );

    JS::Value UnTransformPoint( float x, float y );
    JS::Value UnTransformRect( float x, float y, float w, float h );

    void WriteString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                      float x, float y, float w, float h,
                      uint32_t flags = 0 );
    void WriteStringWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, uint32_t colour,
                             float x, float y, float w, float h,
                             uint32_t flags );

    void WriteText( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                    float x, float y, float w, float h,
                    uint32_t flags = 0 );
    void WriteTextWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, uint32_t colour,
                           float x, float y, float w, float h,
                           uint32_t flags );

private:
    JsGdiGraphics( JSContext* cx );

    void GetRoundRectPath( Gdiplus::GraphicsPath& gp, const Gdiplus::RectF& rect, float arc_width, float arc_height ) const;
    Gdiplus::StringFormat* UnpackStringFormat( uint32_t flags );
    void ParsePoints( JS::HandleValue jsValue, std::vector<Gdiplus::PointF>& gdiPoints );

private:
    JSContext* pJsCtx_ = nullptr;
    Gdiplus::Graphics* pGraphics = nullptr;
};

} // namespace mozjs
