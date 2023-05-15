#include <stdafx.h>

#include "canvas_rendering_context_2d.h"

#include <js_backend/engine/js_to_native_invoker.h>
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
    CanvasRenderingContext2d::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "CanvasRenderingContext2D",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::CanvasRenderingContext2d );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<CanvasRenderingContext2d>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<CanvasRenderingContext2d>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<CanvasRenderingContext2d>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<CanvasRenderingContext2d>::PrototypeId = JsPrototypeId::New_CanvasRenderingContext2d;

CanvasRenderingContext2d::CanvasRenderingContext2d( JSContext* cx, Gdiplus::Graphics& graphics )
    : pJsCtx_( cx )
    , pGraphics_( &graphics )
{
}

CanvasRenderingContext2d::~CanvasRenderingContext2d()
{
}

std::unique_ptr<mozjs::CanvasRenderingContext2d>
CanvasRenderingContext2d::CreateNative( JSContext* cx, Gdiplus::Graphics& graphics )
{
    return std::unique_ptr<CanvasRenderingContext2d>( new CanvasRenderingContext2d( cx, graphics ) );
}

size_t CanvasRenderingContext2d::GetInternalSize()
{
    return 0;
}

void CanvasRenderingContext2d::Reinitialize( Gdiplus::Graphics& graphics )
{
    pGraphics_ = &graphics;
}

} // namespace mozjs
