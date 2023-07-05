#include <stdafx.h>

#include "builtin_modules.h"

#include <resources/resource.h>

namespace mozjs
{

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName )
{
    if ( moduleName == "smp:dom/window" )
    {
        return BuiltinModuleId::kDomWindow;
    }
    if ( moduleName == "smp:dom/canvas" )
    {
        return BuiltinModuleId::kDomCanvas;
    }
    if ( moduleName == "smp:fb/playback" )
    {
        return BuiltinModuleId::kFbPlaybackControl;
    }
    if ( moduleName == "smp:fb/ui-selection-manager" )
    {
        return BuiltinModuleId::kFbUiSelectionManager;
    }
    if ( moduleName == "smp:fb/track" )
    {
        return BuiltinModuleId::kFbTrack;
    }
    if ( moduleName == "smp:fb/playlist-manager" )
    {
        return BuiltinModuleId::kFbPlaylistManager;
    }
    if ( moduleName == "smp:fb/library" )
    {
        return BuiltinModuleId::kFbLibrary;
    }
    if ( moduleName == "smp:fb/track-image-manager" )
    {
        return BuiltinModuleId::kFbTrackImageManager;
    }
    if ( moduleName == "smp:fb/playback-queue" )
    {
        return BuiltinModuleId::kFbPlaybackQueue;
    }
    if ( moduleName == "smp:fb/replay-gain-manager" )
    {
        return BuiltinModuleId::kFbReplayGainManager;
    }

    return std::nullopt;
}

int GetBuiltinModuleResourceId( BuiltinModuleId moduleId )
{
    switch ( moduleId )
    {
    case mozjs::BuiltinModuleId::kFbPlaybackControl:
        return IDR_MODULE_FB_PLAYBACK;
    case mozjs::BuiltinModuleId::kFbUiSelectionManager:
        return IDR_MODULE_FB_UI_SELECTION_MANAGER;
    case mozjs::BuiltinModuleId::kDomWindow:
        return IDR_MODULE_DOM_WINDOW;
    case mozjs::BuiltinModuleId::kDomCanvas:
        return IDR_MODULE_DOM_CANVAS;
    case mozjs::BuiltinModuleId::kFbTrack:
        return IDR_MODULE_FB_TRACK;
    case mozjs::BuiltinModuleId::kFbPlaylistManager:
        return IDR_MODULE_FB_PLAYLIST_MANAGER;
    case mozjs::BuiltinModuleId::kFbLibrary:
        return IDR_MODULE_FB_LIBRARY;
    case mozjs::BuiltinModuleId::kFbTrackImageManager:
        return IDR_MODULE_FB_TRACK_IMAGE_MANAGER;
    case mozjs::BuiltinModuleId::kFbPlaybackQueue:
        return IDR_MODULE_FB_PLAYBACK_QUEUE;
    case mozjs::BuiltinModuleId::kFbReplayGainManager:
        return IDR_MODULE_FB_REPLAY_GAIN_MANAGER;
    default:
        assert( false );
        return 0;
    }
}

} // namespace mozjs
