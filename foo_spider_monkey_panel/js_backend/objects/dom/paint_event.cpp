#include <stdafx.h>

#include "paint_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/gdi/gdi_graphics.h>
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

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Graphics, mozjs::PaintEvent::get_Graphics )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "graphics", Get_Graphics, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::PaintEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PaintEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PaintEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PaintEvent>::PrototypeId = JsPrototypeId::New_PaintEvent;

PaintEvent::PaintEvent( JSContext* cx, Gdiplus::Graphics& graphics )
    : JsEvent( cx, "paint", JsEvent::EventProperties{ .cancelable = false } )
    , pJsCtx_( cx )
    , pGraphics_( &graphics )
{
}

std::unique_ptr<PaintEvent>
PaintEvent::CreateNative( JSContext* cx, Gdiplus::Graphics& graphics )
{
    return std::unique_ptr<PaintEvent>( new mozjs::PaintEvent( cx, graphics ) );
}

size_t PaintEvent::GetInternalSize()
{
    return 0;
}

void PaintEvent::ResetGraphics()
{
    pGraphics_ = nullptr;
}

JSObject* PaintEvent::get_Graphics()
{
    if ( !pGraphics_ )
    {
        return nullptr;
    }

    JS::RootedObject jsGraphics( pJsCtx_, JsGdiGraphics::CreateJs( pJsCtx_ ) );

    auto pNativeGraphics = JsGdiGraphics::ExtractNativeUnchecked( jsGraphics );
    assert( pNativeGraphics );

    pNativeGraphics->SetGraphicsObject( pGraphics_ );

    return jsGraphics;
}

} // namespace mozjs
