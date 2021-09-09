#pragma once

#include <string>

namespace smp::utils
{

/// @throw qwr::QwrException
void ExecuteContextCommandByName( const qwr::u8string& name, const metadb_handle_list& p_handles, uint32_t flags );

/// @throw qwr::QwrException
void ExecuteMainmenuCommandByName( const qwr::u8string& name );

/// @throw qwr::QwrException
uint32_t GetMainmenuCommandStatusByName( const qwr::u8string& name );

} // namespace smp::utils
