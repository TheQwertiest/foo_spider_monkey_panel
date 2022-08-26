#include <stdafx.h>

#include "builtin_modules.h"

namespace mozjs
{

std::optional<BuiltinModuleId> ResolveBuiltinModule( const qwr::u8string& moduleName )
{
    if ( moduleName == "smp:fb/playback" )
    {
        return BuiltinModuleId::kFbPlaybackControl;
    }

    return std::nullopt;
}

} // namespace mozjs
