#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class PaintEvent
    : public JsObjectBase<PaintEvent>
    , private JsEvent
{
    friend class JsObjectBase<PaintEvent>;

private:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasParentProto = true;

    using BaseJsType = JsEvent;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId BasePrototypeId;
    static const JsPrototypeId ParentPrototypeId;
    static const JsPrototypeId PrototypeId;

public:
    ~PaintEvent() override = default;

    void ResetGraphics();

public:
    JSObject* get_Graphics();

protected:
    PaintEvent( JSContext* cx, Gdiplus::Graphics& graphics );
    [[nodiscard]] size_t GetInternalSize();

private:
    static std::unique_ptr<PaintEvent> CreateNative( JSContext* cx, Gdiplus::Graphics& graphics );

private:
    JSContext* pJsCtx_ = nullptr;

    Gdiplus::Graphics* pGraphics_ = nullptr;
};

} // namespace mozjs
