#include <stdafx.h>

#include "track_image_manager.h"

#include <graphics/loaded_image.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/window/image.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/objects/fb2k/track_image.h>
#include <js_backend/objects/fb2k/track_list.h>
#include <js_backend/utils/heap_data_holder.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_promise_event.h>

#include <qwr/algorithm.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace
{

class EmbedImageThreadTask : public threaded_process_callback
{
public:
    [[nodiscard]] EmbedImageThreadTask( const metadb_handle_list& handles,
                                        smp::not_null_shared<const smp::LoadedImage> pLoadedImage,
                                        const GUID& imageType,
                                        JSContext* cx,
                                        JS::HandleObject jsTarget,
                                        HWND hPanelWnd );

    ~EmbedImageThreadTask() = default;

    EmbedImageThreadTask( const EmbedImageThreadTask& ) = delete;
    EmbedImageThreadTask& operator=( const EmbedImageThreadTask& ) = delete;

    void run( threaded_process_status& p_status, abort_callback& p_abort ) override;

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapDataHolder_Object heapHolder_;

    HWND hPanelWnd_ = nullptr;

    const GUID imageType_;
    metadb_handle_list handles_;
    smp::not_null_shared<const smp::LoadedImage> pLoadedImage_;
};

class UnembedImageThreadTask : public threaded_process_callback
{
public:
    [[nodiscard]] UnembedImageThreadTask( const metadb_handle_list& handles,
                                          const GUID& imageType,
                                          JSContext* cx,
                                          JS::HandleObject jsTarget,
                                          HWND hPanelWnd );

    ~UnembedImageThreadTask() = default;

    UnembedImageThreadTask( const UnembedImageThreadTask& ) = delete;
    UnembedImageThreadTask& operator=( const UnembedImageThreadTask& ) = delete;

    void run( threaded_process_status& p_status, abort_callback& p_abort ) override;

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapDataHolder_Object heapHolder_;

    HWND hPanelWnd_ = nullptr;

    const GUID imageType_;
    metadb_handle_list handles_;
};

class UnembedAllImagesThreadTask : public threaded_process_callback
{
public:
    [[nodiscard]] UnembedAllImagesThreadTask( const metadb_handle_list& handles,
                                              JSContext* cx,
                                              JS::HandleObject jsTarget,
                                              HWND hPanelWnd );

    ~UnembedAllImagesThreadTask() = default;

    UnembedAllImagesThreadTask( const UnembedAllImagesThreadTask& ) = delete;
    UnembedAllImagesThreadTask& operator=( const UnembedAllImagesThreadTask& ) = delete;

    void run( threaded_process_status& p_status, abort_callback& p_abort ) override;

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapDataHolder_Object heapHolder_;

    HWND hPanelWnd_ = nullptr;

    metadb_handle_list handles_;
};

} // namespace

