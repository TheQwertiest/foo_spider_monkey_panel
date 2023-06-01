#include <stdafx.h>

#include "window_canvas.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_rendering_context_2d.h>
#include <utils/gdi_error_helpers.h>

#include <qwr/utility.h>

using namespace smp;

namespace
{

enum class ReservedSlots
{
    kRenderingContext = mozjs::kReservedObjectSlot + 1
};

}

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
    DefaultClassFlags( 1 ),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT_AND_SELF( getContext, WindowCanvas::GetContext, WindowCanvas::GetContextWithOpt, 1 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getContext", getContext, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, mozjs::WindowCanvas::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, mozjs::WindowCanvas::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "height", get_height, kDefaultPropsFlags ),
        JS_PSG( "width", get_width, kDefaultPropsFlags ),
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
    , pGraphics_( &graphics )
    , width_( width )
    , height_( height )
{
}

WindowCanvas::~WindowCanvas()
{
}

std::unique_ptr<WindowCanvas>
WindowCanvas::CreateNative( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height )
{
    return std::unique_ptr<WindowCanvas>( new WindowCanvas( cx, graphics, width, height ) );
}

size_t WindowCanvas::GetInternalSize() const
{
    return 0;
}

JSObject* WindowCanvas::GetContext( JS::HandleObject jsSelf, const std::wstring& contextType, JS::HandleValue attributes )
{
    qwr::QwrException::ExpectTrue( contextType == L"2d", L"Unsupported contextType value: {}", contextType );

    // TODO: handle attributes

    JS::RootedValue jsRenderingContextValue(
        pJsCtx_,
        JS::GetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kRenderingContext ) ) );
    JS::RootedObject jsRenderingContext( pJsCtx_ );
    if ( jsRenderingContextValue.isUndefined() )
    {
        jsRenderingContext = JsObjectBase<CanvasRenderingContext2D_Qwr>::CreateJs( pJsCtx_, *pGraphics_ );
        JS_SetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kRenderingContext ), JS::ObjectValue( *jsRenderingContext ) );

        pNativeRenderingContext_ = JsObjectBase<CanvasRenderingContext2D_Qwr>::ExtractNative( pJsCtx_, jsRenderingContext );
    }
    else
    {
        jsRenderingContext.set( &jsRenderingContextValue.toObject() );
    }

    return jsRenderingContext;
}

JSObject* WindowCanvas::GetContextWithOpt( JS::HandleObject jsSelf, size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes )
{
    switch ( optArgCount )
    {
    case 0:
        return GetContext( jsSelf, contextType, attributes );
    case 1:
        return GetContext( jsSelf, contextType );
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
