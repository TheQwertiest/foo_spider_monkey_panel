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
    DrawRect > strokeRect
    FillSolidRect > fillRect

    DrawRoundRect > strokeRect + lineCap
    FillRoundRect > fillRect + lineCap

    CalcTextHeight
    CalcTextWidth
    DrawEllipse
    DrawImage
    DrawLine
    DrawPolygon
    DrawString
    EstimateLineWrap
    FillEllipse
    FillGradRect
    FillPolygon
    GdiAlphaBlend
    GdiDrawBitmap
    GdiDrawText
    MeasureString
    SetInterpolationMode
    SetSmoothingMode
    SetTextRenderingHint
    */

    // TODO: add support for other types
    qwr::u8string get_FillStyle() const;
    qwr::u8string get_StrokeStyle() const;
    void put_FillStyle( const qwr::u8string& color );
    void put_StrokeStyle( const qwr::u8string& color );

private:
    [[nodiscard]] CanvasRenderingContext2d( JSContext* cx, Gdiplus::Graphics& graphics );

private:
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<Gdiplus::Graphics*> pGraphics_;
    std::unique_ptr<Gdiplus::SolidBrush> pFillBrush_;
    std::unique_ptr<Gdiplus::Pen> pStrokePen_;
};

} // namespace mozjs
