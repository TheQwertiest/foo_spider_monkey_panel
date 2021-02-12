#pragma once

#include <optional>

namespace smp::utils
{

GUID GenerateGuid();
std::wstring GuidToStr( const GUID& guid );
std::optional<GUID> StrToGuid( const std::wstring& str );

} // namespace smp::utils
