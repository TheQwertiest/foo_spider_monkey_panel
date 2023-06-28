#include <stdafx.h>

#include "builtin_modules.h"

#include <resources/resource.h>

namespace mozjs
{

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName )
{
    if ( moduleName == "smp:dom/window" )
    {
        return BuiltinModuleId::kWindow;
    }
    if ( moduleName == "smp:dom/canvas" )
    {
        return BuiltinModuleId::kCanvas;
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
        return BuiltinModuleId::kTrack;
    }
    if ( moduleName == "smp:fb/playlist-manager" )
    {
        return BuiltinModuleId::kPlaylistManager;
    }
    if ( moduleName == "smp:fb/library" )
    {
        return BuiltinModuleId::kLibrary;
    }
    if ( moduleName == "smp:fb/track-image-manager" )
    {
        return BuiltinModuleId::kTrackImageManager;
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
        return IDR_MODULE_FB_UI_SELECTION_MANGAGER;
    case mozjs::BuiltinModuleId::kWindow:
        return IDR_MODULE_DOM_WINDOW;
    case mozjs::BuiltinModuleId::kCanvas:
        return IDR_MODULE_DOM_CANVAS;
    case mozjs::BuiltinModuleId::kTrack:
        return IDR_MODULE_FB_TRACK;
    case mozjs::BuiltinModuleId::kPlaylistManager:
        return IDR_MODULE_FB_PLAYLIST_MANAGER;
    case mozjs::BuiltinModuleId::kLibrary:
        return IDR_MODULE_FB_LIBRARY;
    case mozjs::BuiltinModuleId::kTrackImageManager:
        return IDR_MODULE_FB_TRACK_IMAGE_MANAGER;
    default:
        assert( false );
        return 0;
    }
}

} // namespace mozjs
