#include <stdafx.h>
#include "aero.h"

#include <js_engine/js_to_native_invoker.h>
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
    JsAero::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Aero",
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

const JSClass JsAero::JsClass = jsClass;
const JSFunctionSpec* JsAero::JsFunctions = jsFunctions;
const JSPropertySpec* JsAero::JsProperties = jsProperties;

JsAero::JsAero( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsAero::~JsAero()
{  
}

std::unique_ptr<JsAero> 
JsAero::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsAero>( new JsAero(cx) );
}

size_t JsAero::GetInternalSize()
{
    return 0;
}


}
