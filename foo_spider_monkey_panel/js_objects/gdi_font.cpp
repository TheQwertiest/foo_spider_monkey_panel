#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>

#define MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK(baseClass, functionName) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        Mjs_Status mjsRet = InvokeNativeCallback( cx, &baseClass::functionName, argc, vp );\
        if (Mjs_Ok != mjsRet)\
        {\
            JS_ReportErrorASCII( cx, ErrorCodeToString( mjsRet ) );\
        }\
        return Mjs_Ok == mjsRet;\
    }

#define IF_GDI_FAILED_RETURN(x,y) \
    do \
    {\
        if ( x > 0 )\
        {\
            return y;\
        }\
    } while(false)

namespace
{

using namespace mozjs;

static JSClassOps gdiFontOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

static JSClass gdiFontClass = {
    "GdiFont",
    JSCLASS_HAS_PRIVATE,
    &gdiFontOps
};

static const JSFunctionSpec gdiFontFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{


JsGdiFont::JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont )
    : pJsCtx_( cx )
    , gdiFont_( pGdiFont )
    , hFont_( hFont )
{
}


JsGdiFont::~JsGdiFont()
{
}

JSObject* JsGdiFont::Create( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &gdiFontClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, gdiFontFunctions ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiFont( cx, pGdiFont, hFont ) );

    return jsObj;
}

}
