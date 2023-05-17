#include <stdafx.h>

#include "canvas_rendering_context_2d.h"

#include <dom/css_colours.h>
#include <dom/double_helpers.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <utils/gdi_error_helpers.h>

#include <cmath>

using namespace smp;

namespace
{

auto ApplyAlpha( uint32_t colour, double alpha )
{
    const auto newAlpha = static_cast<uint8_t>( std::round( ( ( colour & 0xFF000000 ) >> ( 8 * 3 ) ) * alpha ) );
    return ( ( colour & 0xFFFFFF ) | ( newAlpha << ( 8 * 3 ) ) );
}

} // namespace

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
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_GlobalAlpha, mozjs::CanvasRenderingContext2d::get_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_StrokeStyle, mozjs::CanvasRenderingContext2d::get_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LineJoin, mozjs::CanvasRenderingContext2d::get_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LineWidth, mozjs::CanvasRenderingContext2d::get_LineWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_FillStyle, mozjs::CanvasRenderingContext2d::put_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_GlobalAlpha, mozjs::CanvasRenderingContext2d::put_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_StrokeStyle, mozjs::CanvasRenderingContext2d::put_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_LineJoin, mozjs::CanvasRenderingContext2d::put_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_LineWidth, mozjs::CanvasRenderingContext2d::put_LineWidth )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "fillStyle", Get_FillStyle, Put_FillStyle, kDefaultPropsFlags ),
        JS_PSGS( "globalAlpha", Get_GlobalAlpha, Put_GlobalAlpha, kDefaultPropsFlags ),
        JS_PSGS( "lineJoin", Get_LineJoin, Put_LineJoin, kDefaultPropsFlags ),
        JS_PSGS( "lineWidth", Get_LineWidth, Put_LineWidth, kDefaultPropsFlags ),
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
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( width ) || !smp::dom::IsValidDouble( height )
         || !width || !height )
    {
        return;
    }

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
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( width ) || !smp::dom::IsValidDouble( height ) )
    {
        return;
    }

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
    return smp::dom::ToCssColour( originalFillColour_ );
}

double CanvasRenderingContext2d::get_GlobalAlpha() const
{
    return globalAlpha_;
}

qwr::u8string CanvasRenderingContext2d::get_LineJoin() const
{
    switch ( pStrokePen_->GetLineJoin() )
    {
    case Gdiplus::LineJoinMiter:
        return "miter";
    case Gdiplus::LineJoinBevel:
        return "bevel";
    case Gdiplus::LineJoinRound:
        return "round";
    default:
    {
        assert( false );
        return "miter";
    }
    }
}

double CanvasRenderingContext2d::get_LineWidth() const
{
    return pStrokePen_->GetWidth();
}

qwr::u8string CanvasRenderingContext2d::get_StrokeStyle() const
{
    return smp::dom::ToCssColour( originalStrokeColour_ );
}

void CanvasRenderingContext2d::put_FillStyle( const qwr::u8string& color )
{
    const auto gdiColourOpt = smp::dom::FromCssColour( color );
    if ( !gdiColourOpt )
    {
        return;
    }

    originalFillColour_ = gdiColourOpt->GetValue();

    auto gdiRet = pFillBrush_->SetColor( ApplyAlpha( originalFillColour_, globalAlpha_ ) );
    qwr::error::CheckGdi( gdiRet, "SetColor" );
}

void CanvasRenderingContext2d::put_GlobalAlpha( double alpha )
{
    if ( !smp::dom::IsValidDouble( alpha ) || alpha < 0 || alpha > 1 || globalAlpha_ == alpha )
    {
        return;
    }

    globalAlpha_ = alpha;

    auto gdiRet = pFillBrush_->SetColor( ApplyAlpha( originalFillColour_, globalAlpha_ ) );
    qwr::error::CheckGdi( gdiRet, "SetColor" );

    gdiRet = pStrokePen_->SetColor( ApplyAlpha( originalStrokeColour_, globalAlpha_ ) );
    qwr::error::CheckGdi( gdiRet, "SetColor" );
}

void CanvasRenderingContext2d::put_LineJoin( const qwr::u8string& lineJoin )
{
    const auto gdiLineJoinOpt = [&]() -> std::optional<Gdiplus::LineJoin> {
        if ( lineJoin == "bevel" )
        {
            return Gdiplus::LineJoin::LineJoinBevel;
        }
        if ( lineJoin == "round" )
        {
            return Gdiplus::LineJoin::LineJoinRound;
        }
        if ( lineJoin == "miter" )
        {
            return Gdiplus::LineJoin::LineJoinMiter;
        }
        return std::nullopt;
    }();
    if ( !gdiLineJoinOpt )
    {
        return;
    }

    auto gdiRet = pStrokePen_->SetLineJoin( *gdiLineJoinOpt );
    qwr::error::CheckGdi( gdiRet, "SetLineJoin" );
}

void CanvasRenderingContext2d::put_LineWidth( double lineWidth )
{
    if ( !smp::dom::IsValidDouble( lineWidth ) || lineWidth <= 0 )
    {
        return;
    }

    auto gdiRet = pStrokePen_->SetWidth( static_cast<Gdiplus::REAL>( lineWidth ) );
    qwr::error::CheckGdi( gdiRet, "SetWidth" );
}

void CanvasRenderingContext2d::put_StrokeStyle( const qwr::u8string& color )
{
    const auto gdiColourOpt = smp::dom::FromCssColour( color );
    if ( !gdiColourOpt )
    {
        return;
    }

    originalStrokeColour_ = gdiColourOpt->GetValue();

    auto gdiRet = pStrokePen_->SetColor( ApplyAlpha( originalStrokeColour_, globalAlpha_ ) );
    qwr::error::CheckGdi( gdiRet, "SetColor" );
}

} // namespace mozjs
