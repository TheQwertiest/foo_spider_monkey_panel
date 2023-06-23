#include <stdafx.h>

#include "track_image.h"

#include <graphics/image_manager.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/utils/js_property_helper.h>
#include <js_backend/utils/panel_from_global.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_promise_event.h>

#include <qwr/algorithm.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

/// @brief
#include <utils/gdi_helpers.h>

using namespace smp;

namespace
{

enum class ImageSource
{
    fb2kControlled,
    embedded,
    stub
};

struct AlbumArtManagerConfig_EmbedOnly : public album_art_manager_config
{
    bool get_external_pattern( pfc::string_base& out, const GUID& type ) override
    {
        return false;
    }

    bool use_embedded_pictures() override
    {
        return true;
    };

    bool use_fallbacks() override
    {
        return false;
    };
};

struct ArtData
{
    album_art_data_ptr pArtData;
    std::optional<qwr::u8string> pathOpt;
};

class ImageFetchThreadTask
{
public:
    [[nodiscard]] ImageFetchThreadTask( metadb_handle_ptr handle,
                                        const qwr::u8string& imageType,
                                        const std::vector<ImageSource>& sources,
                                        JSContext* cx,
                                        JS::HandleObject jsTarget,
                                        HWND hPanelWnd );

    ~ImageFetchThreadTask() = default;

    ImageFetchThreadTask( const ImageFetchThreadTask& ) = delete;
    ImageFetchThreadTask& operator=( const ImageFetchThreadTask& ) = delete;

    void Run();

private:
    std::optional<ArtData> GetArtData() const;

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapDataHolder_Object heapHolder_;

    HWND hPanelWnd_ = nullptr;

    metadb_handle_ptr handle_;
    const qwr::u8string imageType_;
    const GUID imageTypeGuid_;
    const std::vector<ImageSource> sources_;
};

} // namespace

namespace
{

ImageFetchThreadTask::ImageFetchThreadTask( metadb_handle_ptr handle,
                                            const qwr::u8string& imageType,
                                            const std::vector<ImageSource>& sources,
                                            JSContext* cx,
                                            JS::HandleObject jsTarget,
                                            HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , handle_( handle )
    , imageType_( imageType )
    , imageTypeGuid_( mozjs::TrackImage::kImageTypeToGuid.at( imageType ) )
    , sources_( sources )
{
    assert( !sources_.empty() );
}

void ImageFetchThreadTask::Run()
{
    try
    {
        const auto artDataOpt = GetArtData();
        if ( !artDataOpt )
        {
            smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                                  std::make_unique<smp::JsPromiseEvent>(
                                                      pJsCtx_,
                                                      std::move( heapHolder_ ),
                                                      [] { return JS::NullValue(); } ) );
            return;
        }
        const auto& [pArt, imagePathOpt] = *artDataOpt;

        auto hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
        qwr::error::CheckHR( hr, "CoInitializeEx" );

        qwr::final_action autoCo( [] { CoUninitialize(); } );

        const auto pLoadedImage = smp::ImageManager::Load( { static_cast<const uint8_t*>( pArt->data() ), pArt->size() } );

        // TODO: put image source in path as well (i.e. fb2k-controlled, embedded, stub)
        // don't cache stub
        // think about single track radio scenario
        const auto src = fmt::format( "{}:{}", qwr::u8string{ handle_->get_path() }, imageType_ );

        const auto promiseResolver = [cx = pJsCtx_, handle = handle_, pLoadedImage, imageType = imageType_, imagePathOpt, src]() -> JS::Value {
            // TODO: implement caching (in getter as well) after implementing binary deduplication
            // smp::ImageManager::Get().MaybeCache( qwr::unicode::ToWide(src), pLoadedImage );

            JS::RootedObject jsResult( cx,
                                       mozjs::JsObjectBase<mozjs::TrackImage>::CreateJs( cx,
                                                                                         handle,
                                                                                         pLoadedImage,
                                                                                         imageType,
                                                                                         imagePathOpt,
                                                                                         src ) );
            JS::RootedValue jsResultValue( cx, JS::ObjectValue( *jsResult ) );
            return jsResultValue;
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
    catch ( const foobar2000_io::exception_aborted& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
    }
    catch ( const pfc::exception& /*e*/ )
    { // album_art_extractor may throw on io failure
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
    }
}

std::optional<ArtData> ImageFetchThreadTask::GetArtData() const
{
    const auto pArtMgr = album_art_manager_v3::get();
    auto& aborter = fb2k::mainAborter();

    const auto handles = pfc::list_single_ref_t<metadb_handle_ptr>( handle_ );
    const auto guids = pfc::list_single_ref_t<GUID>( imageTypeGuid_ );

    const auto getDefaultExtractor = [&] {
        return pArtMgr->open_v3( handles, guids, nullptr, aborter );
    };
    const auto getEmbeddedExtractor = [&] {
        auto pArtConfig = fb2k::service_new<AlbumArtManagerConfig_EmbedOnly>();
        return pArtMgr->open_v3( handles, guids, pArtConfig, aborter );
    };
    const auto getStubExtractor = [&] {
        return pArtMgr->open_stub( aborter );
    };

    const std::unordered_map<ImageSource, std::function<album_art_extractor_instance_v2::ptr()>>
        sourceToFn{
            { ImageSource::fb2kControlled, getDefaultExtractor },
            { ImageSource::embedded, getEmbeddedExtractor },
            { ImageSource::stub, getStubExtractor },
        };

    for ( const auto& source: sources_ )
    {
        assert( sourceToFn.contains( source ) );
        try
        {
            const auto pArtExtractor = std::invoke( sourceToFn.at( source ) );
            if ( !pArtExtractor.is_valid() )
            {
                continue;
            }

            const auto pArt = pArtExtractor->query( imageTypeGuid_, aborter );
            const auto pathOpt = [&]() -> std::optional<qwr::u8string> {
                const auto pPathList = pArtExtractor->query_paths( imageTypeGuid_, aborter );
                if ( !pPathList->get_count() )
                {
                    return std::nullopt;
                }
                return pPathList->get_path( 0 );
            }();

            return ArtData{ pArt, pathOpt };
        }
        catch ( const exception_album_art_not_found& /*e*/ )
        {
            continue;
        }
    }

    return std::nullopt;
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
    JsObjectBase<TrackImage>::FinalizeJsObject,
    nullptr,
    nullptr,
    TrackImage::Trace
};

JSClass jsClass = {
    "TrackImage",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_imagePath, TrackImage::get_ImagePath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_track, TrackImage::get_Track )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_type, TrackImage::get_Type )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "imagePath", get_imagePath, kDefaultPropsFlags ),
        JS_PSG( "track", get_track, kDefaultPropsFlags ),
        JS_PSG( "type", get_type, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::TrackImage );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackImage>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<TrackImage>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<TrackImage>::PrototypeId = JsPrototypeId::New_TrackImage;

const std::unordered_map<qwr::u8string, const GUID> TrackImage::kImageTypeToGuid = {
    { "front-cover", album_art_ids::cover_front },
    { "back-cover", album_art_ids::cover_back },
    { "disc", album_art_ids::disc },
    { "icon", album_art_ids::icon },
    { "artist", album_art_ids::artist },
};

TrackImage::TrackImage( JSContext* cx,
                        metadb_handle_ptr handle,
                        smp::not_null_shared<const smp::LoadedImage> pLoadedImage,
                        const qwr::u8string& imageType,
                        const std::optional<qwr::u8string>& imagePathOpt,
                        const qwr::u8string& src )
    : Image( cx, pLoadedImage, qwr::unicode::ToWide( src ) )
    , pJsCtx_( cx )
    , imageType_( imageType )
    , imagePathOpt_( imagePathOpt )
    , handle_( handle )
{
}

TrackImage::~TrackImage()
{
}

std::unique_ptr<TrackImage>
TrackImage::CreateNative( JSContext* cx,
                          metadb_handle_ptr handle,
                          smp::not_null_shared<const smp::LoadedImage> pLoadedImage,
                          const qwr::u8string& imageType,
                          const std::optional<qwr::u8string>& imagePathOpt,
                          const qwr::u8string& src )

{
    return std::unique_ptr<TrackImage>( new TrackImage( cx, handle, pLoadedImage, imageType, imagePathOpt, src ) );
}

size_t TrackImage::GetInternalSize() const
{
    return 0;
}

void TrackImage::Trace( JSTracer* trc, JSObject* obj )
{
    // TODO: check if underlying object is traced
    auto pNative = JsObjectBase<TrackImage>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsTrack_, "Heap: TrackImage: track" );
}

