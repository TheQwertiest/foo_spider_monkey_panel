#pragma once

namespace smp::panel
{ 

// TODO: consider removing fromhook
struct metadb_callback_data
{
    metadb_handle_list m_items;
    bool m_fromhook;

    metadb_callback_data( const metadb_handle_list& p_items, bool p_fromhook )
        : m_items( p_items )
        , m_fromhook( p_fromhook )
    {
    }
};

} // namespace smp::panel
