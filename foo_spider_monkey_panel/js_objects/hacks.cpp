#include <stdafx.h>
#include "hacks.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_window.h>
#include <js_utils/gdi_error_helper.h>
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

MJS_DEFINE_JS_TO_NATIVE_FN( JsHacks, GetFbWindow )

const JSFunctionSpec jsFunctions[] = {
    JS_FN("GetFbWindow", GetFbWindow, 0, DefaultPropsFlags()),
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


std::optional<JSObject*> JsHacks::GetFbWindow()
{
    JS::RootedObject jsObject( pJsCtx_, JsFbWindow::CreateJs( pJsCtx_ ) );
    if ( !jsObject )
    {// reports
        return std::nullopt;
    }

    return jsObject;
}

}
