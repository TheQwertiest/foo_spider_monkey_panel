#pragma once

#include <dom/font_description.h>
#include <js_backend/objects/core/object_base.h>
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
    CreateNative( JSContext* cx, Gdiplus::Graphics& graphics );
    [[nodiscard]] size_t GetInternalSize() const;

    void Reinitialize( Gdiplus::Graphics& graphics );

public:
    void BeginPath();
    JSObject* CreateLinearGradient( double x0, double y0, double x1, double y1 );
    void Ellipse( double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise = false );
    void EllipseWithOpt( size_t optArgCount, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise );
    void Fill( const qwr::u8string& fillRule = "nonzero" );
    void FillWithOpt( size_t optArgCount, const qwr::u8string& fillRule );
    void FillRect( double x, double y, double w, double h );
    void FillText( const std::wstring& text, double x, double y );
    void FillTextEx( const std::wstring& text, double x, double y, JS::HandleValue options = JS::UndefinedHandleValue );
    void FillTextExWithOpt( size_t optArgCount, const std::wstring& text, double x, double y, JS::HandleValue options );
    void LineTo( double x, double y );
    JSObject* MeasureText( const std::wstring& text );
    JSObject* MeasureTextEx( const std::wstring& text, JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* MeasureTextExWithOpt( size_t optArgCount, const std::wstring& text, JS::HandleValue options );
    void MoveTo( double x, double y );
    // TODO: handle point radii
    void RoundRect( double x, double y, double w, double h, double radii );
    void Stroke();
    void StrokeRect( double x, double y, double w, double h );
    void StrokeText( const std::wstring& text, double x, double y );

    /*
    GdiDrawBitmap
    DrawImage

    GdiAlphaBlend
    ApplyAlpha( uint8_t alpha );
    ApplyMask( JsGdiBitmap* mask );
    Clone( float x, float y, float w, float h );
    CreateRawBitmap();
    Resize( uint32_t w, uint32_t h, uint32_t interpolationMode = 0 );
    RotateFlip( uint32_t mode );

    SetInterpolationMode
    SetSmoothingMode
    SetTextRenderingHint
    */

    qwr::u8string get_GlobalCompositeOperation() const;
    JS::Value get_FillStyle( JS::HandleObject jsSelf ) const;
    std::wstring get_Font();
    double get_GlobalAlpha() const;
    qwr::u8string get_LineJoin() const;
    double get_LineWidth() const;
    JS::Value get_StrokeStyle( JS::HandleObject jsSelf ) const;
    qwr::u8string get_TextAlign() const;
    qwr::u8string get_TextBaseline() const;

    void put_GlobalCompositeOperation( const qwr::u8string& value );
    void put_FillStyle( JS::HandleObject jsSelf, JS::HandleValue jsValue );
    void put_Font( const std::wstring& value );
    void put_GlobalAlpha( double value );
    void put_LineJoin( const qwr::u8string& value );
    void put_LineWidth( double value );
    void put_StrokeStyle( JS::HandleObject jsSelf, JS::HandleValue jsValue );
    void put_TextAlign( const qwr::u8string& value );
    void put_TextBaseline( const qwr::u8string& value );

private:
    [[nodiscard]] CanvasRenderingContext2D_Qwr( JSContext* cx, Gdiplus::Graphics& graphics );

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

private:
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<Gdiplus::Graphics*> pGraphics_;
    const Gdiplus::StringFormat defaultStringFormat_;

    std::unique_ptr<Gdiplus::SolidBrush> pFillBrush_;
    std::unique_ptr<Gdiplus::Pen> pStrokePen_;
    std::unique_ptr<Gdiplus::GraphicsPath> pGraphicsPath_;

    double globalAlpha_ = 1.0;

    uint32_t originalFillColour_ = 0;
    uint32_t originalStrokeColour_ = 0;

    CanvasGradient_Qwr* pFillGradient_ = nullptr;
    CanvasGradient_Qwr* pStrokeGradient_ = nullptr;

    std::optional<Gdiplus::PointF> lastPathPosOpt_;

    smp::dom::FontDescription fontDescription_;
    TextAlign textAlign_ = TextAlign::start;
    TextBaseline textBaseline_ = TextBaseline::alphabetic;
};

} // namespace mozjs
