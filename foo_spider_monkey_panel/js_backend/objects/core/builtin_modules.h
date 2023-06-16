#pragma once

#include <optional>

namespace mozjs
{

enum class BuiltinModuleId : uint8_t
{
    kFbPlaybackControl,
    kFbSelectionManager,
    kWindow,
    kEvents,
    kCanvas,
    kTrack,
    kPlaylistManager,
    kCount
};

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName );

int GetBuiltinModuleResourceId( BuiltinModuleId moduleId );

} // namespace mozjs
