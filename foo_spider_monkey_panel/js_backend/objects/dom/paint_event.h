#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class PaintEvent;

template <>
struct JsObjectTraits<PaintEvent>
{
    using ParentJsType = JsEvent;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class PaintEvent
    : public JsObjectBase<PaintEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PaintEvent );

public:
    ~PaintEvent() override = default;

    void ResetGraphics();

public:
    JSObject* get_Graphics();

protected:
    [[nodiscard]] PaintEvent( JSContext* cx, Gdiplus::Graphics& graphics );
    [[nodiscard]] size_t GetInternalSize();

private:
    static std::unique_ptr<PaintEvent> CreateNative( JSContext* cx, Gdiplus::Graphics& graphics );

private:
    JSContext* pJsCtx_ = nullptr;

    Gdiplus::Graphics* pGraphics_ = nullptr;
};

} // namespace mozjs
