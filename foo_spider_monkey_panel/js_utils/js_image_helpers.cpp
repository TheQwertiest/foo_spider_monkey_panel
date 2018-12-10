#include <stdafx.h>
#include "js_art_helpers.h"

#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_async_task.h>
#include <utils/image_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/string_helpers.h>
#include <utils/thread_pool.h>
#include <convert/native_to_js.h>

#include <helpers.h>
#include <user_message.h>
#include <message_manager.h>

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
    void InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue ) override;

private:
    std::unique_ptr<Gdiplus::Bitmap> image_;
};

class ImageFetchTask
    : public IHeapUser
{
public:
    ImageFetchTask( JSContext* cx,
                    JS::HandleObject jsPromise,
                    HWND hNotifyWnd,
                    const std::wstring& imagePath );

    /// @details Executed off main thread
    ~ImageFetchTask() override;

    ImageFetchTask( const ImageFetchTask& ) = delete;
    ImageFetchTask& operator=( const ImageFetchTask& ) = delete;

    // IHeapUser
    void PrepareForGlobalGc() override;

    void operator()();

private:
    void run();

private:
    HWND hNotifyWnd_;
    std::wstring imagePath_;

    std::mutex cleanupLock_;
    bool isJsAvailable_ = false;

    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;

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

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
    assert( pNativeGlobal_ );

    JS::RootedValue jsPromiseValue( cx, JS::ObjectValue( *jsPromise ) );
    jsTask_ = std::make_unique<JsImageTask>( cx, jsPromiseValue );

    pNativeGlobal_->GetHeapManager().RegisterUser( this );

    isJsAvailable_ = true;
}

ImageFetchTask::~ImageFetchTask()
{
    if ( !isJsAvailable_ )
    {
        return;
    }

    std::scoped_lock sl( cleanupLock_ );
    if ( !isJsAvailable_ )
    {
        return;
    }

    pNativeGlobal_->GetHeapManager().UnregisterUser( this );
}

void ImageFetchTask::operator()()
{
    return run();
}

void ImageFetchTask::PrepareForGlobalGc()
{
    std::scoped_lock sl( cleanupLock_ );
    // Global is being destroyed, can't access anything
    isJsAvailable_ = false;

    jsTask_->PrepareForGlobalGc();
}

void ImageFetchTask::run()
{
    if ( !isJsAvailable_ )
    {
        return;
    }

    std::unique_ptr<Gdiplus::Bitmap> bitmap = smp::image::LoadImage( imagePath_ );

    jsTask_->SetData( std::move( bitmap ) );

    panel::message_manager::instance().post_callback_msg( hNotifyWnd_,
                                                          smp::CallbackMessage::internal_get_album_art_promise_done,
                                                          std::make_unique<
                                                              smp::panel::CallbackDataImpl<
                                                                  std::shared_ptr<JsAsyncTask>>>( jsTask_ ) );
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

void JsImageTask::InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue )
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
}

} // namespace

namespace mozjs::image
{

JSObject* GetImagePromise( JSContext* cx, HWND hWnd, const std::wstring& imagePath )
{
    JS::RootedObject jsObject( cx, JS::NewPromiseObject( cx, nullptr ) );
    JsException::ExpectTrue( jsObject );

    ThreadPool::GetInstance().AddTask( [task = std::make_shared<ImageFetchTask>( cx, jsObject, hWnd, imagePath )] {
        std::invoke( *task );
    } );

    return jsObject;
}

} // namespace mozjs::image
