#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>

namespace mozjs::art
{

Gdiplus::Bitmap* GetBitmapFromEmbeddedData( const std::string& rawpath, uint32_t art_id );
Gdiplus::Bitmap* GetBitmapFromMetadb( const metadb_handle_ptr& handle, uint32_t art_id, bool need_stub, bool no_load, std::string* pImagePath );

}
