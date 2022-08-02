#pragma once

#include <utils/art_loading_options.h>

namespace mozjs::art
{

/// @throw smp::JsException
[[nodiscard]] JSObject* GetAlbumArtPromise( JSContext* cx,
                                            HWND hWnd,
                                            const metadb_handle_ptr& handle,
                                            const smp::art::LoadingOptions& options );

} // namespace mozjs::art
