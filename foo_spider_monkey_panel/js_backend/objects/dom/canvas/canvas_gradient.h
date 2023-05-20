#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

#include <vector>

namespace mozjs
{

class CanvasGradient_Qwr;

template <>
struct JsObjectTraits<CanvasGradient_Qwr>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class CanvasGradient_Qwr
    : public JsObjectBase<CanvasGradient_Qwr>
{
public:
    struct GradientData
    {
        Gdiplus::PointF p0;
        Gdiplus::PointF p1;
        std::vector<Gdiplus::Color> presetColors;
        std::vector<float> blendPositions;
    };

public:
    ~CanvasGradient_Qwr() override;

    [[nodiscard]] static std::unique_ptr<CanvasGradient_Qwr> CreateNative( JSContext* cx, double x0, double y0, double x1, double y1 );
    [[nodiscard]] size_t GetInternalSize() const;

    const GradientData& GetGradientData() const;

public:
    void AddColorStop( double offset, const qwr::u8string& color );

private:
    [[nodiscard]] CanvasGradient_Qwr( JSContext* cx, double x0, double y0, double x1, double y1 );

private:
    JSContext* pJsCtx_ = nullptr;

    GradientData gradientData_;
};

} // namespace mozjs
