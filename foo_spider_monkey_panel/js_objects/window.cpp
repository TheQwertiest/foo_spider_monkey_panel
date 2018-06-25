#include <stdafx.h>

#include "window.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>
#include <js_objects/gdi_error.h>
#include <js_utils/js_utils.h>


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
    JsFinalizeOp<JsGdiUtils>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Window",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{


JsWindow::JsWindow( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsWindow::~JsWindow()
{
}

JSObject* JsWindow::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties(cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsWindow( cx ) );

    return jsObj;
}

const JSClass& JsWindow::GetClass()
{
    return jsClass;
}

}
