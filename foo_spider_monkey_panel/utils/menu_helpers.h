#pragma once

#include <string>

namespace smp::utils
{

bool execute_context_command_by_name( const std::u8string& name, const metadb_handle_list& p_handles, unsigned flags );
bool execute_mainmenu_command_by_name( const std::u8string& name );

/// @throw smp::SmpException
void get_mainmenu_command_status_by_name( const std::u8string& name, uint32_t& status );

} // namespace smp::utils
