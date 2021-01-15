#include <stdafx.h>

#include "js_hwnd_helpers.h"

#include <js_objects/global_object.h>

namespace mozjs
{

HWND GetPanelHwndForCurrentGlobal( JSContext* cx )
{
    assert( cx );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    const auto pNativeGlobal = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
    assert( pNativeGlobal );

    return pNativeGlobal->GetPanelHwnd();
}

} // namespace mozjs
