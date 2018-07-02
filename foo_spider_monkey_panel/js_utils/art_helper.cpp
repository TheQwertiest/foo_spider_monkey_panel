#include <stdafx.h>
#include "art_helper.h"

#include <algorithm>

#include <Shlwapi.h>

namespace
{

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

    CComPtr<IStream> is( SHCreateMemStream( nullptr, 0 ) );
    if ( !is )
    {
        return nullptr;
    }

    ULONG bytes_written = 0;
    HRESULT hr = is->Write( data->get_ptr(), data->get_size(), &bytes_written );
    if ( !SUCCEEDED( hr ) || bytes_written != data->get_size() )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> bmp( new Gdiplus::Bitmap( is, PixelFormat32bppPARGB ) );
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

}
