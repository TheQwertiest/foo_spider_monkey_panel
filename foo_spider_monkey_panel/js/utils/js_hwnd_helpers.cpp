#include <stdafx.h>

#include "js_hwnd_helpers.h"

#include <js/objects/core/global_object.h>

namespace mozjs
{

HWND GetPanelHwndForCurrentGlobal( JSContext* cx )
{
    assert( cx );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    const auto pNativeGlobal = JsGlobalObject::ExtractNative( cx, jsGlobal );
    assert( pNativeGlobal );

    return pNativeGlobal->GetPanelHwnd();
}

} // namespace mozjs
