#include <stdafx.h>

#include "canvas_rendering_context_2d.h"

#include <dom/css_colours.h>
#include <dom/css_fonts.h>
#include <dom/double_helpers.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_gradient.h>
#include <js_backend/objects/dom/canvas/utils/gradient_clamp.h>
#include <utils/gdi_error_helpers.h>

#include <qwr/utility.h>

#include <cmath>
#include <numbers>

using namespace smp;

namespace
{

enum class ReservedSlots
{
    kFillGradientSlot = mozjs::kReservedObjectSlot + 1,
    kStrokeGradientSlot
};

}

namespace
{

inline bool IsEpsilonEqual( float a, float b )
{
    return std::abs( a - b ) <= 1e-5;
}

inline bool IsEpsilonLess( float a, float b )
{
    return !IsEpsilonEqual( a, b ) && a < b;
}

inline bool IsEpsilonGreater( float a, float b )
{
    return !IsEpsilonEqual( a, b ) && a > b;
}

auto ApplyAlpha( uint32_t colour, double alpha )
{
    const auto newAlpha = static_cast<uint8_t>( std::round( ( ( colour & 0xFF000000 ) >> ( 8 * 3 ) ) * alpha ) );
    return ( ( colour & 0xFFFFFF ) | ( newAlpha << ( 8 * 3 ) ) );
}

std::vector<Gdiplus::PointF> RectToPoints( const Gdiplus::RectF& rect )
{
    return {
        {
            { rect.X, rect.Y },
            { rect.X + rect.Width, rect.Y },
            { rect.X, rect.Y + rect.Height },
            { rect.X + rect.Width, rect.Y + rect.Height },
        }
    };
}

/// @throw qwr::QwrException
std::unique_ptr<Gdiplus::LinearGradientBrush>
GenerateLinearGradientBrush( const mozjs::CanvasGradient_Qwr::GradientData& gradientData,
                             const std::vector<Gdiplus::PointF>& drawArea,
                             double alpha )
{
    auto [p0, p1, presetColors, blendPositions] = gradientData;
    if ( p0.Equals( p1 ) || blendPositions.empty() )
    {
        return nullptr;
    }

    // Gdiplus repeats gradient pattern, instead of clamping colours,
    // when exceeding brush coordinates, hence some school math is required
    smp::utils::ClampGradient( p0, p1, blendPositions, drawArea );

    auto zippedGradientData = ranges::views::zip( blendPositions, presetColors );
    ranges::actions::sort( zippedGradientData, []( const auto& a, const auto& b ) { return std::get<0>( a ) < std::get<0>( b ); } );

    // SetInterpolationColors requires that 0.0 and 1.0 positions are always defined
    if ( blendPositions.front() != 0.0 )
    {
        blendPositions.insert( blendPositions.begin(), 0.0f );
        presetColors.insert( presetColors.begin(), presetColors.front() );
    }
    if ( blendPositions.back() != 1.0 )
    {
        blendPositions.emplace_back( 1.0f );
        presetColors.emplace_back( presetColors.back() );
    }

    ranges::for_each( presetColors, [&]( auto& elem ) { elem = ApplyAlpha( elem.GetValue(), alpha ); } );

    auto pBrush = std::make_unique<Gdiplus::LinearGradientBrush>( p0, p1, Gdiplus::Color{}, Gdiplus::Color{} );
    qwr::error::CheckGdiPlusObject( pBrush );

    auto gdiRet = pBrush->SetInterpolationColors( presetColors.data(), blendPositions.data(), blendPositions.size() );
    qwr::error::CheckGdi( gdiRet, "SetInterpolationColors" );

    return pBrush;
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
    CanvasRenderingContext2D_Qwr::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "CanvasRenderingContext2D",
    DefaultClassFlags( 2 ),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( BeginPath, CanvasRenderingContext2D_Qwr::BeginPath );
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateLinearGradient, CanvasRenderingContext2D_Qwr::CreateLinearGradient );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Ellipse, CanvasRenderingContext2D_Qwr::Ellipse, CanvasRenderingContext2D_Qwr::EllipseWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( Fill, CanvasRenderingContext2D_Qwr::Fill );
MJS_DEFINE_JS_FN_FROM_NATIVE( FillRect, CanvasRenderingContext2D_Qwr::FillRect );
MJS_DEFINE_JS_FN_FROM_NATIVE( LineTo, CanvasRenderingContext2D_Qwr::LineTo );
MJS_DEFINE_JS_FN_FROM_NATIVE( MoveTo, CanvasRenderingContext2D_Qwr::MoveTo );
MJS_DEFINE_JS_FN_FROM_NATIVE( Stroke, CanvasRenderingContext2D_Qwr::Stroke );
MJS_DEFINE_JS_FN_FROM_NATIVE( StrokeRect, CanvasRenderingContext2D_Qwr::StrokeRect );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "beginPath", BeginPath, 0, kDefaultPropsFlags ),
        JS_FN( "createLinearGradient", CreateLinearGradient, 4, kDefaultPropsFlags ),
        JS_FN( "ellipse", Ellipse, 7, kDefaultPropsFlags ),
        JS_FN( "fill", Fill, 0, kDefaultPropsFlags ),
        JS_FN( "fillRect", FillRect, 4, kDefaultPropsFlags ),
        JS_FN( "lineTo", LineTo, 2, kDefaultPropsFlags ),
        JS_FN( "moveTo", MoveTo, 2, kDefaultPropsFlags ),
        JS_FN( "stroke", Stroke, 0, kDefaultPropsFlags ),
        JS_FN( "strokeRect", StrokeRect, 4, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Get_FillStyle, mozjs::CanvasRenderingContext2D_Qwr::get_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Font, mozjs::CanvasRenderingContext2D_Qwr::get_Font )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_GlobalAlpha, mozjs::CanvasRenderingContext2D_Qwr::get_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_GlobalCompositeOperation, mozjs::CanvasRenderingContext2D_Qwr::get_GlobalCompositeOperation )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Get_StrokeStyle, mozjs::CanvasRenderingContext2D_Qwr::get_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LineJoin, mozjs::CanvasRenderingContext2D_Qwr::get_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LineWidth, mozjs::CanvasRenderingContext2D_Qwr::get_LineWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Put_FillStyle, mozjs::CanvasRenderingContext2D_Qwr::put_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_Font, mozjs::CanvasRenderingContext2D_Qwr::put_Font )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_GlobalAlpha, mozjs::CanvasRenderingContext2D_Qwr::put_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_GlobalCompositeOperation, mozjs::CanvasRenderingContext2D_Qwr::put_GlobalCompositeOperation )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Put_StrokeStyle, mozjs::CanvasRenderingContext2D_Qwr::put_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_LineJoin, mozjs::CanvasRenderingContext2D_Qwr::put_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_LineWidth, mozjs::CanvasRenderingContext2D_Qwr::put_LineWidth )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "fillStyle", Get_FillStyle, Put_FillStyle, kDefaultPropsFlags ),
        JS_PSGS( "font", Get_Font, Put_Font, kDefaultPropsFlags ),
        JS_PSGS( "globalAlpha", Get_GlobalAlpha, Put_GlobalAlpha, kDefaultPropsFlags ),
        JS_PSGS( "globalCompositeOperation", Get_GlobalCompositeOperation, Put_GlobalCompositeOperation, kDefaultPropsFlags ),
        JS_PSGS( "lineJoin", Get_LineJoin, Put_LineJoin, kDefaultPropsFlags ),
        JS_PSGS( "lineWidth", Get_LineWidth, Put_LineWidth, kDefaultPropsFlags ),
        JS_PSGS( "strokeStyle", Get_StrokeStyle, Put_StrokeStyle, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::CanvasRenderingContext2D_Qwr );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<CanvasRenderingContext2D_Qwr>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<CanvasRenderingContext2D_Qwr>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<CanvasRenderingContext2D_Qwr>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<CanvasRenderingContext2D_Qwr>::PrototypeId = JsPrototypeId::New_CanvasRenderingContext2d;

