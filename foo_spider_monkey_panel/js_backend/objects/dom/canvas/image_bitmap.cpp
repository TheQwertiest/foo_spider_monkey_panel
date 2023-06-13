#include <stdafx.h>

#include "image_bitmap.h"

#include <dom/axis_adjuster.h>
#include <graphics/gdiplus/bitmap_generator.h>
#include <graphics/gdiplus/error_handler.h>
#include <graphics/image_decoder.h>
#include <graphics/loaded_image.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas.h>
#include <js_backend/objects/dom/window/image.h>
#include <js_backend/utils/js_property_helper.h>
#include <js_backend/utils/panel_from_global.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_runnable_event.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace
{

class BitmapExtractThreadTask
{
public:
    [[nodiscard]] BitmapExtractThreadTask( std::unique_ptr<Gdiplus::Image> pImage,
                                           Gdiplus::Rect srcRect,
                                           const mozjs::ImageBitmap::CreateBitmapOptions& options,
                                           JSContext* cx,
                                           JS::HandleObject jsTarget,
                                           HWND hPanelWnd );
    [[nodiscard]] BitmapExtractThreadTask( std::shared_ptr<const smp::LoadedImage> pLoadedImage,
                                           Gdiplus::Rect srcRect,
                                           const mozjs::ImageBitmap::CreateBitmapOptions& options,
                                           JSContext* cx,
                                           JS::HandleObject jsTarget,
                                           HWND hPanelWnd );

    ~BitmapExtractThreadTask() = default;

    BitmapExtractThreadTask( const BitmapExtractThreadTask& ) = delete;
    BitmapExtractThreadTask& operator=( const BitmapExtractThreadTask& ) = delete;

    void Run();

private:
    JSContext* pJsCtx_ = nullptr;
    std::shared_ptr<mozjs::HeapHelper> pHeapHelper_;
    const uint32_t jsTargetId_;

    HWND hPanelWnd_ = nullptr;

    const Gdiplus::Rect srcRect_{};
    const mozjs::ImageBitmap::CreateBitmapOptions options_;

    std::unique_ptr<Gdiplus::Image> pImage_;
    std::shared_ptr<const smp::LoadedImage> pLoadedImage_;
};

class BitmapExtractEvent : public smp::JsRunnableEvent
{
public:
    [[nodiscard]] BitmapExtractEvent( std::unique_ptr<Gdiplus::Image> pBitmap,
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
    std::unique_ptr<Gdiplus::Image> pImage_;
    std::exception_ptr pException_;
};

} // namespace

namespace
{

BitmapExtractThreadTask::BitmapExtractThreadTask( std::unique_ptr<Gdiplus::Image> pImage,
                                                  Gdiplus::Rect srcRect,
                                                  const mozjs::ImageBitmap::CreateBitmapOptions& options,
                                                  JSContext* cx,
                                                  JS::HandleObject jsTarget,
                                                  HWND hPanelWnd )
    : pJsCtx_( cx )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( cx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
    , hPanelWnd_( hPanelWnd )
    , srcRect_( srcRect )
    , options_( options )
    , pImage_( std::move( pImage ) )
{
    assert( pJsCtx_ );
    assert( pImage_ );
}

BitmapExtractThreadTask::BitmapExtractThreadTask( std::shared_ptr<const smp::LoadedImage> pLoadedImage,
                                                  Gdiplus::Rect srcRect,
                                                  const mozjs::ImageBitmap::CreateBitmapOptions& options,
                                                  JSContext* cx,
                                                  JS::HandleObject jsTarget,
                                                  HWND hPanelWnd )
    : pJsCtx_( cx )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( cx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
    , hPanelWnd_( hPanelWnd )
    , srcRect_( srcRect )
    , options_( options )
    , pLoadedImage_( pLoadedImage )
{
    assert( pJsCtx_ );
    assert( pLoadedImage_ );
}

void BitmapExtractThreadTask::Run()
{
    try
    {
        assert( pLoadedImage_ || pImage_ );
        if ( pLoadedImage_ )
        {
            auto hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
            qwr::error::CheckHR( hr, "CoInitializeEx" );

            qwr::final_action autoCo( [] { CoUninitialize(); } );

            auto pDecodedImage = smp::DecodeImage( *pLoadedImage_ );
            pImage_ = smp::GenerateGdiBitmap( pDecodedImage );
        }

        const auto imageW = static_cast<int32_t>( pImage_->GetWidth() );
        const auto imageH = static_cast<int32_t>( pImage_->GetHeight() );

        auto srcRect = srcRect_;
        smp::dom::AdjustAxis( srcRect.X, srcRect.Width );
        smp::dom::AdjustAxis( srcRect.Y, srcRect.Height );

        const auto dstW = static_cast<int32_t>( options_.resizeWidthOpt.value_or( options_.resizeHeightOpt ? imageW * *options_.resizeHeightOpt / imageH : imageW ) );
        const auto dstH = static_cast<int32_t>( options_.resizeHeightOpt.value_or( options_.resizeWidthOpt ? imageH * *options_.resizeWidthOpt / imageW : imageH ) );
        const Gdiplus::Rect dstRect{ 0, 0, dstW, dstH };

        if ( options_.shouldFlipY )
        {
            auto gdiRet = pImage_->RotateFlip( Gdiplus::RotateNoneFlipY );
            smp::CheckGdiPlusStatus( gdiRet, "RotateFlip" );
        }

        if ( !srcRect.Equals( { 0, 0, imageW, imageH } ) || dstRect.Width != imageW || dstRect.Height != imageH )
        {
            auto pBitmap = std::make_unique<Gdiplus::Bitmap>( dstRect.Width, dstRect.Height );
            smp::CheckGdiPlusObject( pBitmap );

            auto pGraphics = std::make_unique<Gdiplus::Graphics>( pBitmap.get() );
            smp::CheckGdiPlusObject( pGraphics );

            const auto interpolationMode = [&] {
                if ( options_.resizeQuality == "high" )
                {
                    return Gdiplus::InterpolationModeHighQualityBicubic;
                }
                else if ( options_.resizeQuality == "medium" )
                {
                    return Gdiplus::InterpolationModeHighQualityBilinear;
                }
                else if ( options_.resizeQuality == "pixelated" )
                {
                    return Gdiplus::InterpolationModeNearestNeighbor;
                }
                else
                {
                    return Gdiplus::InterpolationModeBilinear;
                }
            }();
            auto gdiRet = pGraphics->SetInterpolationMode( interpolationMode );
            smp::CheckGdiPlusStatus( gdiRet, "SetInterpolationMode" );

            gdiRet = pGraphics->DrawImage( pImage_.get(), dstRect, srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, Gdiplus::UnitPixel );
            smp::CheckGdiPlusStatus( gdiRet, "DrawImage" );

            pImage_ = std::move( pBitmap );
        }

        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<BitmapExtractEvent>(
                                                  std::move( pImage_ ),
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

BitmapExtractEvent::BitmapExtractEvent( std::unique_ptr<Gdiplus::Image> pImage,
                                        JSContext* cx,
                                        std::shared_ptr<mozjs::HeapHelper> pHeapHelper,
                                        uint32_t jsTargetId )
    : smp::JsRunnableEvent( cx, pHeapHelper, jsTargetId )
    , pJsCtx_( cx )
    , pImage_( std::move( pImage ) )
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

        JS::RootedObject jsResult( pJsCtx_, mozjs::ImageBitmap::CreateJs( pJsCtx_, std::move( pImage_ ) ) );
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
    nullptr
};

JSClass jsClass = {
    "ImageBitmap",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( close, ImageBitmap::Close );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "close", close, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, mozjs::ImageBitmap::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, mozjs::ImageBitmap::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "height", get_height, kDefaultPropsFlags ),
        JS_PSG( "width", get_width, kDefaultPropsFlags ),
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

ImageBitmap::ImageBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Image> pImage )
    : pJsCtx_( cx )
    , pImage_( std::move( pImage ) )
{
    assert( pImage_ );
}

ImageBitmap::~ImageBitmap()
{
}

std::unique_ptr<ImageBitmap>
ImageBitmap::CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Image> pImage )
{
    return std::unique_ptr<ImageBitmap>( new ImageBitmap( cx, std::move( pImage ) ) );
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

Gdiplus::Image* ImageBitmap::GetBitmap()
{
    return pImage_.get();
}

void ImageBitmap::Close()
{
    pImage_.reset();
}

uint32_t ImageBitmap::get_Height() const
{
    return ( pImage_ ? pImage_->GetHeight() : 0 );
}

uint32_t ImageBitmap::get_Width() const
{
    return ( pImage_ ? pImage_->GetWidth() : 0 );
}

JSObject* ImageBitmap::CreateImageBitmapImpl( JSContext* cx, JS::HandleValue image, int32_t sx, int32_t sy, std::optional<int32_t> sw, std::optional<int32_t> sh, JS::HandleValue options )
{
    const auto parsedOptions = ParseCreateBitmapOptions( cx, options );
    qwr::QwrException::ExpectTrue( !parsedOptions.resizeWidthOpt || *parsedOptions.resizeWidthOpt, "resizeWidth passed must be nonzero" );
    qwr::QwrException::ExpectTrue( !parsedOptions.resizeHeightOpt || *parsedOptions.resizeHeightOpt, "resizeHeight passed must be nonzero" );

    JS::RootedObject jsPromise( cx, JS::NewPromiseObject( cx, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    const auto processBitmap = [&]( auto pBitmap ) {
        qwr::QwrException::ExpectTrue( !!pBitmap, "An attempt was made to use an object that is not, or is no longer, usable" );

        const auto bitmapW = static_cast<int32_t>( pBitmap->GetWidth() );
        const auto bitmapH = static_cast<int32_t>( pBitmap->GetHeight() );

        std::unique_ptr<Gdiplus::Image> pClonedImage( static_cast<Gdiplus::Image*>( pBitmap )->Clone() );
        smp::CheckGdiPlusObject( pClonedImage );

        qwr::QwrException::ExpectTrue( bitmapW, "The source image width is 0" );
        qwr::QwrException::ExpectTrue( bitmapH, "The source image height is 0" );

        const Gdiplus::Rect srcRect{ sx, sy, sw.value_or( bitmapW ), sw.value_or( bitmapH ) };
        auto pFetchTask = std::make_shared<BitmapExtractThreadTask>( std::move( pClonedImage ), srcRect, parsedOptions, cx, jsPromise, GetPanelHwndForCurrentGlobal( cx ) );
        fb2k::inCpuWorkerThread( [pFetchTask] { pFetchTask->Run(); } );
    };

    try
    {
        qwr::QwrException::ExpectTrue( !sw || *sw, "The crop rect width is 0" );
        qwr::QwrException::ExpectTrue( !sh || *sh, "The crop rect height is 0" );
        qwr::QwrException::ExpectTrue( image.isObject(), "image argument is not an object" );
        JS::RootedObject jsObject( cx, &image.toObject() );

        if ( auto pImage = JsObjectBase<Image>::ExtractNative( cx, jsObject ) )
        {
            auto pLoadedImage = pImage->GetLoadedImage();
            qwr::QwrException::ExpectTrue( !!pLoadedImage, "An attempt was made to use an object that is not, or is no longer, usable" );
            qwr::QwrException::ExpectTrue( pLoadedImage->width, "The source image width is 0" );
            qwr::QwrException::ExpectTrue( pLoadedImage->height, "The source image height is 0" );

            const Gdiplus::Rect srcRect{ sx, sy, sw.value_or( pLoadedImage->width ), sw.value_or( pLoadedImage->height ) };
            auto pFetchTask = std::make_shared<BitmapExtractThreadTask>( pLoadedImage, srcRect, parsedOptions, cx, jsPromise, GetPanelHwndForCurrentGlobal( cx ) );
            fb2k::inCpuWorkerThread( [pFetchTask] { pFetchTask->Run(); } );
        }
        else if ( auto pImageBitmap = ImageBitmap::ExtractNative( cx, jsObject ) )
        {
            processBitmap( pImageBitmap->GetBitmap() );
        }
        else if ( auto pCanvas = Canvas::ExtractNative( cx, jsObject ) )
        {
            processBitmap( &pCanvas->GetBitmap() );
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

ImageBitmap::CreateBitmapOptions
ImageBitmap::ParseCreateBitmapOptions( JSContext* cx, JS::HandleValue options )
{
    CreateBitmapOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.resizeWidthOpt = utils::GetOptionalProperty<uint32_t>( cx, jsOptions, "resizeWidth" );
    parsedOptions.resizeHeightOpt = utils::GetOptionalProperty<uint32_t>( cx, jsOptions, "resizeHeight" );

    const auto imageOrientationOpt = utils::GetOptionalProperty<qwr::u8string>( cx, jsOptions, "imageOrientation" );
    parsedOptions.shouldFlipY = ( imageOrientationOpt == "flipY" );

    utils::OptionalPropertyTo( cx, jsOptions, "resizeQuality", parsedOptions.resizeQuality );

    return parsedOptions;
}

} // namespace mozjs
