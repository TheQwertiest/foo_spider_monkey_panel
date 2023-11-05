#pragma once

#include <optional>

namespace smp::utils
{

[[nodiscard]] GUID GenerateGuid();
[[nodiscard]] std::wstring GuidToStr( const GUID& guid, bool stripBraces = false );
[[nodiscard]] std::optional<GUID> StrToGuid( std::wstring_view str );

struct GuidHasher
{
    size_t operator()( const GUID& guid ) const;
};

} // namespace smp::utils
