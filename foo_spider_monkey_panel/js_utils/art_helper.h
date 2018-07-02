#pragma once

#include <optional>
#include <string>

namespace mozjs::art
{

struct AsyncArtTaskResult
{
    metadb_handle_ptr handle;
    uint32_t artId;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    std::string imagePath;
};

Gdiplus::Bitmap* GetBitmapFromEmbeddedData( const std::string& rawpath, uint32_t art_id );
Gdiplus::Bitmap* GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, std::string* pImagePath );
uint32_t GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );

}
