#pragma once

#include <js_backend/objects/core/object_base.h>
#include <utils/not_null.h>

#include <js/TypeDecls.h>

namespace Gdiplus
{
class Brush;
class Graphics;
class Pen;
} // namespace Gdiplus

namespace mozjs
{

// I like CanvasRenderingContext2D more, but it clashes exports with <MsHTML.h> header
class CanvasRenderingContext2d;

template <>
struct JsObjectTraits<CanvasRenderingContext2d>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class CanvasRenderingContext2d
    : public JsObjectBase<CanvasRenderingContext2d>
{
public:
    ~CanvasRenderingContext2d() override;

    [[nodiscard]] static std::unique_ptr<CanvasRenderingContext2d>
    CreateNative( JSContext* cx, Gdiplus::Graphics& graphics );
    [[nodiscard]] size_t GetInternalSize();

    void Reinitialize( Gdiplus::Graphics& graphics );

public:
    void FillRect( double x, double y, double width, double height );
    void StrokeRect( double x, double y, double width, double height );

    /*
    DrawEllipse
    FillEllipse
    DrawLine
    DrawPolygon
    FillPolygon
    GdiAlphaBlend

    FillGradRect

    GdiDrawBitmap
    DrawImage

    DrawString
    GdiDrawText

    EstimateLineWrap
    CalcTextHeight
    CalcTextWidth
    MeasureString

    SetInterpolationMode
    SetSmoothingMode
    SetTextRenderingHint
    */

    qwr::u8string get_FillStyle() const;
    double get_GlobalAlpha() const;
    qwr::u8string get_LineJoin() const;
    double get_LineWidth() const;
    qwr::u8string get_StrokeStyle() const;

    void put_FillStyle( const qwr::u8string& color );
    void put_GlobalAlpha( double alpha );
    void put_LineJoin( const qwr::u8string& lineJoin );
    void put_LineWidth( double lineWidth );
    // TODO: add support for other style types
    void put_StrokeStyle( const qwr::u8string& color );

private:
    [[nodiscard]] CanvasRenderingContext2d( JSContext* cx, Gdiplus::Graphics& graphics );

private:
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<Gdiplus::Graphics*> pGraphics_;
    std::unique_ptr<Gdiplus::SolidBrush> pFillBrush_;
    std::unique_ptr<Gdiplus::Pen> pStrokePen_;

    double globalAlpha_ = 1.0;
    uint32_t originalFillColour_ = 0;
    uint32_t originalStrokeColour_ = 0;
};

} // namespace mozjs
