#include <stdafx.h>

#include "image.h"

#include <graphics/image_loader.h>
#include <js_backend/engine/js_engine.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/utils/js_hwnd_helpers.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_target_event.h>
#include <tasks/micro_tasks/micro_task.h>

#include <qwr/final_action.h>
#include <qwr/utility.h>
#include <qwr/winapi_error_helpers.h>

using namespace smp;
namespace fs = std::filesystem;

namespace
{

// TODO: extract somewhere
constexpr wchar_t kFilePrefix[] = L"file://";
constexpr char kImageFetchEventType[] = "image_fetch";
constexpr char kImageLoadEventType[] = "load";
constexpr char kImageErrorEventType[] = "error";

} // namespace

namespace
{

enum class ReservedSlots
{
    kPromise = mozjs::kReservedObjectSlot + 1
};

class ImageFetchThreadTask
{
public:
    [[nodiscard]] ImageFetchThreadTask( JSContext* cx,
                                        JS::HandleObject jsTarget,
                                        HWND hPanelWnd,
                                        const fs::path& imagePath );

    ~ImageFetchThreadTask() = default;

    ImageFetchThreadTask( const ImageFetchThreadTask& ) = delete;
    ImageFetchThreadTask& operator=( const ImageFetchThreadTask& ) = delete;

    void Run();
    void Cancel();

    [[nodiscard]] const fs::path& GetImagePath() const;

private:
    JSContext* pJsCtx_ = nullptr;
    std::shared_ptr<mozjs::HeapHelper> pHeapHelper_;
    const uint32_t jsTargetId_;

    HWND hPanelWnd_ = nullptr;
    fs::path imagePath_;

    std::atomic<bool> isCanceled_ = false;
};

class ImageFetchEvent : public smp::JsTargetEvent
{
public:
    [[nodiscard]] ImageFetchEvent( const qwr::u8string& type,
                                   const fs::path& imagePath,
                                   std::unique_ptr<const smp::graphics::LoadedImage> pLoadedImage,
                                   JSContext* cx,
                                   std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                   uint32_t jsTargetId );

    [[nodiscard]] std::shared_ptr<const smp::graphics::LoadedImage> GetLoadedImage() const;
    [[nodiscard]] const fs::path& GetImagePath() const;

private:
    fs::path imagePath_;
    std::shared_ptr<const smp::graphics::LoadedImage> pLoadedImage_;
};

} // namespace

namespace
{

ImageFetchThreadTask::ImageFetchThreadTask( JSContext* cx,
                                            JS::HandleObject jsTarget,
                                            HWND hPanelWnd,
                                            const fs::path& imagePath )
    : pJsCtx_( cx )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( cx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
    , hPanelWnd_( hPanelWnd )
    , imagePath_( imagePath )
{
    assert( cx );
}

void ImageFetchThreadTask::Run()
{
    if ( isCanceled_ )
    { // the task still might be executed and posted, since we don't block here,
      // but dispatched events are canceled in Cancel(), so it won't cause a 'misfire'
        return;
    }

    try
    {
        auto hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
        qwr::error::CheckHR( hr, "CoInitializeEx" );

        qwr::final_action autoCo( [] { CoUninitialize(); } );

        auto pLoadedImage = smp::graphics::LoadImageFromFile( imagePath_ );

        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<ImageFetchEvent>(
                                                  kImageFetchEventType,
                                                  imagePath_,
                                                  std::move( pLoadedImage ),
                                                  pJsCtx_,
                                                  pHeapHelper_,
                                                  jsTargetId_ ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        // TODO: think about propagating error message
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<ImageFetchEvent>(
                                                  kImageFetchEventType,
                                                  imagePath_,
                                                  std::unique_ptr<const smp::graphics::LoadedImage>{},
                                                  pJsCtx_,
                                                  pHeapHelper_,
                                                  jsTargetId_ ) );
    }
}

void ImageFetchThreadTask::Cancel()
{
    isCanceled_ = true;
}

const fs::path& ImageFetchThreadTask::GetImagePath() const
{
    return imagePath_;
}

ImageFetchEvent::ImageFetchEvent( const qwr::u8string& type,
                                  const fs::path& imagePath,
                                  std::unique_ptr<const smp::graphics::LoadedImage> pLoadedImage,
                                  JSContext* cx,
                                  std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                  uint32_t jsTargetId )
    : smp::JsTargetEvent( type, cx, pHeapHelper, jsTargetId )
    , imagePath_( imagePath )
    , pLoadedImage_( std::move( pLoadedImage ) )
{
}

std::shared_ptr<const smp::graphics::LoadedImage> ImageFetchEvent::GetLoadedImage() const
{
    return pLoadedImage_;
}

const fs::path& ImageFetchEvent::GetImagePath() const
{
    return imagePath_;
}

bool IsFileSrc( const std::wstring& src )
{
    return src.starts_with( kFilePrefix );
}

std::wstring GenerateSrcFromPath( const fs::path& path )
{
    auto pathStr = path.wstring();
    std::replace( pathStr.begin(), pathStr.end(), '\\', '/' );
    return fmt::format( L"{}{}", kFilePrefix, pathStr );
}

/// @throw std::filesystem::error
std::optional<fs::path> GeneratePathFromSrc( const std::wstring& src )
{
    if ( !IsFileSrc( src ) )
    {
        return std::nullopt;
    }

    return fs::weakly_canonical( fs::path{ src.substr( std::size( kFilePrefix ) - 1 ) } ).wstring();
}

} // namespace

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsObjectBase<Image>::FinalizeJsObject,
    nullptr,
    nullptr,
    Image::Trace
};

