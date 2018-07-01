#include <stdafx.h>
#include "theme_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_graphics.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/winapi_error_helper.h>
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
    JsFinalizeOp<JsThemeManager>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ThemeManager",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsThemeManager, DrawThemeBackground )
MJS_DEFINE_JS_TO_NATIVE_FN( JsThemeManager, IsThemePartDefined )
MJS_DEFINE_JS_TO_NATIVE_FN( JsThemeManager, SetPartID )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "DrawThemeBackground", DrawThemeBackground, 9, DefaultPropsFlags() ),
    JS_FN( "IsThemePartDefined", IsThemePartDefined, 1, DefaultPropsFlags() ),
    JS_FN( "SetPartID", SetPartID, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

JsThemeManager::JsThemeManager( JSContext* cx, HWND hwnd, const std::wstring& classlist )
    : pJsCtx_( cx )
{
    // TODO: move to Create
    hTheme_ = OpenThemeData( hwnd, classlist.c_str() );

    if ( !hTheme_ ) throw pfc::exception_invalid_params();
}

JsThemeManager::~JsThemeManager()
{
    if ( hTheme_ )
    {
        CloseThemeData( hTheme_ );
    }
}

JSObject* JsThemeManager::Create( JSContext* cx, HWND hwnd, const std::wstring& classlist )
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

    JS_SetPrivate( jsObj, new JsThemeManager( cx, hwnd, classlist ) );

    return jsObj;
}

const JSClass& JsThemeManager::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t> 
JsThemeManager::DrawThemeBackground( JsGdiGraphics* gr, 
                                     int32_t x, int32_t y, uint32_t w, uint32_t h, 
                                     int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h )
{
    if ( !gr )
    {
        JS_ReportErrorASCII( pJsCtx_, "gr argument is null" );
        return std::nullopt;
    }

    Gdiplus::Graphics* graphics = gr->GetGraphicsObject();
    assert( graphics );

    HDC dc = graphics->GetHDC();
    assert( dc );

    RECT rc = { x, y, static_cast<LONG>(x + w), static_cast<LONG>(y + h)};
    RECT clip_rc = { clip_x, clip_y, static_cast<LONG>(clip_x + clip_y), static_cast<LONG>(clip_w + clip_h) };
    LPCRECT pclip_rc = &clip_rc;

    if ( !clip_x && !clip_y && !clip_w && !clip_h )
    {
        pclip_rc = nullptr;
    }

    HRESULT hr = ::DrawThemeBackground( hTheme_, dc, partId_, 0, &rc, pclip_rc );
    graphics->ReleaseHDC( dc );
    IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, std::nullopt, DrawThemeBackground );

    return nullptr;
}

std::optional<bool> 
JsThemeManager::IsThemePartDefined( int32_t partid, int32_t stateId )
{
    return ::IsThemePartDefined( hTheme_, partid, stateId );
}

std::optional<std::nullptr_t> 
JsThemeManager::SetPartID( int32_t partid )
{
    partId_ = partid;
    return nullptr;
}

std::optional<std::nullptr_t>
JsThemeManager::SetStateID( int32_t stateId )
{
    stateId_ = stateId;
    return nullptr;
}

}
