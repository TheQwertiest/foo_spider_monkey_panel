#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class TrackList;
class TrackList_Iterator;

template <>
struct JsObjectTraits<TrackList_Iterator>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JsPrototypeId PrototypeId;
};

class TrackList_Iterator final
    : public JsObjectBase<TrackList_Iterator>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackList_Iterator );

public:
    ~TrackList_Iterator() override;

    static std::unique_ptr<TrackList_Iterator> CreateNative( JSContext* cx, JS::HandleObject trackList );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

public:
    JSObject* Next();

private:
    TrackList_Iterator( JSContext* cx, JS::HandleObject trackList );

private:
    JSContext* pJsCtx_ = nullptr;
    JS::Heap<JSObject*> jsParent_;
    smp::not_null<TrackList*> pTrackList_;

    JS::Heap<JSObject*> jsNext_;

    uint32_t curPosition_ = 0;
};

} // namespace mozjs
