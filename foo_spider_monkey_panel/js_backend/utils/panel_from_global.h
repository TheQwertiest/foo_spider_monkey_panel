#pragma once

#include <panel/panel_fwd.h>

namespace mozjs
{

[[nodiscard]] smp::not_null_shared<smp::panel::PanelAccessor>
GetHostPanelForCurrentGlobal( JSContext* cx );
[[nodiscard]] bool HasHostPanelForCurrentGlobal( JSContext* cx );

// TODO: remove
[[nodiscard]] HWND GetPanelHwndForCurrentGlobal( JSContext* cx );

} // namespace mozjs
