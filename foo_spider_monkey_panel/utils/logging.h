#pragma once

#include <string>

namespace smp::utils
{

void LogError( const qwr::u8string& message );
void LogWarning( const qwr::u8string& message );
void LogDebug( const qwr::u8string& message );

} // namespace smp::utils
