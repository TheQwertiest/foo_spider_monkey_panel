#pragma once

#include <dom/font_description.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/canvas/native/canvas_surface.h>
#include <js_backend/objects/dom/canvas/text_metrics.h>
#include <utils/not_null.h>

#include <js/TypeDecls.h>

#include <optional>

namespace Gdiplus
{
class Brush;
class Font;
class Graphics;
class GraphicsPath;
class Pen;
class StringFormat;
} // namespace Gdiplus

namespace mozjs
{
class ImageData;
class CanvasGradient_Qwr;
class CanvasRenderingContext2D_Qwr;

template <>
struct JsObjectTraits<CanvasRenderingContext2D_Qwr>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class CanvasRenderingContext2D_Qwr
    : public JsObjectBase<CanvasRenderingContext2D_Qwr>
{
    enum class TextAlign
    {
        start,
        end,
        left,
        center,
        right,
    };

    enum class TextBaseline
    {
        top,
        middle,
        bottom,
        alphabetic,
    };

    struct FillTextExOptions
    {
        bool hasUnderline = false;
        bool hasLineThrough = false;
        bool shouldCollapseSpaces = true;
        bool shouldCollapseNewLines = true;
        bool shouldUseCanvasCollapseRules = false;
        bool shouldWrapText = true;
        bool shouldClipByRect = true;
        // normal, nowrap, pre, pre-wrap
        qwr::u8string whiteSpace = "normal";
        // clip, ellipsis, clip-char, ellipsis-char
        qwr::u8string textOverflow = "clip";
        // fast, stroke-compat
        qwr::u8string renderMode = "alpha-compat";
        double width = 0;
        double height = 0;
    };

public:
    ~CanvasRenderingContext2D_Qwr() override;

    [[nodiscard]] static std::unique_ptr<CanvasRenderingContext2D_Qwr>
    CreateNative( JSContext* cx, JS::HandleObject jsCanvas, ICanvasSurface& surface );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    void Reinitialize();

public:
    void BeginPath();
    JSObject* CreateLinearGradient( double x0, double y0, double x1, double y1 );
    void DrawImage1( JS::HandleValue image,
                     double dx, double dy );
    void DrawImage2( JS::HandleValue image,
                     double dx, double dy,
                     double dw, double dh );
    void DrawImage3( JS::HandleValue image,
                     double sx, double sy,
                     double sw, double sh,
                     double dx, double dy,
                     double dw, double dh );
    void DrawImageWithOpt( size_t optArgCount, JS::HandleValue image,
                           double arg1, double arg2,
                           double arg3, double arg4,
                           double arg5, double arg6,
                           double arg7, double arg8 );
    void Ellipse( double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise = false );
    void EllipseWithOpt( size_t optArgCount, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise );
    void Fill( const qwr::u8string& fillRule = "nonzero" );
    void FillWithOpt( size_t optArgCount, const qwr::u8string& fillRule );
    void FillRect( double x, double y, double w, double h );
    void FillText( const std::wstring& text, double x, double y );
    void FillTextEx( const std::wstring& text, double x, double y, JS::HandleValue options = JS::UndefinedHandleValue );
    void FillTextExWithOpt( size_t optArgCount, const std::wstring& text, double x, double y, JS::HandleValue options );
    JSObject* GetImageData( int32_t sx, int32_t sy, int32_t sw, int32_t sh );
    void LineTo( double x, double y );
    JSObject* MeasureText( const std::wstring& text );
    JSObject* MeasureTextEx( const std::wstring& text, JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* MeasureTextExWithOpt( size_t optArgCount, const std::wstring& text, JS::HandleValue options );
    void MoveTo( double x, double y );
    void PutImageData( ImageData* imagedata, int32_t dx, int32_t dy, int32_t dirtyX, int32_t dirtyY, int32_t dirtyWidth, int32_t dirtyHeight );
    void PutImageDataWithOpt( size_t optArgCount, ImageData* imagedata, int32_t dx, int32_t dy, int32_t dirtyX, int32_t dirtyY, int32_t dirtyWidth, int32_t dirtyHeight );
    // TODO: handle point radii
    void Reset();
    void ResetTransform();
    void Rotate( double angle );
    void RoundRect( double x, double y, double w, double h, double radii );
    void Scale( double x, double y );
    void Stroke();
    void StrokeRect( double x, double y, double w, double h );
    void StrokeText( const std::wstring& text, double x, double y );
    void Translate( double x, double y );

    /*
    SetTextRenderingHint
    */

    // TODO: implement state saving

    bool get_AntialiasingEnabled() const;
    JS::Value get_FillStyle() const;
    std::wstring get_Font() const;
    double get_GlobalAlpha() const;
    qwr::u8string get_GlobalCompositeOperation() const;
    bool get_ImageSmoothingEnabled() const;
    qwr::u8string get_ImageSmoothingQuality() const;
    qwr::u8string get_LineJoin() const;
    double get_LineWidth() const;
    JS::Value get_StrokeStyle() const;
    qwr::u8string get_TextAlign() const;
    // TODO: implement
    bool get_TextAntialiasingEnabled() const;
    qwr::u8string get_TextAntialiasingQuality() const;
    qwr::u8string get_TextBaseline() const;

    void put_AntialiasingEnabled( bool value );
    void put_FillStyle( JS::HandleValue jsValue );
    void put_Font( const std::wstring& value );
    void put_GlobalAlpha( double value );
    void put_GlobalCompositeOperation( const qwr::u8string& value );
    void put_ImageSmoothingEnabled( bool value );
    void put_ImageSmoothingQuality( const qwr::u8string& value );
    void put_LineJoin( const qwr::u8string& value );
    void put_LineWidth( double value );
    void put_StrokeStyle( JS::HandleValue jsValue );
    void put_TextAlign( const qwr::u8string& value );
    // TODO: implement
    void put_TextAntialiasingEnabled( bool value );
    void put_TextAntialiasingQuality( const qwr::u8string& value );
    void put_TextBaseline( const qwr::u8string& value );

private:
    [[nodiscard]] CanvasRenderingContext2D_Qwr( JSContext* cx, JS::HandleObject jsCanvas, ICanvasSurface& surface );

    void MaybeInitializeMatrix();
    void ResetMatrix();

    void DrawImageImpl( JS::HandleValue image,
                        double& dx, double dy,
                        const std::optional<double>& dh, const std::optional<double>& dw,
                        const std::optional<double>& sx, const std::optional<double>& sy,
                        const std::optional<double>& sw, const std::optional<double>& sh );

    std::unique_ptr<Gdiplus::Pen> GenerateGradientStrokePen( const std::vector<Gdiplus::PointF>& drawArea );
    Gdiplus::PointF GenerateTextOriginPoint( const std::wstring& text, double x, double y );

    FillTextExOptions ParseOptions_FillTextEx( JS::HandleValue options );
    float GenerateTextOriginY_FillTextEx( const std::wstring& text, double y, double descentHeight, double lineHeight, const FillTextExOptions& options );
    std::wstring PrepareText_FillTextEx( const std::wstring& text, const FillTextExOptions& options );
    std::unique_ptr<Gdiplus::StringFormat> GenerateStringFormat_FillTextEx( const FillTextExOptions& options );
    int32_t GenerateStringFormat_GdiEx_FillTextEx( const FillTextExOptions& options );
    int32_t GenerateStringFormat_Gdi_FillTextEx( const FillTextExOptions& options );
    void DrawString_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options );
    void DrawPath_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options );
    void DrawGdiString_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options );

    TextMetrics::MetricsData MeasureString_FillTextEx( const std::wstring& text, const FillTextExOptions& options );
    TextMetrics::MetricsData MeasurePath_FillTextEx( const std::wstring& text, const FillTextExOptions& options );
    TextMetrics::MetricsData MeasureGdiString_FillTextEx( const std::wstring& text, const FillTextExOptions& options );

    static bool IsRect_FillTextEx( const FillTextExOptions& options );
    static bool IsSingleLine_FillTextEx( const FillTextExOptions& options );
    static bool IsSingleLineRect_FillTextEx( const FillTextExOptions& options );

    void DisableSmoothing();
    void SetLastSmoothingQuality();

