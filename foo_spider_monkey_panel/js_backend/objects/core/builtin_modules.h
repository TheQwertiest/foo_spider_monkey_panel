#pragma once

#include <optional>

namespace mozjs
{

enum class BuiltinModuleId : uint8_t
{
    kDomCanvas,
    kDomWindow,
    kEvents,
    kFbDspManager,
    kFbLibrary,
    kFbOutputManager,
    kFbPlaybackControl,
    kFbPlaybackQueue,
    kFbPlaylistManager,
    kFbReplayGainManager,
    kFbTrack,
    kFbTrackCustomMetaManager,
    kFbTrackImageManager,
    kFbUiSelectionManager,
    kCount
};

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName );

int GetBuiltinModuleResourceId( BuiltinModuleId moduleId );

} // namespace mozjs
