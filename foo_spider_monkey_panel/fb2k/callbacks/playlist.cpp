#include <stdafx.h>

#include <fb2k/playlist_index_manager.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

class PlaylistCallbackImpl : public playlist_callback_static
{
public:
    PlaylistCallbackImpl();

    unsigned get_flags() final;
    void on_default_format_changed() final;
    void on_item_ensure_visible( t_size p_playlist, t_size p_idx ) final;
    void on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to ) final;
    void on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection ) final;
    void on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) final;
    void on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) final;
    void on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count ) final;
    void on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data ) final;
    void on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state ) final;
    void on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask ) final;
    void on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level ) final;
    void on_playback_order_changed( t_size p_new_index ) final;
    void on_playlist_activate( t_size p_old, t_size p_new ) final;
    void on_playlist_created( t_size p_index, const char* p_name, t_size p_name_len ) final;
    void on_playlist_locked( t_size p_playlist, bool p_locked ) final;
    void on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len ) final;
    void on_playlists_removing( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) final;
    void on_playlists_removed( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) final;
    void on_playlists_reorder( const t_size* p_order, t_size p_count ) final;

private:
    PlaylistIndexManager& playlistIndexManager_;
};

} // namespace

namespace
{

PlaylistCallbackImpl::PlaylistCallbackImpl()
    : playlistIndexManager_( PlaylistIndexManager::Get() )
{
}

unsigned PlaylistCallbackImpl::get_flags()
{
    return flag_on_items_added | flag_on_items_reordered | flag_on_items_removed | flag_on_items_selection_change | flag_on_item_focus_change | flag_on_item_ensure_visible | flag_on_playlist_activate | flag_on_playlist_created | flag_on_playlists_reorder | flag_on_playlists_removed | flag_on_playlist_renamed | flag_on_playback_order_changed | flag_on_playlist_locked;
}

void PlaylistCallbackImpl::on_default_format_changed()
{
}

void PlaylistCallbackImpl::on_item_ensure_visible( t_size p_playlist, t_size p_idx )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemEnsureVisible ) );
}

void PlaylistCallbackImpl::on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemFocusChange ) );
}

void PlaylistCallbackImpl::on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemsAdded ) );
}

void PlaylistCallbackImpl::on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void PlaylistCallbackImpl::on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemsRemoved ) );
}

void PlaylistCallbackImpl::on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemsReordered ) );
}

void PlaylistCallbackImpl::on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemsReplaced ) );
}

void PlaylistCallbackImpl::on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemsSelectionChange ) );
}

void PlaylistCallbackImpl::on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistItemsModified ) );
}

void PlaylistCallbackImpl::on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level )
{
    // TODO: test if this is covered by playback events
}

void PlaylistCallbackImpl::on_playback_order_changed( t_size p_new_index )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistPlaybackOrderChanged ) );
}

void PlaylistCallbackImpl::on_playlist_activate( t_size p_old, t_size p_new )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistActivate ) );
}

void PlaylistCallbackImpl::on_playlist_created( t_size p_index, const char* p_name, t_size p_name_len )
{
    playlistIndexManager_.OnPlaylistAdded( p_index );
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistCreated ) );
}

void PlaylistCallbackImpl::on_playlist_locked( t_size p_playlist, bool p_locked )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistLocked ) );
}

void PlaylistCallbackImpl::on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistRenamed ) );
}

void PlaylistCallbackImpl::on_playlists_removing( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void PlaylistCallbackImpl::on_playlists_removed( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    playlistIndexManager_.OnPlaylistRemoved();
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistsRemoved ) );
}

void PlaylistCallbackImpl::on_playlists_reorder( const t_size* p_order, t_size p_count )
{
    playlistIndexManager_.OnPlaylistsReordered( { p_order, p_count } );
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbPlaylistsReorder ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( PlaylistCallbackImpl );

} // namespace
