#include <stdafx.h>

#include "selection_holder_helper.h"

namespace
{

const std::array<const GUID*, 7> guids = {
    &contextmenu_item::caller_undefined,
    &contextmenu_item::caller_active_playlist_selection,
    &contextmenu_item::caller_active_playlist,
    &contextmenu_item::caller_playlist_manager,
    &contextmenu_item::caller_now_playing,
    &contextmenu_item::caller_keyboard_shortcut_list,
    &contextmenu_item::caller_media_library_viewer
};

}

namespace smp
{

std::optional<GUID> GetSelectionHolderGuidFromType( uint8_t typeId )
{
    if ( typeId >= guids.size() )
    {
        return std::nullopt;
    }

    return *guids[typeId];
}

std::optional<uint8_t> GetSelectionHolderTypeFromGuid( const GUID& typeGuid )
{
    const auto it = ranges::find_if( guids, [&typeGuid]( const auto pGuid ) { return typeGuid == *pGuid; } );
    if ( ranges::end( guids ) == it )
    {
        return std::nullopt;
    }

    return ranges::distance( ranges::begin( guids ), it );
}

} // namespace smp
