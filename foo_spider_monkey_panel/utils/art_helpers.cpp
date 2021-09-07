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

namespace
{

using namespace smp;

class AlbumArtFetchTask
{
public:
    AlbumArtFetchTask( HWND hNotifyWnd, metadb_handle_ptr handle, uint32_t artId, bool need_stub, bool only_embed, bool no_load );

    AlbumArtFetchTask( const AlbumArtFetchTask& ) = delete;
    AlbumArtFetchTask& operator=( const AlbumArtFetchTask& ) = delete;

    void operator()();

private:
    void run();

private:
    HWND hNotifyWnd_;
    metadb_handle_ptr handle_;
    uint32_t artId_;
    bool needStub_;
    bool onlyEmbed_;
    bool noLoad_;
};

AlbumArtFetchTask::AlbumArtFetchTask( HWND hNotifyWnd, metadb_handle_ptr handle, uint32_t artId, bool need_stub, bool only_embed, bool no_load )
    : hNotifyWnd_( hNotifyWnd )
    , handle_( handle )
    , artId_( artId )
    , needStub_( need_stub )
    , onlyEmbed_( only_embed )
    , noLoad_( no_load )
{
}

void AlbumArtFetchTask::operator()()
{
    return run();
}

void AlbumArtFetchTask::run()
{
    qwr::u8string imagePath;
    auto bitmap = art::GetBitmapFromMetadbOrEmbed( handle_, artId_, needStub_, onlyEmbed_, noLoad_, &imagePath );

    EventDispatcher::Get().PutEvent( hNotifyWnd_,
                                     GenerateEvent_JsCallback(
                                         EventId::kInternalGetAlbumArtDone,
                                         handle_,
                                         artId_,
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
std::unique_ptr<Gdiplus::Bitmap> ExtractBitmap( album_art_extractor_instance_v2::ptr extractor, const GUID& artTypeGuid, bool no_load, qwr::u8string* pImagePath, abort_callback& abort )
{
    album_art_data_ptr data = extractor->query( artTypeGuid, abort );
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( !no_load )
    {
        bitmap = GetBitmapFromAlbumArtData( data );
    }

    if ( pImagePath && ( no_load || bitmap ) )
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

embed_thread::embed_thread( EmbedAction action,
                            album_art_data_ptr data,
                            const metadb_handle_list& handles,
                            GUID what )
    : m_action( action )
    , m_data( data )
    , m_handles( handles )
    , m_what( what )
{
}

void embed_thread::run( threaded_process_status& p_status,
                        abort_callback& p_abort )
{
    auto api = file_lock_manager::get();
    const auto stlHandleList = qwr::pfc_x::Make_Stl_Ref( m_handles );

    for ( auto&& [i, handle]: ranges::views::enumerate( stlHandleList ) )
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
            switch ( m_action )
            {
            case EmbedAction::embed:
            {
                aaep->set( m_what, m_data, p_abort );
                break;
            }
            case EmbedAction::remove:
            {
                aaep->remove( m_what );
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
                    const std::array<const GUID*, 5> guids = {
                        &album_art_ids::cover_front,
                        &album_art_ids::cover_back,
                        &album_art_ids::disc,
                        &album_art_ids::icon,
                        &album_art_ids::artist
                    };

                    for ( const auto pGuid: guids )
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
    const std::array<const GUID*, 5> guids = {
        &album_art_ids::cover_front,
        &album_art_ids::cover_back,
        &album_art_ids::disc,
        &album_art_ids::icon,
        &album_art_ids::artist,
    };

    qwr::QwrException::ExpectTrue( art_id < guids.size(), "Unknown art_id: {}", art_id );

    return *guids[art_id];
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const qwr::u8string& rawpath, uint32_t art_id )
{
    const pfc::string_extension extension( rawpath.c_str() );
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

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, qwr::u8string* pImagePath )
{
    assert( handle.is_valid() );

    const GUID& artTypeGuid = GetGuidForArtId( art_id );
    qwr::TimedAbortCallback abort;
    auto aamv2 = album_art_manager_v2::get();

    try
    {
        auto aaeiv2 = aamv2->open( pfc::list_single_ref_t<metadb_handle_ptr>( handle ), pfc::list_single_ref_t<GUID>( artTypeGuid ), abort );
        return ExtractBitmap( aaeiv2, artTypeGuid, no_load, pImagePath, abort );
    }
    catch ( const pfc::exception& )
    { // not found or aborted
        if ( need_stub )
        {
            try
            {
                auto aaeiv2 = aamv2->open_stub( abort );
                return ExtractBitmap( aaeiv2, artTypeGuid, no_load, pImagePath, abort );
            }
            catch ( const pfc::exception& )
            { // not found or aborted
            }
        }
    }

    return nullptr;
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadbOrEmbed( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load, qwr::u8string* pImagePath )
{
    assert( handle.is_valid() );

    qwr::u8string imagePath;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    try
    {
        if ( only_embed )
        {
            bitmap = GetBitmapFromEmbeddedData( handle->get_path(), art_id );
            if ( bitmap )
            {
                imagePath = handle->get_path();
            }
        }
        else
        {
            bitmap = GetBitmapFromMetadb( handle, art_id, need_stub, no_load, &imagePath );
        }
    }
    catch ( const qwr::QwrException& )
    { // The only possible exception is invalid art_id, which should be checked beforehand
        assert( 0 );
    }

    if ( pImagePath )
    {
        *pImagePath = ( imagePath.empty() ? "" : file_path_display( imagePath.c_str() ).get_ptr() );
    }

    return bitmap;
}

void GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    assert( handle.is_valid() );
    (void)GetGuidForArtId( art_id ); ///< Check that art id is valid, since we don't want to throw in helper thread

    smp::GetThreadPoolInstance().AddTask( [task = std::make_shared<AlbumArtFetchTask>( hWnd, handle, art_id, need_stub, only_embed, no_load )] {
        std::invoke( *task );
    } );
}

} // namespace smp::art
