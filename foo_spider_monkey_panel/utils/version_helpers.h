#pragma once

#include <string>

namespace smp::version
{

/// @brief Performs comparison of semver strings
/// @return true, if string 'a' represents version newer than 'b'
bool IsNewer( const std::string& a, const std::string& b );

}