CanvasRenderingContext2D_Qwr::CanvasRenderingContext2D_Qwr( JSContext* cx, Gdiplus::Graphics& graphics )
    : pJsCtx_( cx )
    , pGraphics_( &graphics )
    , pFillBrush_( std::make_unique<Gdiplus::SolidBrush>( Gdiplus ::Color{} ) )
    , pStrokePen_( std::make_unique<Gdiplus::Pen>( Gdiplus ::Color{} ) )
    , pGraphicsPath_( std::make_unique<Gdiplus::GraphicsPath>() )
{
    qwr::error::CheckGdiPlusObject( pFillBrush_ );
    qwr::error::CheckGdiPlusObject( pStrokePen_ );
    qwr::error::CheckGdiPlusObject( pGraphicsPath_ );
}

CanvasRenderingContext2D_Qwr::~CanvasRenderingContext2D_Qwr()
{
}

std::unique_ptr<mozjs::CanvasRenderingContext2D_Qwr>
CanvasRenderingContext2D_Qwr::CreateNative( JSContext* cx, Gdiplus::Graphics& graphics )
{
    return std::unique_ptr<CanvasRenderingContext2D_Qwr>( new CanvasRenderingContext2D_Qwr( cx, graphics ) );
}

size_t CanvasRenderingContext2D_Qwr::GetInternalSize() const
{
    return 0;
}