namespace
{

EmbedImageThreadTask::EmbedImageThreadTask( const metadb_handle_list& handles,
                                            smp::not_null_shared<const smp::LoadedImage> pLoadedImage,
                                            const GUID& imageType,
                                            JSContext* cx,
                                            JS::HandleObject jsTarget,
                                            HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , handles_( handles )
    , imageType_( imageType )
    , pLoadedImage_( pLoadedImage )
{
}

void EmbedImageThreadTask::run( threaded_process_status& p_status, abort_callback& p_abort )
{
    try
    {
        qwr::QwrException::ExpectTrue( !p_abort.is_aborting(), "Aborted by user request" );

        const auto pFileLockMgr = file_lock_manager::get();
        const auto stlHandleList = qwr::pfc_x::Make_Stl_Ref( handles_ );

        const auto pAlbumArt = album_art_data_impl::g_create( pLoadedImage_->rawData.data(), pLoadedImage_->rawData.size() );

        for ( const auto& [i, handle]: ranges::views::enumerate( stlHandleList ) )
        {
            const qwr::u8string path = handle->get_path();
            p_status.set_progress( i, stlHandleList.size() );
            p_status.set_item_path( path.c_str() );

            album_art_editor::ptr pArtEditor;
            if ( !album_art_editor::g_get_interface( pArtEditor, path.c_str() ) )
            {
                throw qwr::QwrException( "Specified track does not support embedded images: {}", path );
            }

            try
            {
                const auto scopedFileLock = pFileLockMgr->acquire_write( path.c_str(), p_abort );

                const auto pArtEditorInstance = pArtEditor->open( nullptr, path.c_str(), p_abort );
                pArtEditorInstance->set( imageType_, pAlbumArt, p_abort );
                pArtEditorInstance->commit( p_abort );
            }
            catch ( const exception_album_art_unsupported_entry& e )
            {
                throw qwr::QwrException( e.what() );
            }
            catch ( const exception_album_art_unsupported_format& e )
            {
                throw qwr::QwrException( e.what() );
            }
        }

        // TODO: cache and return embedded image
        const auto promiseResolver = [cx = pJsCtx_] {
            return JS::Value{};
        };
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  promiseResolver ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
    }
}

UnembedImageThreadTask::UnembedImageThreadTask( const metadb_handle_list& handles,
                                                const GUID& imageType,
                                                JSContext* cx,
                                                JS::HandleObject jsTarget,
                                                HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , handles_( handles )
    , imageType_( imageType )
{
}

void UnembedImageThreadTask::run( threaded_process_status& p_status, abort_callback& p_abort )
{
    try
    {
        qwr::QwrException::ExpectTrue( !p_abort.is_aborting(), "Aborted by user request" );

        const auto pFileLockMgr = file_lock_manager::get();
        const auto stlHandleList = qwr::pfc_x::Make_Stl_Ref( handles_ );

        for ( const auto& [i, handle]: ranges::views::enumerate( stlHandleList ) )
        {
            const qwr::u8string path = handle->get_path();
            p_status.set_progress( i, stlHandleList.size() );
            p_status.set_item_path( path.c_str() );

            album_art_editor::ptr pArtEditor;
            if ( !album_art_editor::g_get_interface( pArtEditor, path.c_str() ) )
            {
                throw qwr::QwrException( "Specified track does not support embedded images: {}", path );
            }

            try
            {
                const auto scopedFileLock = pFileLockMgr->acquire_write( path.c_str(), p_abort );

                const auto pArtEditorInstance = pArtEditor->open( nullptr, path.c_str(), p_abort );
                pArtEditorInstance->remove( imageType_ );
                pArtEditorInstance->commit( p_abort );
            }
            catch ( const exception_album_art_unsupported_entry& e )
            {
                throw qwr::QwrException( e.what() );
            }
            catch ( const exception_album_art_unsupported_format& e )
            {
                throw qwr::QwrException( e.what() );
            }
        }

        // TODO: return unembedded image
        const auto promiseResolver = [cx = pJsCtx_] {
            return JS::Value{};
        };
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  promiseResolver ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
    }
}

UnembedAllImagesThreadTask::UnembedAllImagesThreadTask( const metadb_handle_list& handles,
                                                        JSContext* cx,
                                                        JS::HandleObject jsTarget,
                                                        HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , handles_( handles )
{
}

void UnembedAllImagesThreadTask::run( threaded_process_status& p_status, abort_callback& p_abort )
{
    try
    {
        qwr::QwrException::ExpectTrue( !p_abort.is_aborting(), "Aborted by user request" );

        const auto pFileLockMgr = file_lock_manager::get();
        const auto stlHandleList = qwr::pfc_x::Make_Stl_Ref( handles_ );

        for ( const auto& [i, handle]: ranges::views::enumerate( stlHandleList ) )
        {
            const qwr::u8string path = handle->get_path();
            p_status.set_progress( i, stlHandleList.size() );
            p_status.set_item_path( path.c_str() );

            album_art_editor::ptr pArtEditor;
            if ( !album_art_editor::g_get_interface( pArtEditor, path.c_str() ) )
            {
                throw qwr::QwrException( "Specified track does not support embedded images: {}", path );
            }

            try
            {
                const auto scopedFileLock = pFileLockMgr->acquire_write( path.c_str(), p_abort );

                const auto pArtEditorInstance = pArtEditor->open( nullptr, path.c_str(), p_abort );
                // not all file formats support this method (e.g. m4a)
                album_art_editor_instance_v2::ptr pArtEditorInstanceV2;
                if ( pArtEditorInstance->cast( pArtEditorInstanceV2 ) )
                {
                    pArtEditorInstanceV2->remove_all();
                }
                else
                {
                    for ( const auto& typeGuid: mozjs::TrackImage::kImageTypeToGuid | ranges::views::values )
                    {
                        pArtEditorInstance->remove( typeGuid );
                    }
                }
                pArtEditorInstance->commit( p_abort );
            }
            catch ( const exception_album_art_unsupported_entry& e )
            {
                throw qwr::QwrException( e.what() );
            }
            catch ( const exception_album_art_unsupported_format& e )
            {
                throw qwr::QwrException( e.what() );
            }
        }

        // TODO: return unembedded images
        const auto promiseResolver = [cx = pJsCtx_] {
            return JS::Value{};
        };
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  promiseResolver ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
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
    TrackImageManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "TrackImageManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( embedImage, TrackImageManager::EmbedImage, TrackImageManager::EmbedImageWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( loadImage, TrackImageManager::LoadImage, TrackImageManager::LoadImageWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( unembedImage, TrackImageManager::UnembedImage, TrackImageManager::UnembedImageWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( unembedAllImages, TrackImageManager::UnembedAllImages, TrackImageManager::UnembedAllImagesWithOpt, 1 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "embedImage", embedImage, 2, kDefaultPropsFlags ),
        JS_FN( "loadImage", loadImage, 1, kDefaultPropsFlags ),
        JS_FN( "unembedImage", unembedImage, 2, kDefaultPropsFlags ),
        JS_FN( "unembedAllImages", unembedAllImages, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::TrackImageManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackImageManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<TrackImageManager>::JsFunctions = jsFunctions.data();

TrackImageManager::TrackImageManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

TrackImageManager::~TrackImageManager()
{
}

std::unique_ptr<TrackImageManager>
TrackImageManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<TrackImageManager>( new TrackImageManager( cx ) );
}

size_t TrackImageManager::GetInternalSize() const
{
    return 0;
}

JSObject* TrackImageManager::EmbedImage( JS::HandleValue tracks, smp::not_null<Image*> image, const qwr::u8string& imageType, JS::HandleValue options )
{
    const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );

    qwr::QwrException::ExpectTrue( image->GetStatus() == Image::CompleteStatus::completely_available, "Passed-in image is empty or broken" );
    qwr::QwrException::ExpectTrue( options.isUndefined() || options.isObject(), "Invalid options imageType" );

    const auto pGuid = qwr::FindAsPointer( mozjs::TrackImage::kImageTypeToGuid, imageType );
    qwr::QwrException::ExpectTrue( pGuid, "Unknown image imageType" );

    int32_t flags = 0;
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "silent" );
             valueOpt && *valueOpt )
        {
            flags |= threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item;
        }
    }

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    auto cb = fb2k::service_new<EmbedImageThreadTask>( handles, image->GetLoadedImage(), *pGuid, pJsCtx_, jsPromise, GetPanelHwndForCurrentGlobal( pJsCtx_ ) );
    (void)threaded_process::get()->run_modeless( cb, flags, core_api::get_main_window(), "Embedding images..." );

    return jsPromise;
}

JSObject* TrackImageManager::EmbedImageWithOpt( size_t optArgCount, JS::HandleValue tracks, smp::not_null<Image*> image, const qwr::u8string& imageType, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return EmbedImage( tracks, image, imageType, options );
    case 1:
        return EmbedImage( tracks, image, imageType );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* TrackImageManager::LoadImage( smp::not_null<Track*> track, const qwr::u8string& imageType, JS::HandleValue options )
{
    return TrackImage::LoadImage( pJsCtx_, track, imageType, options );
}

JSObject* TrackImageManager::LoadImageWithOpt( size_t optArgCount, smp::not_null<Track*> track, const qwr::u8string& imageType, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return TrackImage::LoadImage( pJsCtx_, track, imageType, options );
    case 1:
        return TrackImage::LoadImage( pJsCtx_, track, imageType );
    case 2:
        return TrackImage::LoadImage( pJsCtx_, track );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* TrackImageManager::UnembedImage( JS::HandleValue tracks, const qwr::u8string& imageType, JS::HandleValue options )
{
    const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );

    qwr::QwrException::ExpectTrue( options.isUndefined() || options.isObject(), "Invalid options type" );

    const auto pGuid = qwr::FindAsPointer( mozjs::TrackImage::kImageTypeToGuid, imageType );
    qwr::QwrException::ExpectTrue( pGuid, "Unknown imageType" );

    int32_t flags = 0;
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "silent" );
             valueOpt && *valueOpt )
        {
            flags |= threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item;
        }
    }

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    auto cb = fb2k::service_new<UnembedImageThreadTask>( handles, *pGuid, pJsCtx_, jsPromise, GetPanelHwndForCurrentGlobal( pJsCtx_ ) );
    (void)threaded_process::get()->run_modeless( cb, flags, core_api::get_main_window(), "Removing embedded images..." );

    return jsPromise;
}

JSObject* TrackImageManager::UnembedImageWithOpt( size_t optArgCount, JS::HandleValue tracks, const qwr::u8string& imageType, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return UnembedImage( tracks, imageType, options );
    case 1:
        return UnembedImage( tracks, imageType );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* TrackImageManager::UnembedAllImages( JS::HandleValue tracks, JS::HandleValue options )
{
    const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );

    qwr::QwrException::ExpectTrue( options.isUndefined() || options.isObject(), "Invalid options type" );

    int32_t flags = 0;
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "silent" );
             valueOpt && *valueOpt )
        {
            flags |= threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item;
        }
    }

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    auto cb = fb2k::service_new<UnembedAllImagesThreadTask>( handles, pJsCtx_, jsPromise, GetPanelHwndForCurrentGlobal( pJsCtx_ ) );
    (void)threaded_process::get()->run_modeless( cb, flags, core_api::get_main_window(), "Removing all embedded images..." );

    return jsPromise;
}

JSObject* TrackImageManager::UnembedAllImagesWithOpt( size_t optArgCount, JS::HandleValue tracks, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return UnembedAllImages( tracks, options );
    case 1:
        return UnembedAllImages( tracks );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

} // namespace mozjs
