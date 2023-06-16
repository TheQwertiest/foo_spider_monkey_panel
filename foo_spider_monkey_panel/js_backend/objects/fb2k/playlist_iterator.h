#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Playlist;
class Playlist_Iterator;

template <>
struct JsObjectTraits<Playlist_Iterator>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JsPrototypeId PrototypeId;
};

class Playlist_Iterator
    : public JsObjectBase<Playlist_Iterator>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( Playlist_Iterator );

public:
    ~Playlist_Iterator() override;

    static std::unique_ptr<Playlist_Iterator> CreateNative( JSContext* cx, JS::HandleObject Playlist );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

public:
    JSObject* Next();

private:
    Playlist_Iterator( JSContext* cx, JS::HandleObject Playlist );

private:
    JSContext* pJsCtx_ = nullptr;
    JS::Heap<JSObject*> jsParent_;
    smp::not_null<Playlist*> pPlaylist_;

    JS::Heap<JSObject*> jsNext_;

    uint32_t curPosition_ = 0;
};

} // namespace mozjs
