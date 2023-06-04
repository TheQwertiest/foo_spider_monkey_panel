#include <stdafx.h>

#include "image_bitmap.h"

#include <dom/axis_adjuster.h>
#include <graphics/gdiplus/bitmap_generator.h>
#include <graphics/image_loader.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/window/image.h>
#include <js_backend/utils/js_hwnd_helpers.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_runnable_event.h>
#include <utils/gdi_error_helpers.h>
#include <utils/thread_pool_instance.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace
{

class BitmapExtractThreadTask
{
public:
    [[nodiscard]] BitmapExtractThreadTask( JSContext* cx,
                                           JS::HandleObject jsTarget,
                                           HWND hPanelWnd,
                                           Gdiplus::Rect srcRect,
                                           std::unique_ptr<Gdiplus::Bitmap> pBitmap );
    [[nodiscard]] BitmapExtractThreadTask( JSContext* cx,
                                           JS::HandleObject jsTarget,
                                           HWND hPanelWnd,
                                           Gdiplus::Rect srcRect,
                                           std::shared_ptr<const smp::graphics::LoadedImage> pLoadedImage );

    ~BitmapExtractThreadTask() = default;

    BitmapExtractThreadTask( const BitmapExtractThreadTask& ) = delete;
    BitmapExtractThreadTask& operator=( const BitmapExtractThreadTask& ) = delete;

    void Run();

private:
    JSContext* pJsCtx_ = nullptr;
    std::shared_ptr<mozjs::HeapHelper> pHeapHelper_;
    const uint32_t jsTargetId_;

    HWND hPanelWnd_ = nullptr;

    Gdiplus::Rect srcRect_{};
    std::unique_ptr<Gdiplus::Bitmap> pBitmap_;
    std::shared_ptr<const smp::graphics::LoadedImage> pLoadedImage_;
};

class BitmapExtractEvent : public smp::JsRunnableEvent
{
public:
    [[nodiscard]] BitmapExtractEvent( std::unique_ptr<Gdiplus::Bitmap> pBitmap,
                                      JSContext* cx,
                                      std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                      uint32_t jsTargetId );

    [[nodiscard]] BitmapExtractEvent( std::exception_ptr pException,
                                      JSContext* cx,
                                      std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                      uint32_t jsTargetId );

    void RunJs() final;

private:
    JSContext* pJsCtx_ = nullptr;
    std::unique_ptr<Gdiplus::Bitmap> pBitmap_;
    std::exception_ptr pException_;
};

} // namespace

