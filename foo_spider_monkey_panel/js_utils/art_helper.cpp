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

private:
    virtual void run();

private:
    metadb_handle_ptr handle_;
    std::string rawPath_;
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
        rawPath_.assign( handle_->get_path() );
    }
}

void AlbumArtFetchTask::run()
{
    std::string imagePath;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( handle_.is_valid() )
    {
        if ( onlyEmbed_ )
        {
            bitmap.reset( art::GetBitmapFromEmbeddedData( rawPath_, artId_ ) );
            if ( bitmap )
            {
                imagePath = handle_->get_path();
            }
        }
        else
        {
            bitmap.reset( art::GetBitmapFromMetadb( handle_, artId_, needStub_, noLoad_, &imagePath ) );
        }
    }

    art::AsyncArtTaskResult taskResult;
    taskResult.handle = handle_;
    taskResult.artId = artId_;
    taskResult.bitmap.reset( bitmap.release() );
    taskResult.imagePath = imagePath.empty() ? "" : file_path_display( imagePath.c_str() );
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

    return *guids[std::clamp<uint32_t>( art_id, 0, _countof( guids ) - 1 )];
}

Gdiplus::Bitmap* GetBitmapFromAlbumArtData( const album_art_data_ptr& data )
{
    if ( !data.is_valid() )
    {
        return nullptr;
    }

    pfc::com_ptr_t<IStream> is( SHCreateMemStream( nullptr, 0 ) );
    if ( !is.is_valid() )
    {
        return nullptr;
    }

    ULONG bytes_written = 0;
    HRESULT hr = is->Write( data->get_ptr(), data->get_size(), &bytes_written );
    if ( !SUCCEEDED( hr ) || bytes_written != data->get_size() )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> bmp( new Gdiplus::Bitmap( is.get_ptr(), PixelFormat32bppPARGB ) );
    if ( !IsGdiplusObjectValid( bmp.get() ) )
    {
        return nullptr;
    }

    return bmp.release();
}

Gdiplus::Bitmap* ExtractBitmap( album_art_extractor_instance_v2::ptr extractor, GUID& artTypeGuid, bool no_load, std::string* pImagePath )
{
    abort_callback_dummy abort;
    album_art_data_ptr data = extractor->query( artTypeGuid, abort );
    std::unique_ptr<Gdiplus::Bitmap> bitmap;

    if ( !no_load )
    {
        bitmap.reset( GetBitmapFromAlbumArtData( data ) );
    }

    if ( pImagePath && ( no_load || bitmap ) )
    {
        album_art_path_list::ptr pathlist = extractor->query_paths( artTypeGuid, abort );
        if ( pathlist->get_count() > 0 )
        {
            pImagePath->assign( pathlist->get_path( 0 ) );
        }
    }

    return bitmap.release();
}

}

namespace mozjs::art
{

Gdiplus::Bitmap* GetBitmapFromEmbeddedData( const std::string& rawpath, uint32_t art_id )
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

            Gdiplus::Bitmap* bitmap = GetBitmapFromAlbumArtData( data );
            if ( bitmap )
            {
                return bitmap;
            }
        }
        catch ( ... )
        {
        }
    }
    
    return nullptr;
}

Gdiplus::Bitmap* GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, std::string* pImagePath )
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

        if ( simple_thread_pool::instance().enqueue( task.get() ) )
        {
            uint64_t taskId = reinterpret_cast<uint64_t>(task.release());
            return static_cast<uint32_t>( ( taskId & 0xFFFFFFFF ) ^ ( taskId >> 32 ) );
        }
    }
    catch ( ... )
    {
    }

    return 0;
}

}
