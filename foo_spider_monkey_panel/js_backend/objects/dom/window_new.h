#pragma once

#include <events/event.h>
#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>

#include <optional>
#include <string>
#include <unordered_set>

namespace smp
{
class EventBase;
}

namespace mozjs
{
class JsGdiGraphics;
}

namespace mozjs
{

class WindowNew
    : public JsObjectBase<WindowNew>
    , private JsEventTarget
{
    friend class JsObjectBase<WindowNew>;

private:
    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    using BaseJsType = JsEventTarget;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId BasePrototypeId;
    static const JsPrototypeId ParentPrototypeId;

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~WindowNew() override = default;

    static std::unique_ptr<WindowNew> CreateNative( JSContext* cx, smp::panel::PanelWindow& parentPanel );

    static void Trace( JSTracer* trc, JSObject* obj );
    void PrepareForGc();

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event );

public:
    void Repaint( bool force = false );
    void RepaintWithOpt( size_t optArgCount, bool force );
    void RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force = false );
    void RepaintRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force );

public:
    uint32_t get_Height();
    uint32_t get_Width();

protected:
    WindowNew( JSContext* cx, smp::panel::PanelWindow& parentPanel );

    [[nodiscard]] size_t GetInternalSize();

private:
    const std::string& EventIdToType( smp::EventId eventId );
    void HandlePaintEvent( JS::HandleObject self );

private:
    JSContext* pJsCtx_ = nullptr;
    smp::panel::PanelWindow& parentPanel_;

    bool isFinalized_ = false; ///< if true, then parentPanel_ might be already inaccessible
};

} // namespace mozjs
