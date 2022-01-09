#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;

using Gdiplus::Graphics;

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
    uint32_t BeginContainer( float dstX = 0, float dstY = 0, float dstW = 0, float dstH = 0,
                             float srcX = 0, float srcY = 0, float srcW = 0, float srcH = 0 );
    uint32_t BeginContainerWithOpt( size_t optArgCount,
                                    float dstX, float dstY, float dstW, float dstH,
                                    float srcX, float srcY, float srcW, float srcH );

    uint32_t CalcTextHeight( const std::wstring& text, JsGdiFont* font );
    uint32_t CalcTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcTextWidthWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, boolean use_exact );

    uint32_t CalcWriteTextHeight( const std::wstring& text, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcWriteTextHeightWithOpt( size_t optArgCount,
                                         const std::wstring& text, JsGdiFont* font, boolean use_exact );
    uint32_t CalcWriteTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact = false );
    uint32_t CalcWriteTextWidthWithOpt( size_t optArgCount,
                                        const std::wstring& text, JsGdiFont* font, boolean use_exact );

    void Clear( uint32_t colour );

    JS::Value DrawEdge( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH, uint32_t edge, uint32_t flags );
    void DrawEllipse( float rectX, float rectY, float rectW, float rectH, float lineWidth, uint32_t colour  );
    void DrawFrameControl( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                           uint32_t type, uint32_t state );
    void DrawFocusRect( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH );

    void DrawImage( JsGdiBitmap* image,
                    float dstX, float dstY, float dstW, float dstH,
                    float srcX, float srcY, float srcW, float srcH,
                    float angle = 0, uint8_t alpha = 255 );
    void DrawImageWithOpt( size_t optArgCount,
                           JsGdiBitmap* image,
                           float dstX, float dstY, float dstW, float dstH,
                           float srcX, float srcY, float srcW, float srcH,
                           float angle, uint8_t alpha );

    void DrawLine( float x1, float y1, float x2, float y2, float lineWidth, uint32_t colour );
    void DrawPolygon( uint32_t colour, float lineWidth, JS::HandleValue points );
    void DrawRect( float rectX, float rectY, float rectW, float rectH, float lineWidth, uint32_t colour );
    void DrawRoundRect( float rectX, float rectY, float rectW, float rectH,
                        float arcW, float arcH, float lineWidth, uint32_t colour );

    void DrawString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                     float rectX, float rectY, float rectW, float rectH, uint32_t flags = 0 );
    void DrawStringWithOpt( size_t optArgCount,
                            const std::wstring& text, JsGdiFont* font, uint32_t colour,
                            float rectX, float rectY, float rectW, float rectH, uint32_t flags );

    void EndContainer( uint32_t state );

    JSObject* EstimateLineWrap( const std::wstring& text, JsGdiFont* font, uint32_t max_width );

    void FillEllipse( float rectX, float rectY, float rectW, float rectH, uint32_t colour );

    void FillGradRect( float rectX, float rectY, float rectW, float rectH,
                       float angle,  uint32_t colour1, uint32_t colour2,
                       float focus = 1.0f, float scale = 1.0f );
    void FillGradRectWithOpt( size_t optArgCount,
                              float rectX, float rectY, float rectW, float rectH,
                              float angle, uint32_t colour1, uint32_t colour2,
                              float focus, float scale );

    void FillPolygon( uint32_t colour, uint32_t fillmode, JS::HandleValue points );
    void FillRoundRect( float rectX, float rectY, float rectW, float rectH,
                        float arcW, float arcH, uint32_t colour );
    void FillSolidRect( float rectX, float rectY, float rectW, float rectH, uint32_t colour );

    void GdiAlphaBlend( JsGdiRawBitmap* bitmap,
                        int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstHGdiDrawText,
                        int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                        uint8_t alpha = 255 );
    void GdiAlphaBlendWithOpt( size_t optArgCount,
                               JsGdiRawBitmap* bitmap,
                               int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                               int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                               uint8_t alpha );

    void GdiDrawBitmap( JsGdiRawBitmap* bitmap,
                        int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                        int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH );

    void GdiDrawText( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                      int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                      uint32_t format = 0 );
    void GdiDrawTextWithOpt( size_t optArgCount,
                             const std::wstring& text, JsGdiFont* font, uint32_t colour,
                             int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                             uint32_t format );

    JS::Value GetClip();

    uint32_t GetCompositingQuality();

    float GetDpiX();
    float GetDpiY();

    uint32_t GetInterpolationMode();
    uint32_t GetPixelOffsetMode();
    uint32_t GetSmoothingMode();
    uint32_t GetTextContrast();
    uint32_t GetTextRenderingHint();

    JS::Value GetTransform();

    bool IsClipEmpty();
    bool IsPointVisible( float rectX, float rectY );
    bool IsRectVisible( float rectX, float rectY, float rectW, float rectH );
    bool IsVisibleClipEmpty();

    JSObject* MeasureString( const std::wstring& text, JsGdiFont* font,
                             float rectX, float rectY, float rectW, float rectH,
                             uint32_t flags = 0 );
    JSObject* MeasureStringWithOpt( size_t optArgCount,
                                    const std::wstring& text, JsGdiFont* font,
                                    float rectX, float rectY, float rectW, float rectH,
                                    uint32_t flags );

    void MultiplyTransform( float M11, float M12,
                            float M21, float M22,
                            float dX,  float dY,
                            uint32_t matrixOrder = 0 );
    void MultiplyTransformWithOpt( size_t optArgCount,
                                   float M11, float M12,
                                   float M21, float M22,
                                   float dX,  float dY,
                                   uint32_t matrixOrder );

    void ResetClip();
    void ResetTransform();

    void Restore( uint32_t state );

    void RotateTransform( float angle, uint32_t matrixOrder = 0 );
    void RotateTransformWithOpt( size_t optArgCount, float angle, uint32_t matrixOrder );

    uint32_t Save();

    void ScaleTransform( float sX, float sY, uint32_t matrixOrder = 0 );
    void ScaleTransformWithOpt( size_t optArgCount, float sX, float sY, uint32_t matrixOrder );

    void SetClip( float rectX, float rectY, float rectW, float rectH );

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

    void SetTransform( float M11, float M12, float M21, float M22, float dX, float dy );

    JS::Value TransformPoint( float rectX, float rectY );
    JS::Value TransformRect( float rectX, float rectY, float rectW, float rectH );

    void TranslateTransform( float dX, float dY, uint32_t matrixOrder = 0 );
    void TranslateTransformWithOpt( size_t optArgCount, float dX, float dY, uint32_t matrixOrder );

    JS::Value UnTransformPoint( float rectX, float rectY );
    JS::Value UnTransformRect( float rectX, float rectY, float rectW, float rectH );

    void WriteString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                      float rectX, float rectY, float rectW, float rectH,
                      uint32_t flags = 0 );
    void WriteStringWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, uint32_t colour,
                             float rectX, float rectY, float rectW, float rectH,
                             uint32_t flags );

    void WriteText( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                    float rectX, float rectY, float rectW, float rectH,
                    uint32_t flags = 0 );
    void WriteTextWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, uint32_t colour,
                           float rectX, float rectY, float rectW, float rectH,
                           uint32_t flags );

private:
    JsGdiGraphics( JSContext* ctx );

    void GetRoundRectPath( Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, float arcW, float arcH ) const;
    Gdiplus::StringFormat* UnpackStringFormat( uint32_t flags );
    void ParsePoints( JS::HandleValue jsValue, std::vector<Gdiplus::PointF>& gdiPoints );

private:
    JSContext* pJsCtx_ = nullptr;
    Gdiplus::Graphics* pGraphics = nullptr;
};

} // namespace mozjs
