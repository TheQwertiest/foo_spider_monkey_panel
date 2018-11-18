#pragma once

#include <optional>
#include <string>

namespace mozjs::art
{

JSObject* GetAlbumArtPromise( JSContext* cx, uint32_t hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load ) noexcept( false );

} // namespace mozjs::art
