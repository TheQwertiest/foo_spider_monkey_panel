#include <stdafx.h>

#include "canvas_gradient.h"

#include <dom/css_colours.h>
#include <dom/double_helpers.h>
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
    CanvasGradient_Qwr::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "CanvasGradient",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( addColorStop, CanvasGradient_Qwr::AddColorStop );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "addColorStop", addColorStop, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::CanvasGradient_Qwr );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<CanvasGradient_Qwr>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<CanvasGradient_Qwr>::JsFunctions = jsFunctions.data();
const JsPrototypeId JsObjectTraits<CanvasGradient_Qwr>::PrototypeId = JsPrototypeId::New_Canvas;

CanvasGradient_Qwr::CanvasGradient_Qwr( JSContext* cx, double x0, double y0, double x1, double y1 )
    : pJsCtx_( cx )
    , gradientData_{
        .p0 = { static_cast<float>( x0 ), static_cast<float>( y0 ) },
        .p1 = { static_cast<float>( x1 ), static_cast<float>( y1 ) },
        .presetColors = {},
        .blendPositions = {}
    }
{
}

CanvasGradient_Qwr::~CanvasGradient_Qwr()
{
}

std::unique_ptr<CanvasGradient_Qwr>
CanvasGradient_Qwr::CreateNative( JSContext* cx, double x0, double y0, double x1, double y1 )
{
    return std::unique_ptr<CanvasGradient_Qwr>( new CanvasGradient_Qwr( cx, x0, y0, x1, y1 ) );
}

size_t CanvasGradient_Qwr::GetInternalSize() const
{
    return 0;
}

const CanvasGradient_Qwr::GradientData& CanvasGradient_Qwr::GetGradientData() const
{
    return gradientData_;
}

void CanvasGradient_Qwr::AddColorStop( double offset, const qwr::u8string& color )
{
    qwr::QwrException::ExpectTrue( smp::dom::IsValidDouble( offset ), "Offset is not a finite floating-point value" );
    qwr::QwrException::ExpectTrue( offset >= 0 && offset <= 1, "Offset out of 0-1.0 range" );

    const auto gdiColourOpt = smp::dom::FromCssColour( color );
    qwr::QwrException::ExpectTrue( gdiColourOpt.has_value(), "Invalid color" );

    const auto fOffset = static_cast<float>( offset );

    auto& [p0, p1, presetColors, blendPositions] = gradientData_;

    auto it = ranges::find( blendPositions, fOffset );
    if ( it == blendPositions.end() )
    {
        presetColors.emplace_back( *gdiColourOpt );
        blendPositions.emplace_back( fOffset );
    }
    else
    {
        auto pos = std::distance( blendPositions.begin(), it );
        presetColors[pos] = *gdiColourOpt;
        blendPositions[pos] = fOffset;
    }
}

} // namespace mozjs
