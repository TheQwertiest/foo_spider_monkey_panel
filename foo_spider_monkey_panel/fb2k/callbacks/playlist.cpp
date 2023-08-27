#include <stdafx.h>

#include <fb2k/playlist_index_manager.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/multi_playlist_event.h>
#include <tasks/events/panel_event.h>
#include <tasks/events/playlist_event.h>
#include <tasks/events/playlist_item_event.h>
#include <tasks/events/playlist_multi_item_event.h>

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
    return flag_on_items_added
           | flag_on_items_reordered
           | flag_on_items_removing
           | flag_on_items_removed
           | flag_on_items_selection_change
           | flag_on_item_focus_change
           | flag_on_items_modified
           | flag_on_items_replaced
           | flag_on_item_ensure_visible
           | flag_on_playlist_activate
           | flag_on_playlist_created
           | flag_on_playlists_reorder
           | flag_on_playlists_removed
           | flag_on_playlist_renamed
           | flag_on_playback_order_changed
           | flag_on_playlist_locked;
}

void PlaylistCallbackImpl::on_default_format_changed()
{
}

void PlaylistCallbackImpl::on_item_ensure_visible( t_size p_playlist, t_size p_idx )
{
    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistItemEvent>( EventId::kNew_FbPlaylistItemEnsureVisible,
                                             p_playlist,
                                             p_idx ) );
}

void PlaylistCallbackImpl::on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to )
{
    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistEvent>( EventId::kNew_FbPlaylistItemFocusChange,
                                         p_playlist ) );
}

void PlaylistCallbackImpl::on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection )
{
    auto itemIndices = ranges::views::ints( p_start, static_cast<uint32_t>( p_start + p_data.size() ) ) | ranges::to<std::vector>();
    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistMultiItemEvent>( EventId::kNew_FbPlaylistItemsAdded,
                                                  p_playlist,
                                                  smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( itemIndices ) ) ) );
}

void PlaylistCallbackImpl::on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void PlaylistCallbackImpl::on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    std::vector<uint32_t> itemIndices;
    p_mask.for_each( true, 0, p_old_count, [&]( auto i ) { itemIndices.emplace_back( i ); } );

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistMultiItemEvent>( EventId::kNew_FbPlaylistItemsRemoved,
                                                  p_playlist,
                                                  smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( itemIndices ) ) ) );
}

void PlaylistCallbackImpl::on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count )
{
    std::span<const t_size> orderIndices( p_order, p_count );
    auto itemIndices = orderIndices | ranges::views::transform( []( auto i ) { return static_cast<uint32_t>( i ); } ) | ranges::to<std::vector>();

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistMultiItemEvent>( EventId::kNew_FbPlaylistItemsReordered,
                                                  p_playlist,
                                                  smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( itemIndices ) ) ) );
}

void PlaylistCallbackImpl::on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data )
{
    const auto stlList = qwr::pfc_x::Make_Stl_CRef( p_data );
    // TODO: check that p_data contains only changed items
    auto itemIndices = stlList
                       | ranges::views::transform( []( const auto& elem ) { return static_cast<uint32_t>( elem.m_index ); } )
                       | ranges::to<std::vector>();

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistMultiItemEvent>( EventId::kNew_FbPlaylistItemsReplaced,
                                                  p_playlist,
                                                  smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( itemIndices ) ) ) );
}

void PlaylistCallbackImpl::on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state )
{
    const auto api = playlist_manager::get();

    std::vector<uint32_t> itemIndices;
    p_affected.for_each( true, 0, api->playlist_get_item_count( p_playlist ), [&]( auto i ) { itemIndices.emplace_back( i ); } );

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistMultiItemEvent>( EventId::kNew_FbPlaylistItemsSelectionChange,
                                                  p_playlist,
                                                  smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( itemIndices ) ) ) );
}

void PlaylistCallbackImpl::on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask )
{
    const auto api = playlist_manager::get();

    std::vector<uint32_t> itemIndices;
    p_mask.for_each( true, 0, api->playlist_get_item_count( p_playlist ), [&]( auto i ) { itemIndices.emplace_back( i ); } );

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<PlaylistMultiItemEvent>( EventId::kNew_FbPlaylistItemsModified,
                                                  p_playlist,
                                                  smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( itemIndices ) ) ) );
}

void PlaylistCallbackImpl::on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level )
{ // covered by on_items_modified
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
    EventDispatcher::Get().PutEventToAll( std::make_unique<PlaylistEvent>( EventId::kNew_FbPlaylistCreated, p_index ) );
}

void PlaylistCallbackImpl::on_playlist_locked( t_size p_playlist, bool p_locked )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PlaylistEvent>( EventId::kNew_FbPlaylistLocked, p_playlist ) );
}

void PlaylistCallbackImpl::on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PlaylistEvent>( EventId::kNew_FbPlaylistRenamed, p_index ) );
}

void PlaylistCallbackImpl::on_playlists_removing( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void PlaylistCallbackImpl::on_playlists_removed( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    playlistIndexManager_.OnPlaylistRemoved();

    std::vector<uint32_t> playlistIndices;
    p_mask.for_each( true, 0, p_old_count, [&]( auto i ) { playlistIndices.emplace_back( i ); } );

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<MultiPlaylistEvent>( EventId::kNew_FbPlaylistsRemoved,
                                              smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( playlistIndices ) ) ) );
}

void PlaylistCallbackImpl::on_playlists_reorder( const t_size* p_order, t_size p_count )
{
    playlistIndexManager_.OnPlaylistsReordered( { p_order, p_count } );

    std::span<const t_size> orderIndices( p_order, p_count );
    auto playlistIndices = orderIndices | ranges::views::transform( [&]( auto i ) { return static_cast<uint32_t>( i ); } ) | ranges::to<std::vector>();

    EventDispatcher::Get().PutEventToAll(
        std::make_unique<MultiPlaylistEvent>( EventId::kNew_FbPlaylistsReorder,
                                              smp::make_not_null_shared<const std::vector<uint32_t>>( std::move( playlistIndices ) ) ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( PlaylistCallbackImpl );

} // namespace
