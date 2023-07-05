#pragma once

#include <optional>

namespace mozjs
{

enum class BuiltinModuleId : uint8_t
{
    kFbPlaybackControl,
    kFbUiSelectionManager,
    kDomWindow,
    kEvents,
    kDomCanvas,
    kFbTrack,
    kFbPlaylistManager,
    kFbLibrary,
    kFbTrackImageManager,
    kFbPlaybackQueue,
    kFbReplayGainManager,
    kCount
};

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName );

int GetBuiltinModuleResourceId( BuiltinModuleId moduleId );

} // namespace mozjs
