#include <stdafx.h>

#include "_placeholder.h"

#include <js_engine/js_to_native_invoker.h>
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
    JsFinalizeOp<JsPlaceholder>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiFont",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsPlaceholder, get_Height )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, 0 ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_PSG( "Style",  get_Style, 2, 0 ),
    JS_FS_END
};

}

namespace mozjs
{


JsPlaceholder::JsPlaceholder( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsPlaceholder::~JsPlaceholder()
{
}

JSObject* JsPlaceholder::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsPlaceholder( cx ) );

    return jsObj;
}

const JSClass& JsPlaceholder::GetClass()
{
    return jsClass;
}

}