JSClass jsClass = {
    "Image",
    DefaultClassFlags( 1 ),
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_complete, Image::get_Complete )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_currentSrc, Image::get_CurrentSrc )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, Image::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_naturalHeight, Image::get_NaturalHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_naturalWidth, Image::get_NaturalWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_src, Image::get_Src )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, Image::get_Width )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_height, Image::put_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( put_src, Image::put_Src )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_width, Image::put_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "complete", get_complete, kDefaultPropsFlags ),
        JS_PSG( "currentSrc", get_currentSrc, kDefaultPropsFlags ),
        JS_PSGS( "height", get_height, put_height, kDefaultPropsFlags ),
        JS_PSG( "naturalHeight", get_naturalHeight, kDefaultPropsFlags ),
        JS_PSG( "naturalWidth", get_naturalWidth, kDefaultPropsFlags ),
        JS_PSGS( "src", get_src, put_src, kDefaultPropsFlags ),
        JS_PSGS( "width", get_width, put_width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Image_Constructor, Image::Constructor, Image::ConstructorWithOpt, 2 )

MJS_VERIFY_OBJECT( mozjs::Image );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Image>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<Image>::JsProperties = jsProperties.data();
const JSFunctionSpec* JsObjectTraits<Image>::JsFunctions = jsFunctions.data();
const JsPrototypeId JsObjectTraits<Image>::PrototypeId = JsPrototypeId::New_Image;
const JSNative JsObjectTraits<Image>::JsConstructor = ::Image_Constructor;

Image::Image( JSContext* cx, uint32_t width, uint32_t height )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
    , hPanelWnd_( GetPanelHwndForCurrentGlobal( pJsCtx_ ) )
    , width_( width )
    , height_( height )
{
}

Image::~Image()
{
}

std::unique_ptr<Image>
Image::CreateNative( JSContext* cx, uint32_t width, uint32_t height )
{
    return std::unique_ptr<Image>( new Image( cx, width, height ) );
}

size_t Image::GetInternalSize() const
{
    return 0;
}

void Image::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

EventStatus Image::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    assert( event.GetId() == smp::EventId::kNew_JsTarget );
    const auto& eventType = event.GetType();
    if ( eventType == kImageFetchEventType )
    {
        status.isHandled = true;

        const auto& fetchEvent = static_cast<const ::ImageFetchEvent&>( event );
        ProcessFetchEvent( fetchEvent, self );

        return status;
    }

    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    const auto& targetEvent = static_cast<const smp::JsTargetEvent&>( event );

    JS::RootedObject jsEvent( pJsCtx_,
                              mozjs::JsEvent::CreateJs( pJsCtx_,
                                                        targetEvent.GetType(),
                                                        JsEvent::EventProperties{ .cancelable = false } ) );
    JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
    DispatchEvent( self, jsEventValue );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

