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
public:
    ~CanvasRenderingContext2D_Qwr() override;

    [[nodiscard]] static std::unique_ptr<CanvasRenderingContext2D_Qwr>
    CreateNative( JSContext* cx, Gdiplus::Graphics& graphics );
    [[nodiscard]] size_t GetInternalSize() const;

    void Reinitialize( Gdiplus::Graphics& graphics );

public:
    JSObject* CreateLinearGradient( double x0, double y0, double x1, double y1 );
    void FillRect( double x, double y, double w, double h );
    void StrokeRect( double x, double y, double w, double h );

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

    qwr::u8string get_GlobalCompositeOperation() const;
    JS::Value get_FillStyle( JS::HandleObject jsSelf ) const;
    double get_GlobalAlpha() const;
    qwr::u8string get_LineJoin() const;
    double get_LineWidth() const;
    JS::Value get_StrokeStyle( JS::HandleObject jsSelf ) const;

    // TODO: add support for other modes
    void put_GlobalCompositeOperation( const qwr::u8string& mode );
    void put_FillStyle( JS::HandleObject jsSelf, JS::HandleValue jsValue );
    void put_GlobalAlpha( double alpha );
    void put_LineJoin( const qwr::u8string& lineJoin );
    void put_LineWidth( double lineWidth );
    // TODO: add support for other style types
    void put_StrokeStyle( JS::HandleObject jsSelf, JS::HandleValue jsValue );

private:
    [[nodiscard]] CanvasRenderingContext2D_Qwr( JSContext* cx, Gdiplus::Graphics& graphics );

private:
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<Gdiplus::Graphics*> pGraphics_;

    std::unique_ptr<Gdiplus::SolidBrush> pFillBrush_;
    std::unique_ptr<Gdiplus::Pen> pStrokePen_;

    double globalAlpha_ = 1.0;
    uint32_t originalFillColour_ = 0;
    uint32_t originalStrokeColour_ = 0;
    CanvasGradient_Qwr* pFillGradient_ = nullptr;
    CanvasGradient_Qwr* pStrokeGradient_ = nullptr;
};

} // namespace mozjs
