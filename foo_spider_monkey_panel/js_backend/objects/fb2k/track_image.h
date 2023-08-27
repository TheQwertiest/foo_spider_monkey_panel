#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/window/image.h>

#include <js/TypeDecls.h>

namespace smp
{

struct LoadedImage;

}

namespace mozjs
{

class Track;
class TrackImage;

template <>
struct JsObjectTraits<TrackImage>
{
    using ParentJsType = Image;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class TrackImage final
    : public JsObjectBase<TrackImage>
    , private Image
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackImage );

public:
    static const std::unordered_map<qwr::u8string, const GUID> kImageTypeToGuid;

public:
    ~TrackImage() override;

    [[nodiscard]] static std::unique_ptr<TrackImage> CreateNative( JSContext* cx,
                                                                   metadb_handle_ptr handle,
                                                                   smp::not_null_shared<const smp::LoadedImage> pLoadedImage,
                                                                   const qwr::u8string& imageType,
                                                                   const std::optional<qwr::u8string>& imagePathOpt,
                                                                   const qwr::u8string& src );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    /// @throw qwr::QwrException
    [[nodiscard]] static JSObject* LoadImage(
        JSContext* cx,
        smp::not_null<Track*> track,
        const qwr::u8string& imageType = "front-cover",
        JS::HandleValue options = JS::UndefinedHandleValue );

public:
    std::optional<qwr::u8string> get_ImagePath() const;
    JSObject* get_Track() const;
    qwr::u8string get_Type() const;
    void put_Src( JS::HandleObject jsSelf, const std::wstring& value ) final;

private:
    [[nodiscard]] TrackImage( JSContext* cx,
                              metadb_handle_ptr handle,
                              smp::not_null_shared<const smp::LoadedImage> pLoadedImage,
                              const qwr::u8string& imageType,
                              const std::optional<qwr::u8string>& imagePathOpt,
                              const qwr::u8string& src );

private:
    JSContext* pJsCtx_ = nullptr;
    HWND hPanelWnd_ = nullptr;

    const qwr::u8string imageType_;
    const std::optional<qwr::u8string> imagePathOpt_;
    metadb_handle_ptr handle_;
    mutable JS::Heap<JSObject*> jsTrack_;
};

} // namespace mozjs
