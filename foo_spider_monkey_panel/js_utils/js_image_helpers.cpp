#include <stdafx.h>

#include "js_art_helpers.h"

#include <convert/native_to_js.h>
#include <events/event_dispatcher.h>
#include <events/event_js_task.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_async_task.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_helpers.h>
#include <utils/image_helpers.h>
#include <utils/thread_pool_instance.h>

#include <qwr/string_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

// TODO: remove duplicate code from art_helpers

using namespace smp;

namespace
{

using namespace mozjs;

class JsImageTask
    : public JsAsyncTaskImpl<JS::HandleValue>
{
public:
    JsImageTask( JSContext* cx,
                 JS::HandleValue jsPromise );
    ~JsImageTask() override = default;

    void SetData( std::unique_ptr<Gdiplus::Bitmap> image );

private:
    bool InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue ) override;

private:
    std::unique_ptr<Gdiplus::Bitmap> image_;
};

class ImageFetchTask
{
public:
    ImageFetchTask( JSContext* cx,
                    JS::HandleObject jsPromise,
                    HWND hNotifyWnd,
                    const std::wstring& imagePath );

    /// @details Executed off main thread
    ~ImageFetchTask() = default;

    ImageFetchTask( const ImageFetchTask& ) = delete;
    ImageFetchTask& operator=( const ImageFetchTask& ) = delete;

    /// @details Executed off main thread
    void operator()();

private:
    HWND hNotifyWnd_;
    std::wstring imagePath_;

    std::shared_ptr<JsImageTask> jsTask_;
};

} // namespace

namespace
{

ImageFetchTask::ImageFetchTask( JSContext* cx,
                                JS::HandleObject jsPromise,
                                HWND hNotifyWnd,
                                const std::wstring& imagePath )
    : hNotifyWnd_( hNotifyWnd )
    , imagePath_( imagePath )
{
    assert( cx );

    JS::RootedValue jsPromiseValue( cx, JS::ObjectValue( *jsPromise ) );
    jsTask_ = std::make_unique<JsImageTask>( cx, jsPromiseValue );
}

void ImageFetchTask::operator()()
{
    if ( !jsTask_->IsCanceled() )
    { // the task still might be executed and posted, since we don't block here
        return;
    }

    auto bitmap = smp::image::LoadImage( imagePath_ );

    jsTask_->SetData( std::move( bitmap ) );

    EventDispatcher::Get().PutEvent( hNotifyWnd_,
                                     std::make_unique<Event_JsTask>(
                                         EventId::kInternalLoadImagePromiseDone, jsTask_ ) );
}

JsImageTask::JsImageTask( JSContext* cx,
                          JS::HandleValue jsPromise )
    : JsAsyncTaskImpl( cx, jsPromise )
{
}

void JsImageTask::SetData( std::unique_ptr<Gdiplus::Bitmap> image )
{
    image_ = std::move( image );
}

bool JsImageTask::InvokeJsImpl( JSContext* cx, JS::HandleObject, JS::HandleValue jsPromiseValue )
{
    JS::RootedObject jsPromise( cx, &jsPromiseValue.toObject() );

    try
    {
        JS::RootedValue jsBitmapValue( cx );
        if ( image_ )
        {
            JS::RootedObject jsBitmap( cx, JsGdiBitmap::CreateJs( cx, std::move( image_ ) ) );
            jsBitmapValue = jsBitmap ? JS::ObjectValue( *jsBitmap ) : JS::UndefinedValue();
        }

        (void)JS::ResolvePromise( cx, jsPromise, jsBitmapValue );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );

        JS::RootedValue jsError( cx );
        (void)JS_GetPendingException( cx, &jsError );

        JS::RejectPromise( cx, jsPromise, jsError );
    }

    return true;
}

} // namespace

namespace mozjs::image
{

JSObject* GetImagePromise( JSContext* cx, HWND hWnd, const std::wstring& imagePath )
{
    JS::RootedObject jsObject( cx, JS::NewPromiseObject( cx, nullptr ) );
    JsException::ExpectTrue( jsObject );

    smp::GetThreadPoolInstance().AddTask( [task = std::make_shared<ImageFetchTask>( cx, jsObject, hWnd, imagePath )] {
        std::invoke( *task );
    } );

    return jsObject;
}

} // namespace mozjs::image
