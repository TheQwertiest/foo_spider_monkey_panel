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
    if ( moduleName == "smp:fb/selection" )
    {
        return BuiltinModuleId::kFbSelectionManager;
    }
    if ( moduleName == "smp:fb/track" )
    {
        return BuiltinModuleId::kTrack;
    }

    return std::nullopt;
}

int GetBuiltinModuleResourceId( BuiltinModuleId moduleId )
{
    switch ( moduleId )
    {
    case mozjs::BuiltinModuleId::kFbPlaybackControl:
        return IDR_MODULE_FB_PLAYBACK;
    case mozjs::BuiltinModuleId::kFbSelectionManager:
        return IDR_MODULE_FB_SELECTION;
    case mozjs::BuiltinModuleId::kWindow:
        return IDR_MODULE_WINDOW;
    case mozjs::BuiltinModuleId::kCanvas:
        return IDR_MODULE_CANVAS;
    case mozjs::BuiltinModuleId::kTrack:
        return IDR_MODULE_TRACK;
    default:
        assert( false );
        return 0;
    }
}

} // namespace mozjs
