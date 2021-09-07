#include <stdafx.h>

#include "logging.h"

namespace smp::utils
{

void LogError( const qwr::u8string& message )
{
    FB2K_console_formatter() << fmt::format(
        SMP_UNDERSCORE_NAME ":\n"
                            "Error:\n"
                            "{}\n ",
        message );
}

void LogWarning( const qwr::u8string& message )
{
    FB2K_console_formatter() << fmt::format(
        SMP_UNDERSCORE_NAME ":\n"
                            "Warning:\n"
                            "{}\n",
        message );
}

void LogDebug( const qwr::u8string& message )
{
    FB2K_console_formatter() << fmt::format(
        SMP_UNDERSCORE_NAME ":\n"
                            "Debug:\n"
                            "{}\n",
        message );
}

} // namespace smp::utils