JSObject* TrackImage::LoadImage( JSContext* cx, smp::not_null<Track*> track, const qwr::u8string& imageType, JS::HandleValue options )
{
    qwr::QwrException::ExpectTrue( options.isUndefined() || options.isObject(), "Invalid options type" );

    qwr::QwrException::ExpectTrue( qwr::FindAsPointer( kImageTypeToGuid, imageType ), "Unknown image type" );

    std::vector<ImageSource> sources{ ImageSource::fb2kControlled };
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( cx, &options.toObject() );

        static const std::unordered_map<qwr::u8string, ImageSource> kKnownSources{
            { "fb2k-controlled", ImageSource::fb2kControlled },
            { "embedded", ImageSource::embedded },
            { "stub", ImageSource::stub }
        };

        std::vector<qwr::u8string> sourceStrs;
        utils::OptionalPropertyTo( cx, jsOptions, "sources", sourceStrs );
        if ( !sourceStrs.empty() )
        {
            qwr::QwrException::ExpectTrue(
                ranges::all_of( sourceStrs, [&]( const auto& src ) { return kKnownSources.contains( src ); } ),
                "Unknown image source" );
            sources = sourceStrs
                      | ranges::views::transform( [&]( const auto& src ) { return kKnownSources.at( src ); } )
                      | ranges::to<std::vector>();
        }
    }

    // TODO: check image cache here first (at least for playing track)

    JS::RootedObject jsPromise( cx, JS::NewPromiseObject( cx, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    const auto pFetchTask = std::make_shared<ImageFetchThreadTask>( track->GetHandle(),
                                                                    imageType,
                                                                    sources,
                                                                    cx,
                                                                    jsPromise,
                                                                    GetPanelHwndForCurrentGlobal( cx ) );
    fb2k::inCpuWorkerThread( [pFetchTask] { pFetchTask->Run(); } );

    return jsPromise;
}

std::optional<qwr::u8string> TrackImage::get_ImagePath() const
{
    return imagePathOpt_;
}

JSObject* TrackImage::get_Track() const
{
    if ( !jsTrack_ )
    {
        jsTrack_ = Track::CreateJs( pJsCtx_, handle_ );
    }

    return jsTrack_.get();
}

qwr::u8string TrackImage::get_Type() const
{
    return imageType_;
}

void TrackImage::put_Src( JS::HandleObject /*jsSelf*/, const std::wstring& /*value*/ )
{
    throw qwr::QwrException( "Source changing is not allowed in track image" );
}

} // namespace mozjs
