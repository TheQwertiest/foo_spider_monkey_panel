#include <stdafx.h>
#include "art_helper.h"

#include <js_utils/gdi_helpers.h>
#include <utils/string_helpers.h>

#include <helpers.h>
#include <user_message.h>

#include <Shlwapi.h>

#include <algorithm>

namespace
{

using namespace mozjs;

class AlbumArtFetchTask : public simple_thread_task
{
public:
    AlbumArtFetchTask( HWND hNotifyWnd, metadb_handle_ptr handle, uint32_t artId, bool need_stub, bool only_embed, bool no_load );

private:
    virtual void run() override;

private:
    metadb_handle_ptr handle_;
    pfc::string8_fast rawPath_;
    uint32_t artId_;
    bool needStub_;
    bool onlyEmbed_;
    bool noLoad_;
    HWND hNotifyWnd_;
};

AlbumArtFetchTask::AlbumArtFetchTask( HWND hNotifyWnd, metadb_handle_ptr handle, uint32_t artId, bool need_stub, bool only_embed, bool no_load )
    : hNotifyWnd_( hNotifyWnd )
    , handle_( handle )
    , artId_( artId )
    , needStub_( need_stub )
    , onlyEmbed_( only_embed )
    , noLoad_( no_load )
{
    if ( handle_.is_valid() )
    {
        rawPath_ = handle_->get_path();
    }
}

void AlbumArtFetchTask::run()
{
    pfc::string8_fast imagePath;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( handle_.is_valid() )
    {
        try
        {
            if ( onlyEmbed_ )
            {
                bitmap = art::GetBitmapFromEmbeddedData( rawPath_, artId_ );
                if ( bitmap )
                {
                    imagePath = handle_->get_path();
                }
            }
            else
            {
                bitmap = art::GetBitmapFromMetadb( handle_, artId_, needStub_, noLoad_, &imagePath );
            }
        }
        catch ( const smp::SmpException& )
        {// The only possible exception is invalid art_id, which should be checked beforehand
            assert( 0 );
        }
    }

    art::AsyncArtTaskResult taskResult;
    taskResult.handle = handle_;
    taskResult.artId = artId_;
    taskResult.bitmap.swap( bitmap );
    taskResult.imagePath = imagePath.is_empty() ? "" : file_path_display( imagePath.c_str() );
    SendMessage( hNotifyWnd_, CALLBACK_UWM_ON_GET_ALBUM_ART_DONE, 0, (LPARAM)&taskResult );
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromAlbumArtData( const album_art_data_ptr& data )
{
    if ( !data.is_valid() )
    {
        return nullptr;
    }

    IStreamPtr iStream;
    {
        auto memStream = SHCreateMemStream( nullptr, 0 );
        if ( !memStream )
        {
            return nullptr;
        }

        // copy and assignment operators increase Stream ref count,
        // while SHCreateMemStream returns object with ref count 1,
        // so we need to take ownership without increasing ref count
        // (or decrease ref count manually)
        iStream.Attach( memStream );
    }

    ULONG bytes_written = 0;
    HRESULT hr = iStream->Write( data->get_ptr(), data->get_size(), &bytes_written );
    if ( !SUCCEEDED( hr ) || bytes_written != data->get_size() )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> bmp( new Gdiplus::Bitmap( iStream, PixelFormat32bppPARGB ) );
    if ( !gdi::IsGdiPlusObjectValid( bmp ) )
    {
        return nullptr;
    }

    return bmp;
}

std::unique_ptr<Gdiplus::Bitmap> ExtractBitmap( album_art_extractor_instance_v2::ptr extractor, const GUID& artTypeGuid, bool no_load, pfc::string8_fast* pImagePath )
{
    abort_callback_dummy abort;
    album_art_data_ptr data = extractor->query( artTypeGuid, abort );
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( !no_load )
    {
        bitmap = GetBitmapFromAlbumArtData( data );
    }

    if ( pImagePath && ( no_load || bitmap ) )
    {
        auto pathlist = extractor->query_paths( artTypeGuid, abort );
        if ( pathlist->get_count() > 0 )
        {
            *pImagePath = pathlist->get_path( 0 );
        }
    }

    return bitmap;
}

}

namespace mozjs::art
{

embed_thread::embed_thread( t_size action,
                            album_art_data_ptr data,
                            metadb_handle_list_cref handles,
                            GUID what )
    : m_action( action )
    , m_data( data )
    , m_handles( handles )
    , m_what( what )
{}

void embed_thread::run( threaded_process_status& p_status,
                        abort_callback& p_abort )
{
    t_size count = m_handles.get_count();
    for ( t_size i = 0; i < count; ++i )
    {
        const pfc::string8_fast path = m_handles.get_item( i )->get_path();
        p_status.set_progress( i, count );
        p_status.set_item_path( path );
        album_art_editor::ptr ptr;
        if ( album_art_editor::g_get_interface( ptr, path ) )
        {
            try
            {
                auto aaep = ptr->open( NULL, path, p_abort );
                if ( m_action == 0 )
                {
                    aaep->set( m_what, m_data, p_abort );
                }
                else if ( m_action == 1 )
                {
                    aaep->remove( m_what );
                }
                aaep->commit( p_abort );
            }
            catch ( ... )
            {
            }
        }
    }
}

const GUID& GetGuidForArtId( uint32_t art_id )
{
    const GUID* guids[] =
    {
        &album_art_ids::cover_front,
        &album_art_ids::cover_back,
        &album_art_ids::disc,
        &album_art_ids::icon,
        &album_art_ids::artist,
    };

    if ( art_id >= _countof( guids ) )
    {
        throw smp::SmpException( smp::string::Formatter() << "Unknown art_id: " << art_id );
    }

    return *guids[art_id];
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const pfc::string8_fast& rawpath, uint32_t art_id )
{
    const pfc::string_extension extension( rawpath.c_str() );
    const GUID& artTypeGuid = GetGuidForArtId( art_id );

    service_enum_t<album_art_extractor> extractorEnum;
    album_art_extractor::ptr extractor;
    abort_callback_dummy abort;

    while ( extractorEnum.next( extractor ) )
    {
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
        catch ( ... )
        {
        }
    }
    
    return nullptr;
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, pfc::string8_fast* pImagePath )
{
    if ( !handle.is_valid() )
    {
        return nullptr;
    }

    const GUID& artTypeGuid = GetGuidForArtId( art_id );
    abort_callback_dummy abort;
    auto aamv2 = album_art_manager_v2::get();

    try
    {
        auto aaeiv2 = aamv2->open( pfc::list_single_ref_t<metadb_handle_ptr>( handle ), pfc::list_single_ref_t<GUID>( artTypeGuid ), abort );
        return ExtractBitmap( aaeiv2, artTypeGuid, no_load, pImagePath );
    }
    catch ( ... )
    {
    }

    if ( need_stub )
    {
        auto aaeiv2_stub = aamv2->open_stub( abort );

        try
        {
            auto data = aaeiv2_stub->query( artTypeGuid, abort );
            return ExtractBitmap( aaeiv2_stub, artTypeGuid, no_load, pImagePath );
        }
        catch ( ... )
        {
        }
    }

    return nullptr;    
}

uint32_t GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load )
{
    assert( handle.is_valid() );

    try
    {
        (void)GetGuidForArtId( art_id ); ///< Check that art id is valid, since we don't want to throw in helper thread
        auto pTask = std::make_unique<AlbumArtFetchTask>( hWnd, handle, art_id, need_stub, only_embed, no_load );
        uint32_t taskId = [&]()
        {
            uint64_t tmp = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pTask.get()));
            return static_cast<uint32_t>((tmp & 0xFFFFFFFF) ^ (tmp >> 32));
        }();

        if ( simple_thread_pool::instance().enqueue( std::move( pTask ) ) )
        {
            return taskId;
        }
    }
    catch ( const smp::SmpException& )
    {
        throw;
    }
    catch ( ... )
    {
    }

    return 0;
}

}
