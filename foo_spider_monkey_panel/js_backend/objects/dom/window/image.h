#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>

#include <wincodec.h>

#include <js/TypeDecls.h>
#include <qwr/com_ptr.h>

namespace
{
class ImageFetchThreadTask;
class ImageFetchEvent;
} // namespace

namespace smp
{

class MicroTask;
class JsTargetMicroTask;

} // namespace smp

namespace smp
{

struct LoadedImage;

}

namespace mozjs
{

class Image;

template <>
struct JsObjectTraits<Image>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;
    static constexpr bool IsExtendable = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

// TODO: install proto to global
class Image
    : public JsObjectBase<Image>
    , protected JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( Image );

public:
    enum class CompleteStatus
    {
        unavailable,
        completely_available,
        broken
    };

public:
    ~Image() override;

    [[nodiscard]] static std::unique_ptr<Image> CreateNative( JSContext* cx );
    // TODO: add dynamic size
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

    /// @throw qwr::QwrException
    [[nodiscard]] static JSObject* LoadImage( JSContext* cx, JS::HandleValue value );

    /// @throw qwr::QwrException
    [[nodiscard]] qwr::ComPtr<IWICBitmap> GetDecodedBitmap() const;
    [[nodiscard]] std::shared_ptr<const smp::LoadedImage> GetLoadedImage() const;
    [[nodiscard]] CompleteStatus GetStatus() const;

public:
    static JSObject* Constructor( JSContext* cx );

    bool get_Complete() const;
    std::wstring get_CurrentSrc() const;
    uint32_t get_NaturalHeight() const;
    uint32_t get_NaturalWidth() const;
    std::wstring get_Src() const;
    virtual void put_Src( JS::HandleObject jsSelf, const std::wstring& value );

protected:
    [[nodiscard]] Image( JSContext* cx );
    [[nodiscard]] Image( JSContext* cx, std::shared_ptr<const smp::LoadedImage> pLoadedImage, const std::wstring& src );

private:
    void InitImageUpdate( const std::wstring& source );
    void UpdateImageData( JS::HandleObject jsSelf );
    void ProcessFetchEvent( const ImageFetchEvent& fetchEvent, JS::HandleObject jsSelf );
    void HandleImageLoad( const std::wstring& src, std::shared_ptr<const smp::LoadedImage> pLoadedImage, JS::HandleObject jsSelf );
    void HandleImageError( const std::wstring& src, JS::HandleObject jsSelf );

private:
    JSContext* pJsCtx_ = nullptr;
    HWND hPanelWnd_ = nullptr;

    std::wstring currentSrc_;
    CompleteStatus currentStatus_ = CompleteStatus::unavailable;

    bool isLoading_ = false;
    std::wstring pendingSrc_;
    std::filesystem::path pendingParsedSrc_;

    std::weak_ptr<smp::JsTargetMicroTask> pMicroTask_;
    std::weak_ptr<smp::EventBase> pDispatchedEvent_;
    std::weak_ptr<::ImageFetchThreadTask> pFetchTask_;

    std::shared_ptr<const smp::LoadedImage> pLoadedImage_;
    qwr::ComPtr<IWICBitmap> pDecodedImage_;
};

} // namespace mozjs
