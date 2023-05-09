#include <stdafx.h>

#include "builtin_modules.h"

#include <resources/resource.h>

namespace mozjs
{

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName )
{
    if ( moduleName == "smp:fb/playback" )
    {
        return BuiltinModuleId::kFbPlaybackControl;
    }
    if ( moduleName == "smp:fb/selection" )
    {
        return BuiltinModuleId::kFbSelectionManager;
    }
    if ( moduleName == "smp:window" )
    {
        return BuiltinModuleId::kWindow;
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
    default:
        assert( false );
        return 0;
    }
}

} // namespace mozjs
