#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class TextMetrics;

template <>
struct JsObjectTraits<TextMetrics>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class TextMetrics
    : public JsObjectBase<TextMetrics>
{
public:
    struct MetricsData
    {
        double width = 0;
        double height = 0;
        double actualBoundingBoxAscent = 0;
        double actualBoundingBoxDescent = 0;
        double actualBoundingBoxLeft = 0;
        double actualBoundingBoxRight = 0;
    };

public:
    ~TextMetrics() override;

    [[nodiscard]] static std::unique_ptr<TextMetrics> CreateNative( JSContext* cx, const MetricsData& data );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    double get_ActualBoundingBoxAscent() const;
    double get_ActualBoundingBoxDescent() const;
    double get_ActualBoundingBoxLeft() const;
    double get_ActualBoundingBoxRight() const;
    double get_Height() const;
    double get_Width() const;

private:
    [[nodiscard]] TextMetrics( JSContext* cx, const MetricsData& data );

private:
    JSContext* pJsCtx_ = nullptr;

    const MetricsData data_;
};

} // namespace mozjs
