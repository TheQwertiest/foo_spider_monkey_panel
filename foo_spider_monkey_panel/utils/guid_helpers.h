#pragma once

#include <optional>

namespace smp::utils
{

[[nodiscard]] GUID GenerateGuid();
[[nodiscard]] std::wstring GuidToStr( const GUID& guid );
[[nodiscard]] std::optional<GUID> StrToGuid( const std::wstring& str );

} // namespace smp::utils
