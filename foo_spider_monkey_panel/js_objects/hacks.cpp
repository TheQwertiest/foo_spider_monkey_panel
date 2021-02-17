#include <stdafx.h>

#include "hacks.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_window.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_error_helpers.h>

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsHacks::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Hacks",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsHacks::JsClass = jsClass;
const JSFunctionSpec* JsHacks::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsHacks::JsProperties = jsProperties.data();

JsHacks::JsHacks( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<JsHacks>
JsHacks::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsHacks>( new JsHacks( cx ) );
}

size_t JsHacks::GetInternalSize()
{
    return 0;
}

bool JsHacks::PostCreate( JSContext* cx, JS::HandleObject self )
{
    CreateAndInstallObject<JsFbWindow>( cx, self, "FbWindow" );
    return true;
}

} // namespace mozjs
