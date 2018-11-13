#include <stdafx.h>
#include "hacks.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_window.h>
#include <utils/gdi_error_helpers.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>



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
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsHacks::JsClass = jsClass;
const JSFunctionSpec* JsHacks::JsFunctions = jsFunctions;
const JSPropertySpec* JsHacks::JsProperties = jsProperties;

JsHacks::JsHacks( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsHacks::~JsHacks()
{  
}

std::unique_ptr<JsHacks> 
JsHacks::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsHacks>( new JsHacks(cx) );
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

}
