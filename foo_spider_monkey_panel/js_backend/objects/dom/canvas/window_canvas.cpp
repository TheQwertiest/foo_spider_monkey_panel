#include <stdafx.h>

#include "window_canvas.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_rendering_context_2d.h>
#include <utils/gdi_error_helpers.h>

using namespace smp;

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
    WindowCanvas::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "WindowCanvas",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetContext, WindowCanvas::GetContext, WindowCanvas::GetContextWithOpt, 1 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getContext", GetContext, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Height, mozjs::WindowCanvas::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Width, mozjs::WindowCanvas::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "height", Get_Height, kDefaultPropsFlags ),
        JS_PSG( "width", Get_Width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::WindowCanvas );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<WindowCanvas>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<WindowCanvas>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<WindowCanvas>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<WindowCanvas>::PrototypeId = JsPrototypeId::New_WindowCanvas;

WindowCanvas::WindowCanvas( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height )
    : pJsCtx_( cx )
    , heapHelper_( cx )
    , pGraphics_( &graphics )
    , width_( width )
    , height_( height )
{
}

WindowCanvas::~WindowCanvas()
{
    heapHelper_.Finalize();
}

std::unique_ptr<WindowCanvas>
WindowCanvas::CreateNative( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height )
{
    return std::unique_ptr<WindowCanvas>( new WindowCanvas( cx, graphics, width, height ) );
}

size_t WindowCanvas::GetInternalSize()
{
    return 0;
}

JSObject* WindowCanvas::GetContext( const std::wstring& contextType, JS::HandleValue attributes )
{
    qwr::QwrException::ExpectTrue( contextType == L"2d", L"Unsupported contextType value: {}", contextType );

    // TODO: handle attributes

    JS::RootedObject jsRenderingContext( pJsCtx_ );
    if ( !jsRenderingContextHeapIdOpt_ )
    {
        jsRenderingContext = JsObjectBase<CanvasRenderingContext2d>::CreateJs( pJsCtx_, *pGraphics_ );
        jsRenderingContextHeapIdOpt_ = heapHelper_.Store( jsRenderingContext );
    }
    else
    {
        jsRenderingContext.set( &heapHelper_.Get( *jsRenderingContextHeapIdOpt_ ).toObject() );
    }

    return jsRenderingContext;
}

JSObject* WindowCanvas::GetContextWithOpt( size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes )
{
    switch ( optArgCount )
    {
    case 0:
        return GetContext( contextType, attributes );
    case 1:
        return GetContext( contextType );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t WindowCanvas::get_Height() const
{
    return height_;
}

uint32_t WindowCanvas::get_Width() const
{
    return width_;
}

} // namespace mozjs