namespace
{

BitmapExtractThreadTask::BitmapExtractThreadTask( JSContext* cx,
                                                  JS::HandleObject jsTarget,
                                                  HWND hPanelWnd,
                                                  Gdiplus::Rect srcRect,
                                                  std::unique_ptr<Gdiplus::Bitmap> pBitmap )
    : pJsCtx_( cx )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( cx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
    , hPanelWnd_( hPanelWnd )
    , srcRect_( srcRect )
    , pBitmap_( std::move( pBitmap ) )
{
    assert( pJsCtx_ );
    assert( pBitmap_ );
}

BitmapExtractThreadTask::BitmapExtractThreadTask( JSContext* cx,
                                                  JS::HandleObject jsTarget,
                                                  HWND hPanelWnd,
                                                  Gdiplus::Rect srcRect,
                                                  std::shared_ptr<const smp::graphics::LoadedImage> pLoadedImage )
    : pJsCtx_( cx )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( cx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
    , hPanelWnd_( hPanelWnd )
    , srcRect_( srcRect )
    , pLoadedImage_( pLoadedImage )
{
    assert( pJsCtx_ );
    assert( pLoadedImage_ );
}

void BitmapExtractThreadTask::Run()
{
    try
    {
        assert( pLoadedImage_ || pBitmap_ );
        if ( pLoadedImage_ )
        {
            auto pDecodedImage = smp::graphics::DecodeImage( *pLoadedImage_ );
            pBitmap_ = smp::graphics::GenerateGdiBitmap( pDecodedImage );
        }

        const auto bitmapW = static_cast<int32_t>( pBitmap_->GetWidth() );
        const auto bitmapH = static_cast<int32_t>( pBitmap_->GetHeight() );

        smp::dom::AdjustAxis( srcRect_.X, srcRect_.Width );
        smp::dom::AdjustAxis( srcRect_.Y, srcRect_.Height );

        if ( !srcRect_.Equals( { 0, 0, bitmapW, bitmapH } ) )
        { // TODO: handle options
            Gdiplus::Rect dstRect{ 0, 0, srcRect_.Width - srcRect_.X, srcRect_.Height - srcRect_.Y };
            auto pBitmap = std::make_unique<Gdiplus::Bitmap>( dstRect.Width, dstRect.Height );
            smp::error::CheckGdiPlusObject( pBitmap );

            std::unique_ptr<Gdiplus::Graphics> pGraphics( Gdiplus::Graphics::FromImage( pBitmap.get() ) );
            smp::error::CheckGdiPlusObject( pGraphics );

            auto gdiRet = pGraphics->DrawImage( pBitmap_.get(), dstRect, srcRect_.X, srcRect_.Y, srcRect_.Width, srcRect_.Height, Gdiplus::UnitPixel );
            smp::error::CheckGdi( gdiRet, "DrawImage" );

            pBitmap_ = std::move( pBitmap );
        }

        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<BitmapExtractEvent>(
                                                  std::move( pBitmap_ ),
                                                  pJsCtx_,
                                                  pHeapHelper_,
                                                  jsTargetId_ ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<BitmapExtractEvent>(
                                                  std::current_exception(),
                                                  pJsCtx_,
                                                  pHeapHelper_,
                                                  jsTargetId_ ) );
    }
}

BitmapExtractEvent::BitmapExtractEvent( std::unique_ptr<Gdiplus::Bitmap> pBitmap,
                                        JSContext* cx,
                                        std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                        uint32_t jsTargetId )
    : smp::JsRunnableEvent( cx, pHeapHelper, jsTargetId )
    , pJsCtx_( cx )
    , pBitmap_( std::move( pBitmap ) )
{
}

BitmapExtractEvent::BitmapExtractEvent( std::exception_ptr pException,
                                        JSContext* cx,
                                        std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                        uint32_t jsTargetId )
    : smp::JsRunnableEvent( cx, pHeapHelper, jsTargetId )
    , pJsCtx_( cx )
    , pException_( pException )
{
}

void BitmapExtractEvent::RunJs()
{
    assert( JS::GetCurrentRealmOrNull( pJsCtx_ ) );
    JS::RootedObject jsPromise( pJsCtx_, GetJsTarget() );

    try
    {
        if ( pException_ )
        {
            std::rethrow_exception( pException_ );
        }

        JS::RootedObject jsResult( pJsCtx_, mozjs::ImageBitmap::CreateJs( pJsCtx_, std::move( pBitmap_ ) ) );
        JS::RootedValue jsResultValue( pJsCtx_, JS::ObjectValue( *jsResult ) );
        (void)JS::ResolvePromise( pJsCtx_, jsPromise, jsResultValue );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );

        JS::RootedValue jsError( pJsCtx_ );
        (void)JS_GetPendingException( pJsCtx_, &jsError );

        (void)JS::RejectPromise( pJsCtx_, jsPromise, jsError );
    }
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
    ImageBitmap::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ImageBitmap",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::ImageBitmap );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ImageBitmap>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<ImageBitmap>::JsProperties = jsProperties.data();
const JSFunctionSpec* JsObjectTraits<ImageBitmap>::JsFunctions = jsFunctions.data();
const JsPrototypeId JsObjectTraits<ImageBitmap>::PrototypeId = JsPrototypeId::New_ImageBitmap;

ImageBitmap::ImageBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> pBitmap )
    : pJsCtx_( cx )
    , pBitmap_( std::move( pBitmap ) )
{
    assert( pBitmap_ );
}

ImageBitmap::~ImageBitmap()
{
}

std::unique_ptr<ImageBitmap>
ImageBitmap::CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> pBitmap )
{
    return std::unique_ptr<ImageBitmap>( new ImageBitmap( cx, std::move( pBitmap ) ) );
}

size_t ImageBitmap::GetInternalSize() const
{
    return 0;
}

JSObject* ImageBitmap::CreateImageBitmap1( JSContext* cx, JS::HandleValue image, JS::HandleValue options )
{
    return CreateImageBitmapImpl( cx, image, 0, 0, {}, {}, options );
}

JSObject* ImageBitmap::CreateImageBitmap2( JSContext* cx, JS::HandleValue image, int32_t sx, int32_t sy, int32_t sw, int32_t sh, JS::HandleValue options )
{
    return CreateImageBitmapImpl( cx, image, sx, sy, sw, sh, options );
}

Gdiplus::Bitmap* ImageBitmap::GetBitmap()
{
    return pBitmap_.get();
}

void ImageBitmap::Close()
{
    pBitmap_.reset();
}

JSObject* ImageBitmap::CreateImageBitmapImpl( JSContext* cx, JS::HandleValue image, int32_t sx, int32_t sy, std::optional<int32_t> sw, std::optional<int32_t> sh, JS::HandleValue options )
{
    JS::RootedObject jsPromise( cx, JS::NewPromiseObject( cx, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    try
    {
        qwr::QwrException::ExpectTrue( !sw || *sw, "The crop rect width is 0" );
        qwr::QwrException::ExpectTrue( !sh || *sh, "The crop rect height is 0" );

        // An attempt was made to use an object that is not, or is no longer, usable

        qwr::QwrException::ExpectTrue( image.isObject(), "image argument is not an object" );
        JS::RootedObject jsObject( cx, &image.toObject() );

        if ( auto pImage = JsObjectBase<Image>::ExtractNative( cx, jsObject ) )
        {
            auto pLoadedImage = pImage->GetLoadedImage();
            qwr::QwrException::ExpectTrue( !!pLoadedImage, "An attempt was made to use an object that is not, or is no longer, usable" );
            qwr::QwrException::ExpectTrue( pLoadedImage->width, "The source image width is 0" );
            qwr::QwrException::ExpectTrue( pLoadedImage->height, "The source image height is 0" );

            const Gdiplus::Rect srcRect{ sx, sy, sw.value_or( pLoadedImage->width ), sw.value_or( pLoadedImage->height ) };
            auto pFetchTask = std::make_shared<BitmapExtractThreadTask>( cx, jsPromise, GetPanelHwndForCurrentGlobal( cx ), srcRect, pLoadedImage );
            smp::GetThreadPoolInstance().AddTask( [pFetchTask] { pFetchTask->Run(); } );
        }
        else
        {
            throw qwr::QwrException( "image argument can't be converted to supported object type" );
        }
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );

        JS::RootedValue jsError( cx );
        (void)JS_GetPendingException( cx, &jsError );

        JS::RejectPromise( cx, jsPromise, jsError );
    }

    return jsPromise;
}

} // namespace mozjs
