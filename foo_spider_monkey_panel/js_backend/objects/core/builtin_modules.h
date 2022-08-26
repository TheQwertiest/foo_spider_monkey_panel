#pragma once

#include <optional>

namespace mozjs
{

enum class BuiltinModuleId : uint8_t
{
    kFbPlaybackControl,
    kCount
};

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName );

} // namespace mozjs