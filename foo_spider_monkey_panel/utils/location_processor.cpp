#include <stdafx.h>

#include "location_processor.h"

namespace smp::utils
{

OnProcessLocationsNotify_InsertHandles::OnProcessLocationsNotify_InsertHandles( int playlistIdx, UINT baseIdx, bool shouldSelect )
    : playlistIdx_( playlistIdx )
    , baseIdx_( baseIdx )
    , shouldSelect_( shouldSelect )
{
}

void OnProcessLocationsNotify_InsertHandles::on_completion( metadb_handle_list_cref items )
{
    auto api = playlist_manager::get();
    const size_t adjustedPlIdx = ( playlistIdx_ == -1 ? api->get_active_playlist() : playlistIdx_ );
    if ( adjustedPlIdx >= api->get_playlist_count() 
        || ( api->playlist_lock_get_filter_mask( adjustedPlIdx ) & playlist_lock::filter_add ) )
    {
        return;
    }

    pfc::bit_array_val selection( shouldSelect_ );
    api->playlist_insert_items( adjustedPlIdx, baseIdx_, items, selection );
    if ( shouldSelect_ )
    {
        api->set_active_playlist( adjustedPlIdx );
        api->playlist_set_focus_item( adjustedPlIdx, baseIdx_ );
    }
}

void OnProcessLocationsNotify_InsertHandles::on_aborted()
{
}

} // namespace smp::utils
