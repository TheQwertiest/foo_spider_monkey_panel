#pragma once

#include <optional>
#include <string>

namespace smp::art
{

class embed_thread
    : public threaded_process_callback
{
public:
    enum class EmbedAction : uint32_t
    {
        embed,
        remove,
        removeAll
    };

public:
    embed_thread( EmbedAction action,
                  album_art_data_ptr data,
                  const metadb_handle_list& handles,
                  GUID what );
    void run( threaded_process_status& p_status, abort_callback& p_abort ) override;

private:
    EmbedAction m_action;
    album_art_data_ptr m_data;
    metadb_handle_list m_handles;
    GUID m_what;
};

/// @throw qwr::QwrException
const GUID& GetGuidForArtId( uint32_t art_id );

/// @throw smp::JsException
std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromEmbeddedData( const qwr::u8string& rawpath, uint32_t art_id );

/// @throw qwr::QwrException
std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, qwr::u8string* pImagePath );

/// @details Validate art_id before calling this function!
std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromMetadbOrEmbed( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load, qwr::u8string* pImagePath );

/// @throw qwr::QwrException
/// @throw smp::JsException
void GetAlbumArtAsync( HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );

} // namespace smp::art
