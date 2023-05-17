#include <stdafx.h>

#include "canvas_rendering_context_2d.h"

#include <dom/css_colours.h>
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

MJS_DEFINE_JS_FN_FROM_NATIVE( FillRect, CanvasRenderingContext2d::FillRect );
MJS_DEFINE_JS_FN_FROM_NATIVE( StrokeRect, CanvasRenderingContext2d::StrokeRect );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "fillRect", FillRect, 4, kDefaultPropsFlags ),
        JS_FN( "strokeRect", StrokeRect, 4, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_FillStyle, mozjs::CanvasRenderingContext2d::get_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_StrokeStyle, mozjs::CanvasRenderingContext2d::get_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_FillStyle, mozjs::CanvasRenderingContext2d::put_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_StrokeStyle, mozjs::CanvasRenderingContext2d::put_StrokeStyle )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "fillStyle", Get_FillStyle, Put_FillStyle, kDefaultPropsFlags ),
        JS_PSGS( "strokeStyle", Get_StrokeStyle, Put_StrokeStyle, kDefaultPropsFlags ),
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
    , pFillBrush_( std::make_unique<Gdiplus::SolidBrush>( Gdiplus ::Color{} ) )
    , pStrokePen_( std::make_unique<Gdiplus::Pen>( Gdiplus ::Color{} ) )
{
    qwr::error::CheckGdiPlusObject( pFillBrush_ );
    qwr::error::CheckGdiPlusObject( pStrokePen_ );
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

void CanvasRenderingContext2d::FillRect( double x, double y, double width, double height )
{
    assert( pGraphics_ );

    if ( width < 0 )
    {
        width *= -1;
        x -= width;
    }
    if ( height < 0 )
    {
        height *= -1;
        y -= height;
    }

    auto gdiRet = pGraphics_->FillRectangle( pFillBrush_.get(), static_cast<int32_t>( x ), static_cast<int32_t>( y ), static_cast<int32_t>( width ), static_cast<int32_t>( height ) );
    qwr::error::CheckGdi( gdiRet, "FillRectangle" );
}

void CanvasRenderingContext2d::StrokeRect( double x, double y, double width, double height )
{
    assert( pGraphics_ );

    if ( width < 0 )
    {
        width *= -1;
        x -= width;
    }
    if ( height < 0 )
    {
        height *= -1;
        y -= height;
    }

    auto gdiRet = pGraphics_->DrawRectangle( pStrokePen_.get(), static_cast<int32_t>( x ), static_cast<int32_t>( y ), static_cast<int32_t>( width ), static_cast<int32_t>( height ) );
    qwr::error::CheckGdi( gdiRet, "DrawRectangle" );
}

qwr::u8string CanvasRenderingContext2d::get_FillStyle() const
{
    Gdiplus::Color color{};
    auto gdiRet = pFillBrush_->GetColor( &color );
    qwr::error::CheckGdi( gdiRet, "GetColor" );

    return smp::dom::ToCssColour( color );
}

qwr::u8string CanvasRenderingContext2d::get_StrokeStyle() const
{
    Gdiplus::Color color{};
    auto gdiRet = pStrokePen_->GetColor( &color );
    qwr::error::CheckGdi( gdiRet, "GetColor" );

    return smp::dom::ToCssColour( color );
}

void CanvasRenderingContext2d::put_FillStyle( const qwr::u8string& color )
{
    const auto gdiColourOpt = smp::dom::FromCssColour( color );
    if ( !gdiColourOpt )
    {
        return;
    }

    auto gdiRet = pFillBrush_->SetColor( *gdiColourOpt );
    qwr::error::CheckGdi( gdiRet, "SetColor" );
}

void CanvasRenderingContext2d::put_StrokeStyle( const qwr::u8string& color )
{
    const auto gdiColourOpt = smp::dom::FromCssColour( color );
    if ( !gdiColourOpt )
    {
        return;
    }

    auto gdiRet = pStrokePen_->SetColor( *gdiColourOpt );
    qwr::error::CheckGdi( gdiRet, "SetColor" );
}

} // namespace mozjs
