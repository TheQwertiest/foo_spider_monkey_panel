#pragma once

namespace smp
{

/// @details These messages are synchronous
enum class InternalSyncMessage : UINT
{
    first_message = WM_USER + 100,
    notify_data = first_message,
    script_fail,
    terminate_script,
    timer_proc,
    ui_script_editor_saved,
    wnd_drag_drop,
    wnd_drag_enter,
    wnd_drag_leave,
    wnd_drag_over,
    last_message = wnd_drag_over,
};

/// @brief Message definitions that are not handled by the main panel window
enum class MiscMessage : UINT
{
    heartbeat = static_cast<int>( InternalSyncMessage::last_message ) + 1,
    key_down,
    run_next_event,
    size_limit_changed
};

template <typename T>
bool IsInEnumRange( UINT value )
{
    return ( value >= static_cast<UINT>( T::first_message )
             && value <= static_cast<UINT>( T::last_message ) );
}

} // namespace smp