void CanvasRenderingContext2D_Qwr::Reinitialize( Gdiplus::Graphics& graphics )
{
    pGraphics_ = &graphics;
}

void CanvasRenderingContext2D_Qwr::BeginPath()
{
    auto gdiRet = pGraphicsPath_->Reset();
    qwr::error::CheckGdi( gdiRet, "Reset" );

    lastPathPosOpt_.reset();
}

JSObject* CanvasRenderingContext2D_Qwr::CreateLinearGradient( double x0, double y0, double x1, double y1 )
{
    qwr::QwrException::ExpectTrue( smp::dom::IsValidDouble( x0 ) && smp::dom::IsValidDouble( y0 )
                                       && smp::dom::IsValidDouble( x1 ) && smp::dom::IsValidDouble( y1 ),
                                   "Coordinate is not a finite floating-point value" );

    return JsObjectBase<CanvasGradient_Qwr>::CreateJs( pJsCtx_, x0, y0, x1, y1 );
}

void CanvasRenderingContext2D_Qwr::Ellipse( double x, double y, double radiusX, double radiusY,
                                            double rotation, double startAngle, double endAngle, bool counterclockwise )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( radiusX ) || !smp::dom::IsValidDouble( radiusY )
         || !smp::dom::IsValidDouble( rotation )
         || !smp::dom::IsValidDouble( startAngle ) || !smp::dom::IsValidDouble( endAngle ) )
    {
        return;
    }

    qwr::QwrException::ExpectTrue( radiusX > 0 && radiusY > 0, "Negative radius" );

    const auto convertRadiansToDegrees = []( double radians ) {
        auto degrees = ( radians * 180 / std::numbers::pi );
        return static_cast<float>( degrees );
    };

    Gdiplus::PointF centerPoint{ static_cast<float>( x ), static_cast<float>( y ) };
    const Gdiplus::RectF rect{
        static_cast<float>( x - radiusX ),
        static_cast<float>( y - radiusY ),
        static_cast<float>( 2 * radiusX ),
        static_cast<float>( 2 * radiusY )
    };

    const auto startAngleInDegrees = convertRadiansToDegrees( startAngle );
    const auto endAngleInDegrees = convertRadiansToDegrees( endAngle );
    const auto sweepAngleInDegrees = [&] {
        // TODO: simplify
        if ( !counterclockwise )
        {
            auto sweepAngleRaw = endAngleInDegrees - startAngleInDegrees;
            if ( sweepAngleRaw > 360 )
            { // yup, according to the spec, going backwards and exceeding 360 degrees is different
                // from doing so when going backwards...
                // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-ellipse
                sweepAngleRaw = 360;
            }
            else if ( IsEpsilonLess( sweepAngleRaw, 0 ) )
            {
                sweepAngleRaw = std::fmodf( sweepAngleRaw, 360 );
                sweepAngleRaw += 360;
            }
            return sweepAngleRaw;
        }
        else
        {
            auto sweepAngleRaw = startAngleInDegrees - endAngleInDegrees;
            if ( sweepAngleRaw > 360 )
            {
                sweepAngleRaw = 360;
            }
            else if ( IsEpsilonLess( sweepAngleRaw, 0 ) )
            {
                sweepAngleRaw = std::fmodf( sweepAngleRaw, 360 );
                sweepAngleRaw += 360;
            }
            return -sweepAngleRaw;
        }
    }();

    auto pTmpGraphicsPath = std::make_unique<Gdiplus::GraphicsPath>();
    qwr::error::CheckGdiPlusObject( pTmpGraphicsPath );

    // TODO: AddArc fails when startAngleInDegrees == sweepAngleInDegrees,
    // canvas API does not (and it sets last point at the corresponding angle)
    auto gdiRet = pTmpGraphicsPath->AddArc( rect, startAngleInDegrees, sweepAngleInDegrees );
    qwr::error::CheckGdi( gdiRet, "AddArc" );

    if ( rotation )
    {
        Gdiplus::Matrix matrix;
        gdiRet = matrix.RotateAt( convertRadiansToDegrees( rotation ), centerPoint );
        qwr::error::CheckGdi( gdiRet, "RotateAt" );

        gdiRet = pTmpGraphicsPath->Transform( &matrix );
        qwr::error::CheckGdi( gdiRet, "Transform" );
    }

    gdiRet = pGraphicsPath_->AddPath( pTmpGraphicsPath.get(), true );
    qwr::error::CheckGdi( gdiRet, "AddPath" );

    Gdiplus::PointF lastPoint;
    gdiRet = pGraphicsPath_->GetLastPoint( &lastPoint );
    qwr::error::CheckGdi( gdiRet, "GetLastPoint" );

    lastPathPosOpt_ = lastPoint;
}

