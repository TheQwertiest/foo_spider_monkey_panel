#include <stdafx.h>
#include "art_helper.h"

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
    ~AlbumArtFetchTask();

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

AlbumArtFetchTask::~AlbumArtFetchTask()
{

}

void AlbumArtFetchTask::run()
{
    pfc::string8_fast imagePath;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( handle_.is_valid() )
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

    art::AsyncArtTaskResult taskResult;
    taskResult.handle = handle_;
    taskResult.artId = artId_;
    taskResult.bitmap.swap( bitmap );
    taskResult.imagePath = imagePath.is_empty() ? "" : file_path_display( imagePath.c_str() );
    SendMessage( hNotifyWnd_, CALLBACK_UWM_ON_GET_ALBUM_ART_DONE, 0, (LPARAM)&taskResult );
}

// TODO: move\remove this
template <class T>
bool IsGdiplusObjectValid( T* obj )
{
    return ( ( obj ) && ( obj->GetLastStatus() == Gdiplus::Ok ) );
}

const GUID& GetGuidForArtId( uint32_t art_id )
{
    const GUID* guids[] = {
        &album_art_ids::cover_front,
        &album_art_ids::cover_back,
        &album_art_ids::disc,
        &album_art_ids::icon,
        &album_art_ids::artist,
    };

    return *guids[std::min( art_id, _countof( guids ) - 1 )];
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromAlbumArtData( const album_art_data_ptr& data )
{
    if ( !data.is_valid() )
    {
        return nullptr;
    }

    CComPtr<IStream> iStream;
    {
        auto memStream = SHCreateMemStream( nullptr, 0 );
        if ( !memStream )
        {
            return nullptr;
        }

        // copy and assignment operators increase Stream ref count,
        // while SHCreateMemStream returns ref count 1,
        // so we need to take ownership without increasing ref count
        iStream.Attach( memStream );
    }

    ULONG bytes_written = 0;
    HRESULT hr = iStream->Write( data->get_ptr(), data->get_size(), &bytes_written );
    if ( !SUCCEEDED( hr ) || bytes_written != data->get_size() )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> bmp( new Gdiplus::Bitmap( iStream, PixelFormat32bppPARGB ) );
    if ( !IsGdiplusObjectValid( bmp.get() ) )
    {
        return nullptr;
    }

    return bmp;
}

std::unique_ptr<Gdiplus::Bitmap> ExtractBitmap( album_art_extractor_instance_v2::ptr extractor, GUID& artTypeGuid, bool no_load, pfc::string8_fast* pImagePath )
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
        album_art_path_list::ptr pathlist = extractor->query_paths( artTypeGuid, abort );
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

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const pfc::string8_fast& rawpath, uint32_t art_id )
{
    pfc::string_extension extension( rawpath.c_str() );
    service_enum_t<album_art_extractor> extractorEnum;
    album_art_extractor::ptr extractor;
    abort_callback_dummy abort;

    while ( extractorEnum.next( extractor ) )
    {
        if ( !extractor->is_our_path( rawpath.c_str(), extension ) )
        {
            continue;
        }

        album_art_extractor_instance_ptr aaep;
        GUID artTypeGuid = GetGuidForArtId( art_id );

        try
        {
            aaep = extractor->open( nullptr, rawpath.c_str(), abort );
            album_art_data_ptr data = aaep->query( artTypeGuid, abort );

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

    GUID artTypeGuid = GetGuidForArtId( art_id );
    abort_callback_dummy abort;
    auto aamv2 = album_art_manager_v2::get();
    album_art_extractor_instance_v2::ptr aaeiv2;

    try
    {
        aaeiv2 = aamv2->open( pfc::list_single_ref_t<metadb_handle_ptr>( handle ), pfc::list_single_ref_t<GUID>( artTypeGuid ), abort );
        return ExtractBitmap( aaeiv2, artTypeGuid, no_load, pImagePath );
    }
    catch ( ... )
    {
    }

    if ( need_stub )
    {
        album_art_extractor_instance_v2::ptr aaeiv2_stub = aamv2->open_stub( abort );

        try
        {
            album_art_data_ptr data = aaeiv2_stub->query( artTypeGuid, abort );
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
        std::unique_ptr<AlbumArtFetchTask> task( new AlbumArtFetchTask( hWnd, handle, art_id, need_stub, only_embed, no_load ) );
        uint32_t taskId = [&]()
        {
            uint64_t tmp = reinterpret_cast<uint64_t>(task.get());
            return static_cast<uint32_t>((tmp & 0xFFFFFFFF) ^ (tmp >> 32));
        }();

        if ( simple_thread_pool::instance().enqueue( std::move( task ) ) )
        {
            return taskId;
        }
    }
    catch ( ... )
    {
    }

    return 0;
}

}
