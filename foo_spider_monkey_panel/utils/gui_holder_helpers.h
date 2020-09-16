#pragma once

namespace
{

using namespace smp;

const GUID& GetHolderGuiFromType( uint8_t type_id )
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

    SmpException::ExpectTrue( type_id < guids.size(), "Unknown Holder type_id: {}", type_id );

    return *guids[type_id];
}

} // namespace