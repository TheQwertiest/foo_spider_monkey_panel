#include <stdafx.h>

#include "paint_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/window_canvas.h>
#include <js_backend/utils/js_property_helper.h>

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
    JsObjectBase<mozjs::PaintEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PaintEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Canvas, mozjs::PaintEvent::get_Canvas )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "canvas", Get_Canvas, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::PaintEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PaintEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PaintEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PaintEvent>::PrototypeId = JsPrototypeId::New_PaintEvent;

PaintEvent::PaintEvent( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height )
    : JsEvent( cx, "paint", JsEvent::EventProperties{ .cancelable = false } )
    , pJsCtx_( cx )
    , pGraphics_( &graphics )
    , width_( width )
    , height_( height )
{
}

std::unique_ptr<PaintEvent>
PaintEvent::CreateNative( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height )
{
    return std::unique_ptr<PaintEvent>( new mozjs::PaintEvent( cx, graphics, width, height ) );
}

size_t PaintEvent::GetInternalSize()
{
    return 0;
}

void PaintEvent::ResetGraphics()
{
    pGraphics_ = nullptr;
    // TODO: add reset graphics in generated canvas
}

JSObject* PaintEvent::get_Canvas()
{
    if ( !pGraphics_ )
    {
        return nullptr;
    }

    // TODO: maybe save js canvas object to avoid generating it on each access
    return WindowCanvas::CreateJs( pJsCtx_, *pGraphics_, width_, height_ );
}

} // namespace mozjs
