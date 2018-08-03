#include <stdafx.h>
#include "html_window.h"

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
    JsHtmlWindow::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "HtmlWindow",
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

//MJS_DEFINE_JS_TO_NATIVE_FN( JsHtmlWindow, get_Height )

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsHtmlWindow::JsClass = jsClass;
const JSFunctionSpec* JsHtmlWindow::JsFunctions = jsFunctions;
const JSPropertySpec* JsHtmlWindow::JsProperties = jsProperties;
const JsPrototypeId JsHtmlWindow::PrototypeId = JsPrototypeId::HtmlWindow;

JsHtmlWindow::JsHtmlWindow( JSContext* cx)
    : pJsCtx_( cx )
{
}

JsHtmlWindow::~JsHtmlWindow()
{ 
}

std::unique_ptr<JsHtmlWindow> 
JsHtmlWindow::CreateNative( JSContext* cx, const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{

    return std::unique_ptr<JsHtmlWindow>( new JsHtmlWindow(cx ) );
}

size_t JsHtmlWindow::GetInternalSize( const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    return htmlCode.length() * sizeof( wchar_t ) + data.length() * sizeof( wchar_t );
}

}
