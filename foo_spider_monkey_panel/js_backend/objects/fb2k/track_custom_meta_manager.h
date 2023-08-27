#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;
class TrackCustomMetaManager;

template <>
struct JsObjectTraits<TrackCustomMetaManager>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class TrackCustomMetaManager
    : public JsObjectBase<TrackCustomMetaManager>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackCustomMetaManager );

public:
    ~TrackCustomMetaManager() override;

    [[nodiscard]] static std::unique_ptr<TrackCustomMetaManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    // TODO: add transaction for multiple tracks handling
    JS::Value GetField( smp::not_null<Track*> track, const qwr::u8string& name ) const;
    void RefreshMeta( JS::HandleValue tracks ) const;
    void SetField( smp::not_null<Track*> track, const qwr::u8string& name, JS::HandleValue value );

private:
    [[nodiscard]] TrackCustomMetaManager( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