JSObject* Image::LoadImage( JSContext* cx, JS::HandleValue source )
{
    if ( source.isString() )
    {
        const auto srcStr = convert::to_native::ToValue<std::wstring>( cx, source );

        JS::RootedObject jsImage( cx, JsObjectBase<Image>::CreateJs( cx, 0, 0 ) );
        auto pNative = JsObjectBase<Image>::ExtractNative( cx, jsImage );
        assert( pNative );

        JS::RootedObject jsPromise( cx, JS::NewPromiseObject( cx, nullptr ) );
        JsException::ExpectTrue( jsPromise );

        JS_SetReservedSlot( jsImage, qwr::to_underlying( ReservedSlots::kPromise ), JS::ObjectValue( *jsPromise ) );

        pNative->InitImageUpdate( srcStr );
        pNative->UpdateImageData( jsImage );

        return jsPromise;
    }
    else
    {
        // TODO: add other supported types
        throw qwr::QwrException( "source argument can't be converted to supported object type" );
    }
}

qwr::ComPtr<IWICBitmap> Image::GetDecodedBitmap() const
{
    if ( !pLoadedImage_ )
    {
        return nullptr;
    }

    // TODO: add decode() handling
    return ( pDecodedImage_ ? pDecodedImage_ : smp::graphics::DecodeImage( *pLoadedImage_ ) );
}

std::shared_ptr<const smp::graphics::LoadedImage> Image::GetLoadedImage() const
{
    return pLoadedImage_;
}

Image::CompleteStatus Image::GetStatus() const
{
    return currentStatus_;
}

JSObject* Image::Constructor( JSContext* cx, uint32_t width, uint32_t height )
{
    return JsObjectBase<Image>::CreateJs( cx, width, height );
}

JSObject* Image::ConstructorWithOpt( JSContext* cx, size_t optArgCount, uint32_t width, uint32_t height )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, width, height );
    case 1:
        return Constructor( cx, width );
    case 2:
        return Constructor( cx );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool Image::get_Complete() const
{
    return !isLoading_;
}

std::wstring Image::get_CurrentSrc() const
{
    return currentSrc_;
}

uint32_t Image::get_Height() const
{
    return height_;
}

uint32_t Image::get_NaturalHeight() const
{
    return ( pLoadedImage_ ? pLoadedImage_->height : 0 );
}

uint32_t Image::get_NaturalWidth() const
{
    return ( pLoadedImage_ ? pLoadedImage_->width : 0 );
}

std::wstring Image::get_Src() const
{
    return pendingSrc_;
}

uint32_t Image::get_Width() const
{
    return width_;
}

void Image::put_Height( uint32_t value )
{
    height_ = value;
}

void Image::put_Src( JS::HandleObject jsSelf, const std::wstring& value )
{
    InitImageUpdate( value );

    if ( auto pLockedMicroTask = pMicroTask_.lock(); !pLockedMicroTask )
    {
        auto pMicroTask = std::make_shared<MicroTask>( pJsCtx_, jsSelf, []( JSContext* cx, JS::HandleObject jsSelf ) {
            auto pSelf = JsObjectBase<mozjs::Image>::ExtractNative( cx, jsSelf );
            assert( pSelf );

            pSelf->UpdateImageData( jsSelf );
        } );

        pMicroTask_ = pMicroTask;
        EventDispatcher::Get().PutMicroTask( hPanelWnd_, pMicroTask );
    }
}

void Image::put_Width( uint32_t value )
{
    width_ = value;
}

void Image::InitImageUpdate( const std::wstring& source )
{
    pendingSrc_ = source;
    isLoading_ = true;
}

