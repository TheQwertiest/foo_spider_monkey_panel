#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>

#include <unordered_map>
#include <vector>

namespace smp
{

class EventBase;

}

namespace mozjs
{

class JsEventTarget;

template <>
struct JsObjectTraits<JsEventTarget>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool IsExtendable = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

// TODO: rename to EventTarget
class JsEventTarget
    : public JsObjectBase<JsEventTarget>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( JsEventTarget );

public:
    ~JsEventTarget() override = default;

    static void Trace( JSTracer* trc, JSObject* obj );

    bool HasEventListener( const qwr::u8string& type );

public:
    static JSObject* Constructor( JSContext* cx );

    void AddEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener );
    // TODO: add on{EVENT_TYPE} proxy
    void DispatchEvent( JS::HandleObject self, JS::HandleValue event );

    virtual EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event );

protected:
    JsEventTarget( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();

private:
    static std::unique_ptr<JsEventTarget> CreateNative( JSContext* cx );

    void InvokeListener( JS::HandleObject currentGlobal, JS::HandleObject listener, JS::HandleValue event );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    struct HeapElement
    {
        HeapElement( JSObject* jsObject )
            : jsObject( jsObject )
        {
        }

        bool isRemoved = false;
        JS::Heap<JSObject*> jsObject;
    };

    std::unordered_map<std::string, std::vector<std::shared_ptr<HeapElement>>> typeToListeners_;

    static int64_t dispatchNestedCounter_;
};

} // namespace mozjs