void CanvasRenderingContext2D_Qwr::EllipseWithOpt( size_t optArgCount, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise )
{
    switch ( optArgCount )
    {
    case 0:
        return Ellipse( x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise );
    case 1:
        return Ellipse( x, y, radiusX, radiusY, rotation, startAngle, endAngle );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void CanvasRenderingContext2D_Qwr::Fill()
{
    if ( !lastPathPosOpt_ )
    {
        return;
    }

    const auto pGradientBrush = [&]() -> std::unique_ptr<Gdiplus::Brush> {
        if ( !pFillGradient_ )
        {
            return nullptr;
        }

        Gdiplus::RectF bounds;
        auto gdiRet = pGraphicsPath_->GetBounds( &bounds, nullptr, nullptr );
        qwr::error::CheckGdi( gdiRet, "GetBounds" );

        return GenerateLinearGradientBrush( pFillGradient_->GetGradientData(), RectToPoints( bounds ), globalAlpha_ );
    }();
    if ( pFillGradient_ && !pGradientBrush )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphics_->FillPath( ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ), pGraphicsPath_.get() );
    qwr::error::CheckGdi( gdiRet, "FillPath" );
}

void CanvasRenderingContext2D_Qwr::FillRect( double x, double y, double w, double h )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( w ) || !smp::dom::IsValidDouble( h )
         || !w || !h )
    {
        return;
    }

    if ( w < 0 )
    {
        w *= -1;
        x -= w;
    }
    if ( h < 0 )
    {
        h *= -1;
        y -= h;
    }

    const Gdiplus::RectF rect{
        static_cast<float>( x ),
        static_cast<float>( y ),
        static_cast<float>( w ),
        static_cast<float>( h )
    };

    const auto pGradientBrush = [&]() -> std::unique_ptr<Gdiplus::Brush> {
        if ( !pFillGradient_ )
        {
            return nullptr;
        }
        return GenerateLinearGradientBrush( pFillGradient_->GetGradientData(), RectToPoints( rect ), globalAlpha_ );
    }();
    if ( pFillGradient_ && !pGradientBrush )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphics_->FillRectangle( ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ), rect );
    qwr::error::CheckGdi( gdiRet, "FillRectangle" );
}

