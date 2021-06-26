#pragma once

#include <optional>

namespace smp::utils
{

[[nodiscard]] GUID GenerateGuid();
[[nodiscard]] std::wstring GuidToStr( const GUID& guid );
[[nodiscard]] std::optional<GUID> StrToGuid( const std::wstring& str );

struct GuidHasher
{
    size_t operator()( const GUID& guid ) const;
};

} // namespace smp::utils
