#include <stdafx.h>

#include "panel_from_global.h"

#include <js_backend/objects/core/global_object.h>
#include <panel/panel_accessor.h>
#include <panel/panel_window.h>

namespace mozjs
{

smp::not_null_shared<smp::panel::PanelAccessor> GetHostPanelForCurrentGlobal( JSContext* cx )
{
    assert( cx );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    const auto pNativeGlobal = JsGlobalObject::ExtractNative( cx, jsGlobal );
    assert( pNativeGlobal );

    return pNativeGlobal->GetHostPanel();
}

[[nodiscard]] bool HasHostPanelForCurrentGlobal( JSContext* cx )
{
    return !!GetHostPanelForCurrentGlobal( cx )->GetPanel();
}

HWND GetPanelHwndForCurrentGlobal( JSContext* cx )
{
    auto pPanel = GetHostPanelForCurrentGlobal( cx )->GetPanel();
    return ( pPanel ? pPanel->GetHWND() : nullptr );
}

} // namespace mozjs
