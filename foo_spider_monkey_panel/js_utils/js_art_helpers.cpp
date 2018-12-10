#include <stdafx.h>
#include "js_art_helpers.h"

#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_async_task.h>
#include <utils/art_helpers.h>
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

class JsAlbumArtTask
    : public JsAsyncTaskImpl<JS::HandleValue>
{
public:
    JsAlbumArtTask( JSContext* cx,
                    JS::HandleValue jsPromise );
    ~JsAlbumArtTask() override = default;

    void SetData( std::unique_ptr<Gdiplus::Bitmap> image,
                  const pfc::string8_fast& path );

private:
    void InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue ) override;

private:
    std::unique_ptr<Gdiplus::Bitmap> image_;
    pfc::string8_fast path_;
};

class AlbumArtV2FetchTask
    : public IHeapUser
{
public:
    AlbumArtV2FetchTask( JSContext* cx,
                         JS::HandleObject jsPromise,
                         HWND hNotifyWnd, 
                         metadb_handle_ptr handle,
                         uint32_t artId,
                         bool need_stub,
                         bool only_embed,
                         bool no_load );

    /// @details Executed off main thread
    ~AlbumArtV2FetchTask() override;

    AlbumArtV2FetchTask( const AlbumArtV2FetchTask& ) = delete;
    AlbumArtV2FetchTask& operator=( const AlbumArtV2FetchTask& ) = delete;

    // IHeapUser
    void PrepareForGlobalGc() override;

    void operator()();

private:
    void run();

private:
    HWND hNotifyWnd_;
    metadb_handle_ptr handle_;
    pfc::string8_fast rawPath_;
    uint32_t artId_;
    bool needStub_;
    bool onlyEmbed_;
    bool noLoad_;

    std::mutex cleanupLock_;
    bool isJsAvailable_ = false;

    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;

    std::shared_ptr<JsAlbumArtTask> jsTask_;
};

} // namespace

namespace
{

AlbumArtV2FetchTask::AlbumArtV2FetchTask( JSContext* cx,
                                          JS::HandleObject jsPromise,
                                          HWND hNotifyWnd, 
                                          metadb_handle_ptr handle,
                                          uint32_t artId,
                                          bool need_stub,
                                          bool only_embed,
                                          bool no_load )
    : hNotifyWnd_( hNotifyWnd )
    , handle_( handle )
    , rawPath_( handle_->get_path() )
    , artId_( artId )
    , needStub_( need_stub )
    , onlyEmbed_( only_embed )
    , noLoad_( no_load )
{
    assert( cx );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
    assert( pNativeGlobal_ );

    JS::RootedValue jsPromiseValue( cx, JS::ObjectValue( *jsPromise ) );
    jsTask_ = std::make_unique<JsAlbumArtTask>( cx, jsPromiseValue );

    pNativeGlobal_->GetHeapManager().RegisterUser( this );

    isJsAvailable_ = true;
}

AlbumArtV2FetchTask::~AlbumArtV2FetchTask()
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

void AlbumArtV2FetchTask::operator()()
{
    return run();
}

void AlbumArtV2FetchTask::PrepareForGlobalGc()
{
    std::scoped_lock sl( cleanupLock_ );
    // Global is being destroyed, can't access anything
    isJsAvailable_ = false;

    jsTask_->PrepareForGlobalGc();
}

void AlbumArtV2FetchTask::run()
{
    if ( !isJsAvailable_ )
    {
        return;
    }

    pfc::string8_fast imagePath;
    std::unique_ptr<Gdiplus::Bitmap> bitmap = smp::art::GetBitmapFromMetadbOrEmbed( handle_, artId_, needStub_, onlyEmbed_, noLoad_, &imagePath );

    jsTask_->SetData( std::move( bitmap ), imagePath );

    panel::message_manager::instance().post_callback_msg( hNotifyWnd_,
                                                          smp::CallbackMessage::internal_get_album_art_promise_done,
                                                          std::make_unique<
                                                              smp::panel::CallbackDataImpl<
                                                                  std::shared_ptr<JsAsyncTask>>>( jsTask_ ) );
}

JsAlbumArtTask::JsAlbumArtTask( JSContext* cx,
                                JS::HandleValue jsPromise )
    : JsAsyncTaskImpl( cx, jsPromise )
{
}

void JsAlbumArtTask::SetData( std::unique_ptr<Gdiplus::Bitmap> image, const pfc::string8_fast& path )
{
    image_ = std::move( image );
    path_ = path;
}

void JsAlbumArtTask::InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue )
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
        JS::RootedValue jsPath( cx );
        convert::to_js::ToValue( cx, path_, &jsPath );

        JS::RootedObject jsResult( cx, JS_NewPlainObject( cx ) );
        if ( !jsResult
             || !JS_DefineProperty( cx, jsResult, "image", jsBitmapValue, DefaultPropsFlags() )
             || !JS_DefineProperty( cx, jsResult, "path", jsPath, DefaultPropsFlags() ) )
        {
            throw JsException();
        }

        JS::RootedValue jsResultValue( cx, JS::ObjectValue( *jsResult ) );
        (void)JS::ResolvePromise( cx, jsPromise, jsResultValue );
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

namespace mozjs::art
{

JSObject* GetAlbumArtPromise( JSContext* cx, HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    assert( handle.is_valid() );
    (void)smp::art::GetGuidForArtId( art_id ); ///< Check that art id is valid, since we don't want to throw in helper thread

    JS::RootedObject jsObject( cx, JS::NewPromiseObject( cx, nullptr ) );
    JsException::ExpectTrue( jsObject );

    ThreadPool::GetInstance().AddTask( [task = std::make_shared<AlbumArtV2FetchTask>( cx, jsObject, hWnd, handle, art_id, need_stub, only_embed, no_load )] {
        std::invoke( *task );
    } );

    return jsObject;
}

} // namespace mozjs::art