void CanvasRenderingContext2D_Qwr::LineTo( double x, double y )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y ) )
    {
        return;
    }

    Gdiplus::PointF pointTo{ static_cast<float>( x ), static_cast<float>( y ) };
    if ( !lastPathPosOpt_ )
    {
        lastPathPosOpt_.emplace( pointTo );
    }

    auto gdiRet = pGraphicsPath_->AddLine( *lastPathPosOpt_, pointTo );
    qwr::error::CheckGdi( gdiRet, "AddLine" );

    Gdiplus::PointF lastPoint;
    gdiRet = pGraphicsPath_->GetLastPoint( &lastPoint );
    qwr::error::CheckGdi( gdiRet, "GetLastPoint" );

    lastPathPosOpt_ = pointTo;
}

void CanvasRenderingContext2D_Qwr::MoveTo( double x, double y )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y ) )
    {
        return;
    }

    BeginPath();
    lastPathPosOpt_.emplace( static_cast<float>( x ), static_cast<float>( y ) );
}

void CanvasRenderingContext2D_Qwr::Stroke()
{
    if ( !lastPathPosOpt_ )
    {
        return;
    }

    const auto pGradientPen = [&]() -> std::unique_ptr<Gdiplus::Pen> {
        if ( !pStrokeGradient_ )
        {
            return nullptr;
        }

        Gdiplus::RectF bounds;
        auto gdiRet = pGraphicsPath_->GetBounds( &bounds, nullptr, nullptr );
        qwr::error::CheckGdi( gdiRet, "GetBounds" );

        return GenerateGradientStrokePen( RectToPoints( bounds ) );
    }();
    if ( pFillGradient_ && !pGradientPen )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphics_->DrawPath( ( pGradientPen ? pGradientPen.get() : pStrokePen_.get() ), pGraphicsPath_.get() );
    qwr::error::CheckGdi( gdiRet, "DrawPath" );
}

void CanvasRenderingContext2D_Qwr::StrokeRect( double x, double y, double w, double h )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( w ) || !smp::dom::IsValidDouble( h ) )
    {
        return;
    }

    if ( w < 0 )
    {
        w *= -1;
        x -= w;
    }
    if ( h < 0 )
    {
        h *= -1;
        y -= h;
    }

    const Gdiplus::RectF rect{
        static_cast<float>( x ),
        static_cast<float>( y ),
        static_cast<float>( w ),
        static_cast<float>( h )
    };

    const auto pGradientPen = [&]() -> std::unique_ptr<Gdiplus::Pen> {
        if ( !pStrokeGradient_ )
        {
            return nullptr;
        }

        return GenerateGradientStrokePen( RectToPoints( rect ) );
    }();
    if ( pStrokeGradient_ && !pGradientPen )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphics_->DrawRectangle( ( pGradientPen ? pGradientPen.get() : pStrokePen_.get() ), rect );
    qwr::error::CheckGdi( gdiRet, "DrawRectangle" );
}

