#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;
class Image;
class TrackImageManager;

template <>
struct JsObjectTraits<TrackImageManager>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class TrackImageManager
    : public JsObjectBase<TrackImageManager>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackImageManager );

public:
    ~TrackImageManager() override;

    [[nodiscard]] static std::unique_ptr<TrackImageManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    JSObject* EmbedImage( JS::HandleValue tracks, smp::not_null<Image*> image, const qwr::u8string& imageType, JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* EmbedImageWithOpt( size_t optArgCount, JS::HandleValue tracks, smp::not_null<Image*> image, const qwr::u8string& imageType, JS::HandleValue options );
    JSObject* LoadImage( smp::not_null<Track*> track, const qwr::u8string& imageType, JS::HandleValue options );
    JSObject* LoadImageWithOpt( size_t optArgCount, smp::not_null<Track*> track, const qwr::u8string& imageType, JS::HandleValue options );
    JSObject* UnembedImage( JS::HandleValue tracks, const qwr::u8string& imageType, JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* UnembedImageWithOpt( size_t optArgCount, JS::HandleValue tracks, const qwr::u8string& imageType, JS::HandleValue options );
    JSObject* UnembedAllImages( JS::HandleValue tracks, JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* UnembedAllImagesWithOpt( size_t optArgCount, JS::HandleValue tracks, JS::HandleValue options );

private:
    [[nodiscard]] TrackImageManager( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