void Image::UpdateImageData( JS::HandleObject jsSelf )
{
    pMicroTask_.reset();

    auto selectedSrc = pendingSrc_;

    const auto cancelAndReset = []( auto& ptr ) {
        if ( auto pLockedPtr = ptr.lock(); pLockedPtr )
        {
            pLockedPtr->Cancel();
            ptr.reset();
        }
    };

    cancelAndReset( pDispatchedEvent_ );

    if ( selectedSrc.empty() )
    {
        cancelAndReset( pFetchTask_ );
        HandleImageLoad( selectedSrc, nullptr, jsSelf );
    }

    const auto parsedSrcOpt = [&]() -> std::optional<fs::path> {
        try
        {
            return GeneratePathFromSrc( selectedSrc );
        }
        catch ( const fs::filesystem_error& /*e*/ )
        {
            return std::nullopt;
        }
    }();

    if ( !parsedSrcOpt )
    {
        cancelAndReset( pFetchTask_ );
        HandleImageError( selectedSrc, jsSelf );

        return;
    }
    const auto& parsedSrc = *parsedSrcOpt;

    // TODO: implement cache
    /*
    if ( cache_.has( parsedSrc ) )
    {
        cancelAndReset( pFetchTask_ );
        HandleImageLoad( GenerateSrcFromPath(parsedSrc), cache_.at( parsedSrc ) );

        return;
    }
    */

    if ( auto pLockedFetchTask = pFetchTask_.lock();
         pLockedFetchTask && pLockedFetchTask->GetImagePath() == parsedSrc )
    {
        return;
    }

    cancelAndReset( pFetchTask_ );

    auto pFetchTask = std::make_shared<ImageFetchThreadTask>( pJsCtx_, jsSelf, hPanelWnd_, parsedSrc );
    pFetchTask_ = pFetchTask;
    pendingParsedSrc_ = parsedSrc;

    fb2k::inCpuWorkerThread( [pFetchTask] { pFetchTask->Run(); } );
}

void Image::ProcessFetchEvent( const ImageFetchEvent& fetchEvent, JS::HandleObject jsSelf )
{
    if ( pendingParsedSrc_ != fetchEvent.GetImagePath() )
    {
        return;
    }
    pFetchTask_.reset();

    const auto src = GenerateSrcFromPath( pendingParsedSrc_ );
    if ( const auto pLoadedImage = fetchEvent.GetLoadedImage() )
    {
        HandleImageLoad( src, pLoadedImage, jsSelf );
    }
    else
    {
        HandleImageError( src, jsSelf );
    }
}

void Image::HandleImageLoad( const std::wstring& src, std::shared_ptr<const smp::graphics::LoadedImage> pLoadedImage, JS::HandleObject jsSelf )
{
    JS::RootedValue jsPromiseValue( pJsCtx_, JS::GetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kPromise ) ) );
    const auto isLoadedAsPromise = jsPromiseValue.isObject();

    isLoading_ = false;
    currentSrc_ = src;
    if ( pLoadedImage )
    {
        currentStatus_ = CompleteStatus::completely_available;
        pLoadedImage_ = pLoadedImage;

        if ( !isLoadedAsPromise )
        {
            auto pEvent = std::make_shared<smp::JsTargetEvent>( kImageLoadEventType, pJsCtx_, jsSelf );
            pDispatchedEvent_ = pEvent;
            smp::EventDispatcher::Get().PutEvent( hPanelWnd_, pEvent );
        }
    }
    else
    {
        currentStatus_ = CompleteStatus::unavailable;
        pLoadedImage_.reset();
    }

    if ( isLoadedAsPromise )
    {
        JS::RootedObject jsPromiseObject( pJsCtx_, &jsPromiseValue.toObject() );
        JS::RootedValue jsResultValue( pJsCtx_, JS::ObjectValue( *jsSelf ) );
        JS::SetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kPromise ), JS::UndefinedValue() );

        (void)JS::ResolvePromise( pJsCtx_, jsPromiseObject, jsResultValue );
    }
}

void Image::HandleImageError( const std::wstring& src, JS::HandleObject jsSelf )
{
    JS::RootedValue jsPromiseValue( pJsCtx_, JS::GetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kPromise ) ) );
    const auto isLoadedAsPromise = jsPromiseValue.isObject();

    isLoading_ = false;
    currentSrc_ = src;
    currentStatus_ = CompleteStatus::broken;
    pLoadedImage_.reset();

    if ( !isLoadedAsPromise )
    {
        // TODO: think about adding message to error
        auto pEvent = std::make_shared<smp::JsTargetEvent>( kImageErrorEventType, pJsCtx_, jsSelf );
        pDispatchedEvent_ = pEvent;
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_, pEvent );
    }
    else
    {
        JS::RootedObject jsPromiseObject( pJsCtx_, &jsPromiseValue.toObject() );
        JS::RootedValue jsResultValue( pJsCtx_, JS::ObjectValue( *jsSelf ) );
        JS::SetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kPromise ), JS::UndefinedValue() );

        (void)JS::RejectPromise( pJsCtx_, jsPromiseObject, JS::UndefinedHandleValue );
    }
}

} // namespace mozjs
