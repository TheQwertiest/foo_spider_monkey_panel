#pragma once

#include <string>

namespace smp::utils
{

bool execute_context_command_by_name( const pfc::string8_fast& name, const metadb_handle_list& p_handles, unsigned flags );
bool execute_mainmenu_command_by_name( const pfc::string8_fast& name );
void get_mainmenu_command_status_by_name( const pfc::string8_fast& name, uint32_t& status ) noexcept( false );

} // namespace smp::utils
