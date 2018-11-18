#pragma once

namespace mozjs::art
{

JSObject* GetAlbumArtPromise( JSContext* cx, HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load ) noexcept( false );

} // namespace mozjs::art
