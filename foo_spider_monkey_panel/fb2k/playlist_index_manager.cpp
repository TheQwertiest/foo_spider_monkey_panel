#include <stdafx.h>

#include "playlist_index_manager.h"

#include <component_guids.h>

#include <qwr/algorithm.h>

namespace smp
{

PlaylistIndexManager::PlaylistIndexManager()
{
}

PlaylistIndexManager& PlaylistIndexManager::Get()
{
    static PlaylistIndexManager pim;
    return pim;
}

uint64_t PlaylistIndexManager::GetId( uint32_t index ) const
{
    const auto api = playlist_manager_v6::get();
    assert( index < api->get_playlist_count() );

    static uint64_t currentId = 0;

    uint64_t id = 0;
    if ( !playlist_manager_v6::get()->playlist_get_property_int( index, guid::playlist_property_id, id ) )
    {
        id = currentId++;
        playlist_manager_v6::get()->playlist_set_property_int( index, guid::playlist_property_id, id );
    }

    idToIdx_[id] = index;

    return id;
}

std::optional<uint32_t> PlaylistIndexManager::GetIndex( uint64_t id ) const
{
    return qwr::FindAsOptional( idToIdx_, id );
}

void PlaylistIndexManager::OnPlaylistAdded( uint32_t index )
{
    const auto api = playlist_manager_v6::get();
    if ( index == api->get_playlist_count() - 1 )
    {
        return;
    }

    RefreshMapping();
}

void PlaylistIndexManager::OnPlaylistRemoved()
{
    RefreshMapping();
}

void PlaylistIndexManager::OnPlaylistsReordered( std::span<const size_t> /*indices*/ )
{
    RefreshMapping();
}

void PlaylistIndexManager::RefreshMapping()
{
    idToIdx_.clear();

    const auto api = playlist_manager_v6::get();
    for ( auto index: ranges::views::indices( api->get_playlist_count() ) )
    {
        uint64_t id = 0;
        if ( !playlist_manager_v6::get()->playlist_get_property_int( index, guid::playlist_property_id, id ) )
        {
            continue;
        }

        idToIdx_.try_emplace( id, index );
    }
}

} // namespace smp