qwr::u8string CanvasRenderingContext2D_Qwr::get_GlobalCompositeOperation() const
{
    switch ( pGraphics_->GetCompositingMode() )
    {
    case Gdiplus::CompositingModeSourceOver:
        return "source-over";
    default:
    {
        assert( false );
        return "source-over";
    }
    }
}

JS::Value CanvasRenderingContext2D_Qwr::get_FillStyle( JS::HandleObject jsSelf ) const
{
    if ( pFillGradient_ )
    {
        // assumes that state is valid and corresponding js object is saved
        JS::RootedValue jsValue(
            pJsCtx_,
            JS::GetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kFillGradientSlot ) ) );
        assert( jsValue.isObject() );

        return jsValue;
    }
    else
    {
        JS::RootedValue jsValue( pJsCtx_ );
        mozjs::convert::to_js::ToValue( pJsCtx_, smp::dom::ToCssColour( originalFillColour_ ), &jsValue );
        return jsValue;
    }
}

qwr::u8string CanvasRenderingContext2D_Qwr::get_Font()
{
    return fontDescription_.cssFont;
}

double CanvasRenderingContext2D_Qwr::get_GlobalAlpha() const
{
    return globalAlpha_;
}

qwr::u8string CanvasRenderingContext2D_Qwr::get_LineJoin() const
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

double CanvasRenderingContext2D_Qwr::get_LineWidth() const
{
    return pStrokePen_->GetWidth();
}

JS::Value CanvasRenderingContext2D_Qwr::get_StrokeStyle( JS::HandleObject jsSelf ) const
{
    if ( pStrokeGradient_ )
    {
        // assumes that state is valid and corresponding js object is saved
        JS::RootedValue jsValue(
            pJsCtx_,
            JS::GetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kStrokeGradientSlot ) ) );
        assert( jsValue.isObject() );

        return jsValue;
    }
    else
    {
        JS::RootedValue jsValue( pJsCtx_ );
        mozjs::convert::to_js::ToValue( pJsCtx_, smp::dom::ToCssColour( originalStrokeColour_ ), &jsValue );
        return jsValue;
    }
}

void CanvasRenderingContext2D_Qwr::put_GlobalCompositeOperation( const qwr::u8string& value )
{
    const auto gdiCompositingModeOpt = [&]() -> std::optional<Gdiplus::CompositingMode> {
        if ( value == "source-over" )
        {
            return Gdiplus::CompositingMode::CompositingModeSourceOver;
        }
        return std::nullopt;
    }();
    if ( !gdiCompositingModeOpt )
    {
        return;
    }

    auto gdiRet = pGraphics_->SetCompositingMode( *gdiCompositingModeOpt );
    qwr::error::CheckGdi( gdiRet, "SetCompositingMode" );
}

void CanvasRenderingContext2D_Qwr::put_FillStyle( JS::HandleObject jsSelf, JS::HandleValue jsValue )
{
    // TODO: deduplicate code with stroke style

    if ( jsValue.isString() )
    {
        const auto color = mozjs::convert::to_native::ToValue<qwr::u8string>( pJsCtx_, jsValue );
        const auto gdiColourOpt = smp::dom::FromCssColour( color );
        if ( !gdiColourOpt )
        {
            return;
        }

        const auto newColour = gdiColourOpt->GetValue();

        auto gdiRet = pFillBrush_->SetColor( ApplyAlpha( newColour, globalAlpha_ ) );
        qwr::error::CheckGdi( gdiRet, "SetColor" );

        originalFillColour_ = newColour;
        pFillGradient_ = nullptr;
    }
    else
    {
        try
        {
            // first, verify with ExtractNative
            pFillGradient_ = JsObjectBase<CanvasGradient_Qwr>::ExtractNative( pJsCtx_, jsValue );
            // and only then save
            JS_SetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kFillGradientSlot ), jsValue );
        }
        catch ( const qwr::QwrException& /**/ )
        {
        }
        catch ( const smp::JsException& /**/ )
        {
        }
    }
}

