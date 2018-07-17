#pragma once

#include <optional>
#include <string>

namespace mozjs::art
{

struct AsyncArtTaskResult
{
    metadb_handle_ptr handle;
    uint32_t artId = 0;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    pfc::string8_fast imagePath;
};

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const pfc::string8_fast& rawpath, uint32_t art_id );
std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, pfc::string8_fast* pImagePath );
uint32_t GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );

}
