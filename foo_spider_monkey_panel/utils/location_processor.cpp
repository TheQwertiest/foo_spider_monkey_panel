#include <stdafx.h>
#include "location_processor.h"

namespace smp::utils
{

js_process_locations::js_process_locations( int playlist_idx, UINT base, bool to_select )
    : m_playlist_idx( playlist_idx )
    , m_base( base )
    , m_to_select( to_select )
{
}

void js_process_locations::on_completion( metadb_handle_list_cref p_items )
{
    pfc::bit_array_val selection( m_to_select );
    auto api = playlist_manager::get();
    t_size playlist = m_playlist_idx == -1 ? api->get_active_playlist() : m_playlist_idx;

    if ( playlist < api->get_playlist_count() && !api->playlist_lock_is_present( playlist ) )
    {
        api->playlist_insert_items( playlist, m_base, p_items, selection );
        if ( m_to_select )
        {
            api->set_active_playlist( playlist );
            api->playlist_set_focus_item( playlist, m_base );
        }
    }
}

void js_process_locations::on_aborted()
{
}

} // namespace smp::utils
