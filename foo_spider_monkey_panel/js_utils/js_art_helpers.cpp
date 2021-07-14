#include <stdafx.h>

#include "js_art_helpers.h"

#include <convert/native_to_js.h>
#include <events/event_js_task.h>
#include <events/event_manager.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_async_task.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <utils/art_helpers.h>
#include <utils/gdi_helpers.h>
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

class JsAlbumArtTask
    : public JsAsyncTaskImpl<JS::HandleValue>
{
public:
    JsAlbumArtTask( JSContext* cx,
                    JS::HandleValue jsPromise );
    ~JsAlbumArtTask() override = default;

    void SetData( std::unique_ptr<Gdiplus::Bitmap> image,
                  const qwr::u8string& path );

private:
    bool InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue ) override;

private:
    std::unique_ptr<Gdiplus::Bitmap> image_;
    qwr::u8string path_;
};

class AlbumArtV2FetchTask
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
    ~AlbumArtV2FetchTask() = default;

    AlbumArtV2FetchTask( const AlbumArtV2FetchTask& ) = delete;
    AlbumArtV2FetchTask& operator=( const AlbumArtV2FetchTask& ) = delete;

    /// @details Executed off main thread
    void operator()();

private:
    HWND hNotifyWnd_;
    metadb_handle_ptr handle_;
    qwr::u8string rawPath_;
    uint32_t artId_;
    bool needStub_;
    bool onlyEmbed_;
    bool noLoad_;

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

    JS::RootedValue jsPromiseValue( cx, JS::ObjectValue( *jsPromise ) );
    jsTask_ = std::make_unique<JsAlbumArtTask>( cx, jsPromiseValue );
}

void AlbumArtV2FetchTask::operator()()
{
    if ( !jsTask_->IsCanceled() )
    { // the task still might be executed and posted, since we don't block here
        return;
    }

    qwr::u8string imagePath;
    auto bitmap = smp::art::GetBitmapFromMetadbOrEmbed( handle_, artId_, needStub_, onlyEmbed_, noLoad_, &imagePath );

    jsTask_->SetData( std::move( bitmap ), imagePath );

    EventManager::Get().PutEvent( hNotifyWnd_,
                                  std::make_unique<Event_JsTask>(
                                      EventId::kInternalGetAlbumArtPromiseDone, jsTask_ ) );
}

JsAlbumArtTask::JsAlbumArtTask( JSContext* cx,
                                JS::HandleValue jsPromise )
    : JsAsyncTaskImpl( cx, jsPromise )
{
}

void JsAlbumArtTask::SetData( std::unique_ptr<Gdiplus::Bitmap> image, const qwr::u8string& path )
{
    image_ = std::move( image );
    path_ = path;
}

bool JsAlbumArtTask::InvokeJsImpl( JSContext* cx, JS::HandleObject, JS::HandleValue jsPromiseValue )
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

        JS::RootedObject jsResult( cx, JS_NewPlainObject( cx ) );
        if ( !jsResult )
        {
            throw JsException();
        }

        AddProperty( cx, jsResult, "image", JS::HandleValue{ jsBitmapValue } );
        AddProperty( cx, jsResult, "path", path_ );

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

    return true;
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

    smp::GetThreadPoolInstance().AddTask( [task = std::make_shared<AlbumArtV2FetchTask>( cx, jsObject, hWnd, handle, art_id, need_stub, only_embed, no_load )] {
        std::invoke( *task );
    } );

    return jsObject;
}

} // namespace mozjs::art
