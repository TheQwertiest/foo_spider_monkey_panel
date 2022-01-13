#include <stdafx.h>

#include "art_helpers.h"

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <utils/gdi_helpers.h>
#include <utils/image_helpers.h>
#include <utils/thread_pool_instance.h>

#include <Shlwapi.h>

#include <qwr/abort_callback.h>
#include <qwr/string_helpers.h>

#include <algorithm>

using namespace smp;

namespace
{

class AlbumArtFetchTask
{
public:
    AlbumArtFetchTask( HWND hNotifyWnd, metadb_handle_ptr handle, const art::LoadingOptions& options );

    AlbumArtFetchTask( const AlbumArtFetchTask& ) = delete;
    AlbumArtFetchTask& operator=( const AlbumArtFetchTask& ) = delete;

    void operator()();

private:
    void run();

private:
    HWND hNotifyWnd_;
    metadb_handle_ptr handle_;
    const art::LoadingOptions options_;
};

} // namespace

namespace
{

const std::array<const GUID*, 5> gKnownArtGuids = {
    &album_art_ids::cover_front,
    &album_art_ids::cover_back,
    &album_art_ids::disc,
    &album_art_ids::icon,
    &album_art_ids::artist
};
}

namespace
{

AlbumArtFetchTask::AlbumArtFetchTask( HWND hNotifyWnd, metadb_handle_ptr handle, const art::LoadingOptions& options )
    : hNotifyWnd_( hNotifyWnd )
    , handle_( handle )
    , options_( options )
{
}

void AlbumArtFetchTask::operator()()
{
    return run();
}

void AlbumArtFetchTask::run()
{
    qwr::u8string imagePath;
    auto bitmap = art::GetBitmapFromMetadbOrEmbed( handle_, options_, &imagePath );

    EventDispatcher::Get().PutEvent( hNotifyWnd_,
                                     GenerateEvent_JsCallback(
                                         EventId::kInternalGetAlbumArtDone,
                                         handle_,
                                         options_.artId,
                                         std::move( bitmap ),
                                         imagePath ) );
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromAlbumArtData( const album_art_data_ptr& data )
{
    if ( !data.is_valid() )
    {
        return nullptr;
    }

    IStreamPtr pStream;
    {
        auto pMemStream = SHCreateMemStream( nullptr, 0 );
        if ( !pMemStream )
        {
            return nullptr;
        }

        // copy and assignment operators increase Stream ref count,
        // while SHCreateMemStream returns object with ref count 1,
        // so we need to take ownership without increasing ref count
        // (or decrease ref count manually)
        pStream.Attach( pMemStream );
    }

    ULONG bytes_written = 0;
    HRESULT hr = pStream->Write( data->get_ptr(), data->get_size(), &bytes_written );
    if ( FAILED( hr ) || bytes_written != data->get_size() )
    {
        return nullptr;
    }

    auto pImg = std::make_unique<Gdiplus::Bitmap>( static_cast<IStream*>( pStream ), TRUE );
    if ( !gdi::IsGdiPlusObjectValid( pImg ) )
    {
        return smp::image::LoadImageWithWIC( pStream );
    }

    return pImg;
}

/// @details Throws pfc::exception, if art is not found or if aborted
std::unique_ptr<Gdiplus::Bitmap> ExtractBitmap( album_art_extractor_instance_v2::ptr extractor, const GUID& artTypeGuid, bool onlyGetPath, qwr::u8string* pImagePath, abort_callback& abort )
{
    album_art_data_ptr data = extractor->query( artTypeGuid, abort );
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( !onlyGetPath )
    {
        bitmap = GetBitmapFromAlbumArtData( data );
    }

    if ( pImagePath && ( onlyGetPath || bitmap ) )
    {
        auto pathlist = extractor->query_paths( artTypeGuid, abort );
        if ( pathlist->get_count() )
        {
            *pImagePath = file_path_display( pathlist->get_path( 0 ) );
        }
    }

    return bitmap;
}

} // namespace

namespace smp::art
{

EmbedThread::EmbedThread( EmbedAction action,
                          album_art_data_ptr data,
                          const metadb_handle_list& handles,
                          GUID what )
    : action_( action )
    , data_( data )
    , handles_( handles )
    , what_( what )
{
}

void EmbedThread::run( threaded_process_status& p_status,
                       abort_callback& p_abort )
{
    auto api = file_lock_manager::get();
    const auto stlHandleList = qwr::pfc_x::Make_Stl_Ref( handles_ );

    for ( const auto& [i, handle]: ranges::views::enumerate( stlHandleList ) )
    {
        const qwr::u8string path = handle->get_path();
        p_status.set_progress( i, stlHandleList.size() );
        p_status.set_item_path( path.c_str() );

        album_art_editor::ptr ptr;
        if ( !album_art_editor::g_get_interface( ptr, path.c_str() ) )
        {
            continue;
        }

        try
        {
            auto scopedLock = api->acquire_write( path.c_str(), p_abort );

            auto aaep = ptr->open( nullptr, path.c_str(), p_abort );
            switch ( action_ )
            {
            case EmbedAction::embed:
            {
                aaep->set( what_, data_, p_abort );
                break;
            }
            case EmbedAction::remove:
            {
                aaep->remove( what_ );
                break;
            }
            case EmbedAction::removeAll:
            {
                album_art_editor_instance_v2::ptr v2;
                if ( aaep->cast( v2 ) )
                { // not all file formats support this
                    v2->remove_all();
                }
                else
                { // m4a is one example that needs this fallback
                    for ( const auto pGuid: gKnownArtGuids )
                    {
                        aaep->remove( *pGuid );
                    }
                }
                break;
            }
            default:
                assert( 0 );
                break;
            }

            aaep->commit( p_abort );
        }
        catch ( const pfc::exception& )
        { // operation failed (e.g. file does not support art embedding)
        }
    }
}

const GUID& GetGuidForArtId( uint32_t art_id )
{
    qwr::QwrException::ExpectTrue( art_id < gKnownArtGuids.size(), "Unknown art_id: {}", art_id );

    return *gKnownArtGuids[art_id];
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const qwr::u8string& rawpath, uint32_t art_id )
{
    const auto extension = pfc::string_extension( rawpath.c_str() );
    const GUID& artTypeGuid = GetGuidForArtId( art_id );

    qwr::TimedAbortCallback abort;
    for ( service_enum_t<album_art_extractor> e; !e.finished(); ++e )
    {
        auto extractor = e.get();
        if ( !extractor->is_our_path( rawpath.c_str(), extension ) )
        {
            continue;
        }

        try
        {
            auto aaep = extractor->open( nullptr, rawpath.c_str(), abort );
            auto data = aaep->query( artTypeGuid, abort );

            return GetBitmapFromAlbumArtData( data );
        }
        catch ( const pfc::exception& )
        { // not found or aborted
        }
    }

    return nullptr;
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadb( const metadb_handle_ptr& handle, const LoadingOptions& options, qwr::u8string* pImagePath )
{
    assert( handle.is_valid() );

    const GUID& artTypeGuid = GetGuidForArtId( options.artId );
    qwr::TimedAbortCallback abort;
    auto aamv2 = album_art_manager_v2::get();

    try
    {
        auto aaeiv2 = aamv2->open( pfc::list_single_ref_t<metadb_handle_ptr>( handle ), pfc::list_single_ref_t<GUID>( artTypeGuid ), abort );
        return ExtractBitmap( aaeiv2, artTypeGuid, options.onlyGetPath, pImagePath, abort );
    }
    catch ( const pfc::exception& )
    { // not found or aborted
        if ( options.fallbackToStubImage )
        {
            try
            {
                auto aaeiv2 = aamv2->open_stub( abort );
                return ExtractBitmap( aaeiv2, artTypeGuid, options.onlyGetPath, pImagePath, abort );
            }
            catch ( const pfc::exception& )
            { // not found or aborted
            }
        }
    }

    return nullptr;
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadbOrEmbed( const metadb_handle_ptr& handle, const LoadingOptions& options, qwr::u8string* pImagePath )
{
    assert( handle.is_valid() );

    qwr::u8string imagePath;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    try
    {
        if ( options.loadOnlyEmbedded )
        {
            bitmap = GetBitmapFromEmbeddedData( handle->get_path(), options.artId );
            if ( bitmap )
            {
                imagePath = handle->get_path();
            }
        }
        else
        {
            bitmap = GetBitmapFromMetadb( handle, options, &imagePath );
        }
    }
    catch ( const qwr::QwrException& )
    { // The only possible exception is invalid art id, which should be checked beforehand
        assert( 0 );
    }

    if ( pImagePath )
    {
        *pImagePath = ( imagePath.empty() ? "" : file_path_display( imagePath.c_str() ).get_ptr() );
    }

    return bitmap;
}

void GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, const LoadingOptions& options )
{
    assert( handle.is_valid() );

    // Art id is validated in LoadingOptions constructor, so we don't need to worry about it here

    smp::GetThreadPoolInstance().AddTask( [task = std::make_shared<AlbumArtFetchTask>( hWnd, handle, options )] {
        std::invoke( *task );
    } );
}

} // namespace smp::art
