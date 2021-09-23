#pragma once

namespace smp
{

/// @details These messages are synchronous
enum class InternalSyncMessage : UINT
{
    first_message = WM_USER + 100,
    legacy_notify_others = first_message,
    prepare_for_exit,
    run_next_event,
    script_fail,
    ui_script_editor_saved,
    wnd_drag_drop,
    wnd_drag_enter,
    wnd_drag_leave,
    wnd_drag_over,
    wnd_internal_drag_start,
    wnd_internal_drag_stop,
    last_message = wnd_internal_drag_stop,
};

/// @brief Message definitions that are not handled by the main panel window
enum class MiscMessage : UINT
{
    heartbeat = static_cast<int>( InternalSyncMessage::last_message ) + 1,
    key_down,
    size_limit_changed
};

template <typename T>
bool IsInEnumRange( UINT value )
{
    return ( value >= static_cast<UINT>( T::first_message )
             && value <= static_cast<UINT>( T::last_message ) );
}

} // namespace smp
