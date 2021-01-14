#pragma once

#include <optional>

namespace smp
{

std::optional<GUID> GetSelectionHolderGuidFromType( uint8_t typeId );
std::optional<uint8_t> GetSelectionHolderTypeFromGuid( const GUID& typeGuid );

} // namespace smp
