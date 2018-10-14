#pragma once

#include <optional>
#include <string>

namespace mozjs::art
{

class embed_thread 
    : public threaded_process_callback
{
public:
    embed_thread( t_size action,
                  album_art_data_ptr data,
                  metadb_handle_list_cref handles,
                  GUID what );
    void run( threaded_process_status& p_status, abort_callback& p_abort );

private:
    t_size m_action; // 0 embed, 1 remove
    album_art_data_ptr m_data;
    metadb_handle_list m_handles;
    GUID m_what;
};

struct AsyncArtTaskResult
{
    metadb_handle_ptr handle;
    uint32_t artId = 0;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    pfc::string8_fast imagePath;
};

const GUID& GetGuidForArtId( uint32_t art_id ) noexcept(false);
std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const pfc::string8_fast& rawpath, uint32_t art_id ) noexcept(false);
std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, pfc::string8_fast* pImagePath ) noexcept(false);
uint32_t GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load ) noexcept(false);

}