private:
    JSContext* pJsCtx_ = nullptr;

    JS::Heap<JSObject*> jsCanvas_;

    ICanvasSurface& surface_;
    smp::not_null<Gdiplus::Graphics*> pGraphics_;
    const Gdiplus::StringFormat defaultStringFormat_;

    std::unique_ptr<Gdiplus::SolidBrush> pFillBrush_;
    std::unique_ptr<Gdiplus::Pen> pStrokePen_;
    std::unique_ptr<Gdiplus::GraphicsPath> pGraphicsPath_;
    std::unique_ptr<Gdiplus::Matrix> pMatrix_;

    double globalAlpha_ = 1.0;

    uint32_t originalFillColour_ = 0;
    uint32_t originalStrokeColour_ = 0;

    JS::Heap<JSObject*> jsFillGradient_;
    CanvasGradient_Qwr* pFillGradient_ = nullptr;

    JS::Heap<JSObject*> jsStrokeGradient_;
    CanvasGradient_Qwr* pStrokeGradient_ = nullptr;

    std::optional<Gdiplus::PointF> lastPathPosOpt_;

    smp::dom::FontDescription fontDescription_;
    TextAlign textAlign_ = TextAlign::start;
    TextBaseline textBaseline_ = TextBaseline::alphabetic;

    qwr::u8string lastSmoothingQuality_ = "low";
    // TODO: implement
    qwr::u8string lastTextAntialisingQuality_ = "low";
};

} // namespace mozjs
