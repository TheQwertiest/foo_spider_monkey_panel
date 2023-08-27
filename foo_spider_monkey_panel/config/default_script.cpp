#include <stdafx.h>

#include "default_script.h"

#include <resources/resource.h>
#include <utils/script_resource.h>

namespace smp::config
{

qwr::u8string GetDefaultScript()
{
    try
    {
        return utils::LoadScriptResource( IDR_DEFAULT_SCRIPT );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        return qwr::u8string{};
    }
}

} // namespace smp::config
