#pragma once

#include <tuple>

namespace smp::config
{

/// @throw qwr::QwrException
void VerifyModulePackageName( const qwr::u8string& name );

/// @throw qwr::QwrException
std::tuple<qwr::u8string, qwr::u8string> ParseBareModuleSpecifier( const qwr::u8string& moduleSpecifier );

} // namespace smp::config