void CanvasRenderingContext2D_Qwr::put_Font( const qwr::u8string& value )
{
    auto fontDescriptionOpt = smp::dom::FromCssFont( value );
    if ( !fontDescriptionOpt )
    {
        return;
    }

    fontDescription_ = *fontDescriptionOpt;
}

void CanvasRenderingContext2D_Qwr::put_GlobalAlpha( double value )
{
    if ( !smp::dom::IsValidDouble( value ) || value < 0 || value > 1 || globalAlpha_ == value )
    {
        return;
    }

    globalAlpha_ = value;

    auto gdiRet = pFillBrush_->SetColor( ApplyAlpha( originalFillColour_, globalAlpha_ ) );
    qwr::error::CheckGdi( gdiRet, "SetColor" );

    gdiRet = pStrokePen_->SetColor( ApplyAlpha( originalStrokeColour_, globalAlpha_ ) );
    qwr::error::CheckGdi( gdiRet, "SetColor" );
}

void CanvasRenderingContext2D_Qwr::put_LineJoin( const qwr::u8string& value )
{
    const auto gdiLineJoinOpt = [&]() -> std::optional<Gdiplus::LineJoin> {
        if ( value == "bevel" )
        {
            return Gdiplus::LineJoin::LineJoinBevel;
        }
        if ( value == "round" )
        {
            return Gdiplus::LineJoin::LineJoinRound;
        }
        if ( value == "miter" )
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

void CanvasRenderingContext2D_Qwr::put_LineWidth( double value )
{
    if ( !smp::dom::IsValidDouble( value ) || value <= 0 )
    {
        return;
    }

    auto gdiRet = pStrokePen_->SetWidth( static_cast<Gdiplus::REAL>( value ) );
    qwr::error::CheckGdi( gdiRet, "SetWidth" );
}

void CanvasRenderingContext2D_Qwr::put_StrokeStyle( JS::HandleObject jsSelf, JS::HandleValue jsValue )
{
    if ( jsValue.isString() )
    {
        const auto color = mozjs::convert::to_native::ToValue<qwr::u8string>( pJsCtx_, jsValue );
        const auto gdiColourOpt = smp::dom::FromCssColour( color );
        if ( !gdiColourOpt )
        {
            return;
        }

        const auto newColour = gdiColourOpt->GetValue();

        auto gdiRet = pStrokePen_->SetColor( ApplyAlpha( newColour, globalAlpha_ ) );
        qwr::error::CheckGdi( gdiRet, "SetColor" );

        originalStrokeColour_ = newColour;
        pStrokeGradient_ = nullptr;
    }
    else
    {
        try
        {
            // first, verify with ExtractNative
            pStrokeGradient_ = JsObjectBase<CanvasGradient_Qwr>::ExtractNative( pJsCtx_, jsValue );
            // and only then save
            JS_SetReservedSlot( jsSelf, qwr::to_underlying( ReservedSlots::kStrokeGradientSlot ), jsValue );
        }
        catch ( const qwr::QwrException& /**/ )
        {
        }
        catch ( const smp::JsException& /**/ )
        {
        }
    }
}

std::unique_ptr<Gdiplus::Pen>
CanvasRenderingContext2D_Qwr::GenerateGradientStrokePen( const std::vector<Gdiplus::PointF>& drawArea )
{
    if ( !pStrokeGradient_ )
    {
        return nullptr;
    }

    const auto pBrush = GenerateLinearGradientBrush( pStrokeGradient_->GetGradientData(), drawArea, globalAlpha_ );
    if ( !pBrush )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Pen> pPen( pStrokePen_->Clone() );
    qwr::error::CheckGdiPlusObject( pPen );

    auto gdiRet = pPen->SetBrush( pBrush.get() );
    qwr::error::CheckGdi( gdiRet, "SetLineJoin" );

    return pPen;
}

} // namespace mozjs
