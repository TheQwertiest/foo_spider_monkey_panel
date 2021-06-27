#pragma once

#include <optional>

namespace smp
{

std::optional<qwr::u8string> LoadStringResource( int resourceId, const char* resourceType );

} // namespace smp
