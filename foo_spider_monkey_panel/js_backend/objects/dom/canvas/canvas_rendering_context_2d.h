#pragma once

#include <dom/font_description.h>
#include <js_backend/objects/core/object_base.h>
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
        bool shouldWrapText = true;
        // visible, clip, ???
        qwr::u8string overflow = "clip";
        // normal, nowrap, pre, pre-wrap
        qwr::u8string whiteSpace = "normal";
        // clip, ellipsis, clip-char, ellipsis-char, ellipsis-path
        qwr::u8string textOverflow = "clip";
        // fast, stroke-compat
        qwr::u8string renderMode = "alpha-support";
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
    // TODO: Implement additional args (forgot to)
    void Fill();
    void FillRect( double x, double y, double w, double h );
    void FillText( const std::wstring& text, double x, double y );
    void FillTextEx( const std::wstring& text, double x, double y, JS::HandleValue options = JS::UndefinedHandleValue );
    void FillTextExWithOpt( size_t optArgCount, const std::wstring& text, double x, double y, JS::HandleValue options );
    void LineTo( double x, double y );
    void MoveTo( double x, double y );
    void Stroke();
    void StrokeRect( double x, double y, double w, double h );
    void StrokeText( const std::wstring& text, double x, double y );

    /*
    GdiAlphaBlend
    GdiDrawBitmap
    DrawImage

    GdiDrawText

    EstimateLineWrap
    CalcTextHeight
    CalcTextWidth
    MeasureString

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
    float GenerateTextOriginY_FillTextEx( const std::wstring& text, double y, const FillTextExOptions& options );
    std::wstring PrepareText_FillTextEx( const std::wstring& text, const FillTextExOptions& options );
    std::unique_ptr<Gdiplus::StringFormat> GenerateStringFormat_FillTextEx( const FillTextExOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<Gdiplus::Graphics*> pGraphics_;
    std::unique_ptr<Gdiplus::SolidBrush> pFillBrush_;
    std::unique_ptr<Gdiplus::Pen> pStrokePen_;
    std::unique_ptr<Gdiplus::GraphicsPath> pGraphicsPath_;
    Gdiplus::StringFormat stringFormat_;

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
