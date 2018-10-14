#pragma once

#include <optional>
#include <string>

namespace mozjs::file
{

std::wstring CleanPath( const std::wstring &path );
std::wstring ReadFromFile( JSContext* cx, const pfc::string8_fast& path, uint32_t codepage = 0 ) noexcept(false);

}
