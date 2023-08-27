#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/canvas/native/canvas_surface.h>
#include <js_backend/objects/dom/event_target.h>
#include <panel/panel_fwd.h>
#include <tasks/events/event_id.h>

#include <optional>
#include <string>
#include <unordered_set>

namespace smp
{
class EventBase;

} // namespace smp

namespace mozjs
{
class JsGdiGraphics;
class CanvasRenderingContext2D_Qwr;
} // namespace mozjs

namespace mozjs
{

class WindowNew;

template <>
struct JsObjectTraits<WindowNew>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const PostJsCreateFn PostCreate;
};

class WindowNew
    : public JsObjectBase<WindowNew>
    , private JsEventTarget
    , public ICanvasSurface
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( WindowNew );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~WindowNew() override = default;

    static std::unique_ptr<WindowNew> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

    bool IsDevice() const final;
    Gdiplus::Graphics& GetGraphics() final;
    uint32_t GetHeight() const final;
    uint32_t GetWidth() const final;

public:
    // TODO: wrap context and disable it while there is not redraw event
    JSObject* GetContext( JS::HandleObject jsSelf, const std::wstring& contextType, JS::HandleValue attributes = JS::UndefinedHandleValue );
    JSObject* GetContextWithOpt( JS::HandleObject jsSelf, size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes );
    // TODO: add decode option
    JSObject* LoadImage( JS::HandleValue source );
    void Redraw( bool force = false );
    void RedrawWithOpt( size_t optArgCount, bool force );
    void RedrawRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force = false );
    void RedrawRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force );

public:
    uint32_t get_Height();
    uint32_t get_Width();

protected:
    [[nodiscard]] WindowNew( JSContext* cx );

private:
    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

    void HandlePaintEvent( JS::HandleObject self );

    [[nodiscard]] JSObject* GenerateEvent( const smp::EventBase& event, const qwr::u8string& eventType );

private:
    JSContext* pJsCtx_ = nullptr;
    smp::not_null_shared<smp::panel::PanelAccessor> pHostPanel_;

    JS::Heap<JSObject*> jsRenderingContext_;
    CanvasRenderingContext2D_Qwr* pNativeRenderingContext_ = nullptr;
    Gdiplus::Graphics* pGraphics_ = nullptr;
};

} // namespace mozjs
