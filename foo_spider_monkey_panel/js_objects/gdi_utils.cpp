#include <stdafx.h>

#include "gdi_utils.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>
#include <js_objects/js_object_wrapper.h>

#include <helpers.h>

// TODO: extract this macro and common object code somewhere

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

static JSClassOps gdiUtilsOps = {
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

static JSClass gdiUtilsClass = {
    "gdiUtils",
    JSCLASS_HAS_PRIVATE,
    &gdiUtilsOps
};

MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiUtils, Font )

static const JSFunctionSpec gdiUtilsFunctions[] = {
    JS_FN( "Font", Font, 3, 0 ),
    JS_FS_END
};

}

namespace mozjs
{


JsGdiUtils::JsGdiUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsGdiUtils::~JsGdiUtils()
{
}

JSObject* JsGdiUtils::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &gdiUtilsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, gdiUtilsFunctions ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiUtils( cx ) );

    return jsObj;
}

std::tuple<Mjs_Status, JsObjectWrapper<JsGdiFont>*> JsGdiUtils::Font( std::string fontName, float pxSize, int style )
{
    std::wstring wFontName (pfc::stringcvt::string_wide_from_utf8( fontName.c_str() ));
    // <codecvt> is deprecated in C++17...
    Gdiplus::Font* pGdiFont = new Gdiplus::Font( wFontName.data(), pxSize, style, Gdiplus::UnitPixel );
    if ( !helpers::ensure_gdiplus_object( pGdiFont ) )
    {
        if ( pGdiFont )
        {
            delete pGdiFont;
        }

        return { Mjs_InternalError, nullptr };
    }

    // Generate HFONT
    // The benefit of replacing Gdiplus::Font::GetLogFontW is that you can get it work with CCF/OpenType fonts.
    HFONT hFont = CreateFont(
        -(int)pxSize,
        0,
        0,
        0,
        (style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
        (style & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
        (style & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
        (style & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        wFontName.data() );
    
    // TODO: think about removing CurrentGlobalOrNull
    JS::RootedObject global( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
    JsObjectWrapper<JsGdiFont>* pJsFont = JsObjectWrapper<JsGdiFont>::Create( pJsCtx_, global, pGdiFont, hFont );
    if ( !pJsFont )
    {
        return { Mjs_InternalError, nullptr };
    }

    return { Mjs_Ok, pJsFont };
}

}
