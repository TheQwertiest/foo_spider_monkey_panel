#include <stdafx.h>

#include "canvas_rendering_context_2d.h"

#include <dom/css_colours.h>
#include <dom/css_fonts.h>
#include <dom/double_helpers.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_gradient.h>
#include <js_backend/objects/dom/canvas/utils/gradient_clamp.h>
#include <js_backend/utils/js_property_helper.h>
#include <utils/colour_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/string_utils.h>

#include <qwr/final_action.h>
#include <qwr/string_helpers.h>
#include <qwr/utility.h>
#include <qwr/winapi_error_helpers.h>

#include <cmath>
#include <numbers>
#include <string_view>

namespace str_utils = smp::utils::string;
using namespace std::literals;
using namespace smp;

namespace
{

enum class ReservedSlots
{
    kFillGradientSlot = mozjs::kReservedObjectSlot + 1,
    kStrokeGradientSlot
};

struct GdiPlusFontData
{
    std::unique_ptr<const Gdiplus::Font> pFont;
    std::unique_ptr<const Gdiplus::FontFamily> pFontFamily;
    float ascentHeight;
    float lineHeight;
};

struct GdiFontData
{
    std::unique_ptr<CFont> pFont;
    int32_t ascentHeight;
    int32_t lineHeight;
};

} // namespace

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

/// @brief Replaces all new lines and tabs with spaces
auto PrepareText( const std::wstring& text )
{
    auto cleanText = text;
    for ( auto& ch: cleanText )
    {
        if ( ch == L'\t' || ch == L'\r' || ch == L'\n' )
        {
            ch = L' ';
        }
    }
    return cleanText;
}

/// @throw qwr::QwrException
const GdiPlusFontData& FetchGdiPlusFont( const smp::dom::FontDescription& fontDescription, bool isUnderlined = false, bool isStrikeout = false )
{
    // TODO: make cache size configurable?
    // TODO: move to a separate file?
    auto cssFont = fontDescription.cssFont;
    if ( isUnderlined )
    {
        cssFont += L" underline";
    }
    if ( isStrikeout )
    {
        cssFont += L" line-through";
    }

    static std::unordered_map<std::wstring, GdiPlusFontData> cssStrToFontData;
    if ( cssStrToFontData.contains( cssFont ) )
    {
        return cssStrToFontData.at( cssFont );
    }

    auto pFamily = std::make_unique<Gdiplus::FontFamily>( fontDescription.family.c_str() );
    qwr::error::CheckGdiPlusObject( pFamily );

    auto pFont = std::make_unique<Gdiplus::Font>(
        pFamily.get(),
        static_cast<float>( fontDescription.size ),
        [&] {
            const auto isBold = ( fontDescription.weight == qwr::to_underlying( smp::dom::FontWeight::bold ) );
            int32_t style = 0;
            if ( isUnderlined )
            {
                style |= Gdiplus::FontStyleUnderline;
            }
            if ( isStrikeout )
            {
                style |= Gdiplus::FontStyleStrikeout;
            }
            switch ( fontDescription.style )
            {
            case smp::dom::FontStyle::regular:
                return style | ( isBold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular );
            case smp::dom::FontStyle::italic:
                return style | ( isBold ? Gdiplus::FontStyleBoldItalic : Gdiplus::FontStyleItalic );
            default:
            {
                assert( false );
                return qwr::to_underlying( Gdiplus::FontStyleRegular );
            }
            }
        }(),
        [&] {
            switch ( fontDescription.sizeUnit )
            {
            case smp::dom::FontSizeUnit::px:
                return Gdiplus::UnitPixel;
            default:
            {
                assert( false );
                return Gdiplus::UnitPixel;
            }
            }
        }() );
    qwr::error::CheckGdiPlusObject( pFont );

    // TODO: handle dpi and units here
    const auto fontStyle = pFont->GetStyle();
    const auto lineHeight = fontDescription.size * pFamily->GetLineSpacing( fontStyle ) / pFamily->GetEmHeight( fontStyle );
    const auto ascentHeight = fontDescription.size * pFamily->GetCellAscent( fontStyle ) / pFamily->GetEmHeight( fontStyle );

    const auto [it, isEmplaced] = cssStrToFontData.try_emplace(
        cssFont,
        GdiPlusFontData{
            std::move( pFont ),
            std::move( pFamily ),
            static_cast<float>( static_cast<int>( ascentHeight + 0.5 ) ),
            static_cast<float>( lineHeight ) } );
    return it->second;
}

/// @throw qwr::QwrException
const GdiFontData& FetchGdiFont( Gdiplus::Graphics& graphics, const smp::dom::FontDescription& fontDescription, bool isUnderlined = false, bool isStrikeout = false )
{
    auto cssFont = fontDescription.cssFont;
    if ( isUnderlined )
    {
        cssFont += L" underline";
    }
    if ( isStrikeout )
    {
        cssFont += L" line-through";
    }

    // TODO: replace with lru cache and make size configurable
    static std::unordered_map<std::wstring, GdiFontData> cssStrToFontData;
    if ( cssStrToFontData.contains( cssFont ) )
    {
        return cssStrToFontData.at( cssFont );
    }

    auto pFont = std::make_unique<CFont>();
    auto hFont = pFont->CreateFontW( -static_cast<int>( fontDescription.size ),
                                     0,
                                     0,
                                     0,
                                     fontDescription.weight,
                                     fontDescription.style == smp::dom::FontStyle::italic,
                                     isUnderlined,
                                     isStrikeout,
                                     DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE,
                                     fontDescription.family.c_str() );
    qwr::error::CheckWinApi( !!hFont, "CreateFontW" );

    const auto hDc = graphics.GetHDC();
    qwr::final_action autoHdcReleaser( [&] { graphics.ReleaseHDC( hDc ); } );
    smp::gdi::ObjectSelector autoFont( hDc, hFont );

    TEXTMETRICW metrics{};
    auto iRet = ::GetTextMetricsW( hDc, &metrics );
    qwr::error::CheckWinApi( iRet, "GetTextMetricsW" );

    const auto lineHeight = metrics.tmAscent + metrics.tmInternalLeading + metrics.tmExternalLeading + metrics.tmDescent;
    const auto ascentHeight = metrics.tmAscent;

    const auto [it, isEmplaced] = cssStrToFontData.try_emplace(
        cssFont,
        GdiFontData{
            std::move( pFont ),
            static_cast<int32_t>( ascentHeight ),
            static_cast<int32_t>( lineHeight ) } );
    return it->second;
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
MJS_DEFINE_JS_FN_FROM_NATIVE( FillText, CanvasRenderingContext2D_Qwr::FillText );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( FillTextEx, CanvasRenderingContext2D_Qwr::FillTextEx, CanvasRenderingContext2D_Qwr::FillTextExWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( LineTo, CanvasRenderingContext2D_Qwr::LineTo );
MJS_DEFINE_JS_FN_FROM_NATIVE( MoveTo, CanvasRenderingContext2D_Qwr::MoveTo );
MJS_DEFINE_JS_FN_FROM_NATIVE( Stroke, CanvasRenderingContext2D_Qwr::Stroke );
MJS_DEFINE_JS_FN_FROM_NATIVE( StrokeRect, CanvasRenderingContext2D_Qwr::StrokeRect );
MJS_DEFINE_JS_FN_FROM_NATIVE( StrokeText, CanvasRenderingContext2D_Qwr::StrokeText );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "beginPath", BeginPath, 0, kDefaultPropsFlags ),
        JS_FN( "createLinearGradient", CreateLinearGradient, 4, kDefaultPropsFlags ),
        JS_FN( "ellipse", Ellipse, 7, kDefaultPropsFlags ),
        JS_FN( "fill", Fill, 0, kDefaultPropsFlags ),
        JS_FN( "fillRect", FillRect, 4, kDefaultPropsFlags ),
        JS_FN( "fillText", FillText, 3, kDefaultPropsFlags ),
        JS_FN( "fillTextEx", FillTextEx, 3, kDefaultPropsFlags ),
        JS_FN( "lineTo", LineTo, 2, kDefaultPropsFlags ),
        JS_FN( "moveTo", MoveTo, 2, kDefaultPropsFlags ),
        JS_FN( "stroke", Stroke, 0, kDefaultPropsFlags ),
        JS_FN( "strokeRect", StrokeRect, 4, kDefaultPropsFlags ),
        JS_FN( "strokeText", StrokeText, 3, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Get_FillStyle, mozjs::CanvasRenderingContext2D_Qwr::get_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Font, mozjs::CanvasRenderingContext2D_Qwr::get_Font )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_GlobalAlpha, mozjs::CanvasRenderingContext2D_Qwr::get_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_GlobalCompositeOperation, mozjs::CanvasRenderingContext2D_Qwr::get_GlobalCompositeOperation )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Get_StrokeStyle, mozjs::CanvasRenderingContext2D_Qwr::get_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LineJoin, mozjs::CanvasRenderingContext2D_Qwr::get_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LineWidth, mozjs::CanvasRenderingContext2D_Qwr::get_LineWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_TextAlign, mozjs::CanvasRenderingContext2D_Qwr::get_TextAlign )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_TextBaseline, mozjs::CanvasRenderingContext2D_Qwr::get_TextBaseline )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Put_FillStyle, mozjs::CanvasRenderingContext2D_Qwr::put_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_Font, mozjs::CanvasRenderingContext2D_Qwr::put_Font )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_GlobalAlpha, mozjs::CanvasRenderingContext2D_Qwr::put_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_GlobalCompositeOperation, mozjs::CanvasRenderingContext2D_Qwr::put_GlobalCompositeOperation )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( Put_StrokeStyle, mozjs::CanvasRenderingContext2D_Qwr::put_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_LineJoin, mozjs::CanvasRenderingContext2D_Qwr::put_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_LineWidth, mozjs::CanvasRenderingContext2D_Qwr::put_LineWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_TextAlign, mozjs::CanvasRenderingContext2D_Qwr::put_TextAlign )
MJS_DEFINE_JS_FN_FROM_NATIVE( Put_TextBaseline, mozjs::CanvasRenderingContext2D_Qwr::put_TextBaseline )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "fillStyle", Get_FillStyle, Put_FillStyle, kDefaultPropsFlags ),
        JS_PSGS( "font", Get_Font, Put_Font, kDefaultPropsFlags ),
        JS_PSGS( "globalAlpha", Get_GlobalAlpha, Put_GlobalAlpha, kDefaultPropsFlags ),
        JS_PSGS( "globalCompositeOperation", Get_GlobalCompositeOperation, Put_GlobalCompositeOperation, kDefaultPropsFlags ),
        JS_PSGS( "lineJoin", Get_LineJoin, Put_LineJoin, kDefaultPropsFlags ),
        JS_PSGS( "lineWidth", Get_LineWidth, Put_LineWidth, kDefaultPropsFlags ),
        JS_PSGS( "strokeStyle", Get_StrokeStyle, Put_StrokeStyle, kDefaultPropsFlags ),
        JS_PSGS( "textAlign", Get_TextAlign, Put_TextAlign, kDefaultPropsFlags ),
        JS_PSGS( "textBaseline", Get_TextBaseline, Put_TextBaseline, kDefaultPropsFlags ),
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
    , stringFormat_( Gdiplus::StringFormat::GenericTypographic() )
{
    qwr::error::CheckGdiPlusObject( pFillBrush_ );
    qwr::error::CheckGdiPlusObject( pStrokePen_ );
    qwr::error::CheckGdiPlusObject( pGraphicsPath_ );
    qwr::error::CheckGdi( stringFormat_.GetLastStatus(), "GenericTypographic" );
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

void CanvasRenderingContext2D_Qwr::FillText( const std::wstring& text, double x, double y )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y ) )
    {
        return;
    }

    FillTextExOptions options;
    options.shouldCollapseNewLines = true;
    options.shouldCollapseSpaces = true;
    options.shouldUseCanvasCollapseRules = true;

    DrawString_FillTextEx( text, x, y, options );
}

void CanvasRenderingContext2D_Qwr::FillTextEx( const std::wstring& text, double x, double y, JS::HandleValue options )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y ) )
    {
        return;
    }

    const auto parsedOptions = ParseOptions_FillTextEx( options );
    if ( !smp::dom::IsValidDouble( parsedOptions.width ) || !smp::dom::IsValidDouble( parsedOptions.height ) )
    {
        return;
    }

    if ( parsedOptions.renderMode == "clarity" )
    {
        DrawGdiString_FillTextEx( text, x, y, parsedOptions );
    }
    else if ( parsedOptions.renderMode == "stroke-compat" )
    {
        DrawPath_FillTextEx( text, x, y, parsedOptions );
    }
    else
    {
        DrawString_FillTextEx( text, x, y, parsedOptions );
    }
}

void CanvasRenderingContext2D_Qwr::FillTextExWithOpt( size_t optArgCount, const std::wstring& text, double x, double y, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return FillTextEx( text, x, y, options );
    case 1:
        return FillTextEx( text, x, y );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
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
    if ( pStrokeGradient_ && !pGradientPen )
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

void CanvasRenderingContext2D_Qwr::StrokeText( const std::wstring& text, double x, double y )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y ) )
    {
        return;
    }

    auto cleanText = PrepareText( text );

    auto pGraphicsPath = std::make_unique<Gdiplus::GraphicsPath>();
    qwr::error::CheckGdiPlusObject( pGraphicsPath );

    const auto drawPoint = GenerateTextOriginPoint( text, x, y );
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_ );

    auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                            cleanText.size(),
                                            gdiPlusFontData.pFontFamily.get(),
                                            gdiPlusFontData.pFont->GetStyle(),
                                            gdiPlusFontData.pFont->GetSize(),
                                            drawPoint,
                                            &stringFormat_ );
    qwr::error::CheckGdi( gdiRet, "AddString" );

    const auto pGradientPen = [&]() -> std::unique_ptr<Gdiplus::Pen> {
        if ( !pStrokeGradient_ )
        {
            return nullptr;
        }

        Gdiplus::RectF bounds;
        auto gdiRet = pGraphicsPath->GetBounds( &bounds, nullptr, nullptr );
        qwr::error::CheckGdi( gdiRet, "GetBounds" );

        return GenerateGradientStrokePen( RectToPoints( bounds ) );
    }();
    if ( pStrokeGradient_ && !pGradientPen )
    { // means that gradient data is empty
        return;
    }

    gdiRet = pGraphics_->DrawPath( ( pGradientPen ? pGradientPen.get() : pStrokePen_.get() ), pGraphicsPath.get() );
    qwr::error::CheckGdi( gdiRet, "DrawPath" );
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

std::wstring CanvasRenderingContext2D_Qwr::get_Font()
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

qwr::u8string CanvasRenderingContext2D_Qwr::get_TextAlign() const
{
    switch ( textAlign_ )
    {
    case TextAlign::start:
        return "start";
    case TextAlign::end:
        return "end";
    case TextAlign::left:
        return "left";
    case TextAlign::center:
        return "center";
    case TextAlign::right:
        return "right";
    default:
    {
        assert( false );
        return "start";
    }
    }
}

qwr::u8string CanvasRenderingContext2D_Qwr::get_TextBaseline() const
{
    switch ( textBaseline_ )
    {
    case TextBaseline::top:
        return "top";
    case TextBaseline::middle:
        return "middle";
    case TextBaseline::bottom:
        return "bottom";
    case TextBaseline::alphabetic:
        return "alphabetic";
    default:
    {
        assert( false );
        return "alphabetic";
    }
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

void CanvasRenderingContext2D_Qwr::put_Font( const std::wstring& value )
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

void CanvasRenderingContext2D_Qwr::put_TextAlign( const qwr::u8string& value )
{
    if ( value == "start" )
    {
        textAlign_ = TextAlign::start;
    }
    else if ( value == "end" )
    {
        textAlign_ = TextAlign::end;
    }
    else if ( value == "left" )
    {
        textAlign_ = TextAlign::left;
    }
    else if ( value == "right" )
    {
        textAlign_ = TextAlign::right;
    }
    else if ( value == "center" )
    {
        textAlign_ = TextAlign::center;
    }
}

void CanvasRenderingContext2D_Qwr::put_TextBaseline( const qwr::u8string& value )
{
    if ( value == "top" )
    {
        textBaseline_ = TextBaseline::top;
    }
    else if ( value == "middle" )
    {
        textBaseline_ = TextBaseline::middle;
    }
    else if ( value == "bottom" )
    {
        textBaseline_ = TextBaseline::bottom;
    }
    else if ( value == "alphabetic" )
    {
        textBaseline_ = TextBaseline::alphabetic;
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

Gdiplus::PointF CanvasRenderingContext2D_Qwr::GenerateTextOriginPoint( const std::wstring& text, double x, double y )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_ );

    Gdiplus::PointF drawPoint{ static_cast<float>( x ), static_cast<float>( y ) };
    if ( textBaseline_ == TextBaseline::alphabetic )
    {
        drawPoint.Y -= gdiPlusFontData.ascentHeight;
    }
    else
    { // this is the only way I've found that makes it look the same as in browsers...
        drawPoint.Y -= gdiPlusFontData.lineHeight / 2;
        if ( textBaseline_ == TextBaseline::top )
        {
            drawPoint.Y -= static_cast<float>( fontDescription_.size / 2 );
        }
        else if ( textBaseline_ == TextBaseline::bottom )
        {
            drawPoint.Y += static_cast<float>( fontDescription_.size / 2 );
        }
    }

    if ( textAlign_ != TextAlign::left && textAlign_ != TextAlign::start )
    {
        Gdiplus::RectF bounds;
        auto gdiRet = pGraphics_->MeasureString( text.c_str(),
                                                 text.size(),
                                                 gdiPlusFontData.pFont.get(),
                                                 drawPoint,
                                                 &stringFormat_,
                                                 &bounds );
        qwr::error::CheckGdi( gdiRet, "MeasureString" );

        if ( textAlign_ == TextAlign::right || textAlign_ == TextAlign::end )
        {
            drawPoint.X -= bounds.Width;
        }
        else if ( textAlign_ == TextAlign::center )
        {
            drawPoint.X -= bounds.Width / 2;
        }
    }

    return drawPoint;
}

CanvasRenderingContext2D_Qwr::FillTextExOptions
CanvasRenderingContext2D_Qwr::ParseOptions_FillTextEx( JS::HandleValue options )
{
    if ( options.isNullOrUndefined() )
    {
        return {};
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );

    FillTextExOptions parsedOptions;

    qwr::u8string textDecoration;
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "text-decoration", textDecoration );
    qwr::u8string_view textDecorationSv{ textDecoration };

    // TODO: move to constants
    static const std::unordered_set<qwr::u8string_view> kKnownTextDecorationValues{
        {
            "none",
            "underline",
            "line-through",
        }
    };
    std::unordered_set<qwr::u8string_view> textVisitedComponents;
    for ( const auto& component: qwr::string::SplitView( textDecorationSv, " "sv ) )
    {
        if ( textVisitedComponents.contains( component ) || !kKnownTextDecorationValues.contains( component ) )
        {
            textVisitedComponents.clear();
            break;
        }
        textVisitedComponents.emplace( component );
    }
    if ( !textVisitedComponents.contains( "none"sv ) )
    {
        parsedOptions.hasUnderline = textVisitedComponents.contains( "underline"sv );
        parsedOptions.hasLineThrough = textVisitedComponents.contains( "line-through"sv );
    }

    qwr::u8string whiteSpace;
    // TODO: move to constants
    static const std::unordered_set<qwr::u8string> kKnownWhiteSpaceValues{
        {
            "normal",
            "nowrap",
            "pre-wrap",
            "pre",
        }
    };
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "white-space", whiteSpace );
    if ( kKnownWhiteSpaceValues.contains( whiteSpace ) )
    {
        parsedOptions.shouldCollapseNewLines = ( whiteSpace == "normal" || whiteSpace == "nowrap" );
        parsedOptions.shouldCollapseSpaces = ( whiteSpace == "normal" || whiteSpace == "nowrap" );
        parsedOptions.shouldWrapText = ( whiteSpace == "normal" || whiteSpace == "pre-wrap" );
    }

    qwr::u8string overflow;
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "overflow", overflow );
    if ( overflow == "visible" )
    {
        parsedOptions.shouldClipByRect = false;
    }

    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "text-overflow", parsedOptions.textOverflow );
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "render-mode", parsedOptions.renderMode );
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "width", parsedOptions.width );
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "height", parsedOptions.height );

    if ( !parsedOptions.width || !parsedOptions.height )
    {
        parsedOptions.shouldWrapText = false;
    }

    return parsedOptions;
}

float CanvasRenderingContext2D_Qwr::GenerateTextOriginY_FillTextEx( const std::wstring& text, double y, double ascentHeight, const FillTextExOptions& options )
{
    auto drawPointY = y;
    if ( textBaseline_ == TextBaseline::alphabetic )
    {
        if ( options.height && options.width )
        {
            if ( !options.shouldWrapText )
            {
                drawPointY -= ascentHeight;
                drawPointY += options.height;
            }
        }
        else
        {
            drawPointY -= ascentHeight;
        }
    }

    return static_cast<float>( drawPointY );
}

std::wstring CanvasRenderingContext2D_Qwr::PrepareText_FillTextEx( const std::wstring& text, const FillTextExOptions& options )
{
    auto cleanText = text;
    for ( auto& ch: cleanText )
    {
        if ( ch == L'\t' && options.shouldCollapseSpaces )
        {
            ch = L' ';
        }
        if ( ( ch == L'\r' || ch == L'\n' ) && options.shouldCollapseNewLines )
        {
            ch = L' ';
        }
    }

    if ( !options.shouldUseCanvasCollapseRules && ( options.shouldCollapseSpaces || options.shouldCollapseNewLines ) )
    {
        auto it = std::unique( cleanText.begin(), cleanText.end(), []( wchar_t a, wchar_t b ) { return ( a == b ) && ( a == L' ' ); } );
        cleanText.erase( it, cleanText.end() );
    }

    return cleanText;
}

std::unique_ptr<Gdiplus::StringFormat>
CanvasRenderingContext2D_Qwr::GenerateStringFormat_FillTextEx( const FillTextExOptions& options )
{
    auto pStringFormat = std::make_unique<Gdiplus::StringFormat>( &stringFormat_ );
    qwr::error::CheckGdiPlusObject( pStringFormat );

    auto stringFormatValue = pStringFormat->GetFormatFlags();
    if ( options.shouldWrapText )
    {
        stringFormatValue &= ~Gdiplus::StringFormatFlagsNoWrap;
    }
    else
    {
        stringFormatValue |= Gdiplus::StringFormatFlagsNoWrap;
    }
    if ( options.shouldClipByRect )
    {
        stringFormatValue &= ~Gdiplus::StringFormatFlagsNoClip;
    }
    else
    {
        stringFormatValue |= Gdiplus::StringFormatFlagsNoClip;
    }
    stringFormatValue &= ~Gdiplus::StringFormatFlagsLineLimit;

    auto gdiRet = pStringFormat->SetFormatFlags( stringFormatValue );
    qwr::error::CheckGdi( gdiRet, "SetFormatFlags" );

    const auto trimmingFlag = [&] {
        if ( options.textOverflow == "ellipsis" )
        {
            return Gdiplus::StringTrimmingEllipsisWord;
        }
        else if ( options.textOverflow == "ellipsis-char" )
        {
            return Gdiplus::StringTrimmingEllipsisCharacter;
        }
        else if ( options.textOverflow == "clip-char" )
        {
            return Gdiplus::StringTrimmingCharacter;
        }
        else
        {
            if ( options.shouldClipByRect )
            {
                return Gdiplus::StringTrimmingWord;
            }
            else
            {
                return Gdiplus::StringTrimmingNone;
            }
        }
    }();
    gdiRet = pStringFormat->SetTrimming( trimmingFlag );
    qwr::error::CheckGdi( gdiRet, "SetTrimming" );

    const auto lineAlign = [&] {
        switch ( textBaseline_ )
        {
        case TextBaseline::top:
        case TextBaseline::alphabetic:
            return Gdiplus::StringAlignmentNear;
        case TextBaseline::middle:
            return Gdiplus::StringAlignmentCenter;
        case TextBaseline::bottom:
            return Gdiplus::StringAlignmentFar;
        default:
        {
            assert( false );
            return Gdiplus::StringAlignmentNear;
        }
        }
    }();
    gdiRet = pStringFormat->SetLineAlignment( lineAlign );
    qwr::error::CheckGdi( gdiRet, "SetLineAlignment" );

    const auto align = [&] {
        switch ( textAlign_ )
        {
        case TextAlign::start:
        case TextAlign::left:
            return Gdiplus::StringAlignmentNear;
        case TextAlign::end:
        case TextAlign::right:
            return Gdiplus::StringAlignmentFar;
        case TextAlign::center:
            return Gdiplus::StringAlignmentCenter;
        default:
        {
            assert( false );
            return Gdiplus::StringAlignmentNear;
        }
        }
    }();
    gdiRet = pStringFormat->SetAlignment( align );
    qwr::error::CheckGdi( gdiRet, "SetAlignment" );

    return pStringFormat;
}

int32_t CanvasRenderingContext2D_Qwr::GenerateStringFormat_GdiEx_FillTextEx( const FillTextExOptions& options )
{
    int32_t stringFormat = DT_NOPREFIX;

    switch ( textAlign_ )
    {
    case TextAlign::start:
    case TextAlign::left:
    {
        stringFormat |= DT_LEFT;
        break;
    }
    case TextAlign::end:
    case TextAlign::right:
    {
        stringFormat |= DT_RIGHT;
        break;
    }
    case TextAlign::center:
    {
        stringFormat |= DT_CENTER;
        break;
    }
    default:
    {
        assert( false );
        stringFormat |= DT_LEFT;
    }
    }

    switch ( textBaseline_ )
    {
    case TextBaseline::top:
    case TextBaseline::alphabetic:
    {
        stringFormat |= DT_TOP;
        break;
    }
    case TextBaseline::middle:
    {
        stringFormat |= DT_VCENTER;
        break;
    }
    case TextBaseline::bottom:
    {
        stringFormat |= DT_BOTTOM;
        break;
    }
    default:
    {
        assert( false );
        stringFormat |= DT_TOP;
    }
    }

    if ( !options.shouldClipByRect )
    {
        stringFormat |= DT_NOCLIP;
    }

    if ( options.shouldWrapText )
    {
        stringFormat |= DT_WORDBREAK | DT_EDITCONTROL;
    }

    if ( options.textOverflow == "ellipsis" )
    {
        stringFormat |= DT_WORD_ELLIPSIS;
    }
    else if ( options.textOverflow == "ellipsis-char" )
    {
        stringFormat |= DT_END_ELLIPSIS;
    }

    return stringFormat;
}

int32_t CanvasRenderingContext2D_Qwr::GenerateStringFormat_Gdi_FillTextEx( const FillTextExOptions& options )
{
    int32_t stringFormat = TA_NOUPDATECP;

    switch ( textAlign_ )
    {
    case TextAlign::start:
    case TextAlign::left:
    {
        stringFormat |= TA_LEFT;
        break;
    }
    case TextAlign::end:
    case TextAlign::right:
    {
        stringFormat |= TA_RIGHT;
        break;
    }
    case TextAlign::center:
    {
        stringFormat |= TA_CENTER;
        break;
    }
    default:
    {
        assert( false );
        stringFormat |= TA_LEFT;
    }
    }

    switch ( textBaseline_ )
    {
    case TextBaseline::top:
    case TextBaseline::alphabetic:
    {
        stringFormat |= TA_TOP;
        break;
    }
    case TextBaseline::middle:
    {
        // TODO: fix this
        stringFormat |= TA_TOP;
        break;
    }
    case TextBaseline::bottom:
    {
        stringFormat |= TA_BOTTOM;
        break;
    }
    default:
    {
        assert( false );
        stringFormat |= TA_TOP;
    }
    }

    return stringFormat;
}

void CanvasRenderingContext2D_Qwr::DrawString_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );

    const auto drawPointY = GenerateTextOriginY_FillTextEx( text, y, gdiPlusFontData.ascentHeight, options );
    const Gdiplus::PointF drawPoint{ static_cast<float>( x ), drawPointY };
    std::optional<Gdiplus::RectF> rectOpt;
    if ( options.width && options.height )
    {
        rectOpt.emplace( static_cast<float>( x ),
                         drawPointY,
                         static_cast<float>( options.width ),
                         static_cast<float>( options.height + ( drawPointY - y ) ) );
    }
    const auto pStringFormat = GenerateStringFormat_FillTextEx( options );

    const auto pGradientBrush = [&]() -> std::unique_ptr<Gdiplus::Brush> {
        if ( !pFillGradient_ )
        {
            return nullptr;
        }

        const auto rect = [&] {
            if ( rectOpt )
            {
                return *rectOpt;
            }
            else
            {
                Gdiplus::RectF bounds;
                auto gdiRet = pGraphics_->MeasureString( cleanText.c_str(),
                                                         cleanText.size(),
                                                         gdiPlusFontData.pFont.get(),
                                                         drawPoint,
                                                         &stringFormat_,
                                                         &bounds );
                qwr::error::CheckGdi( gdiRet, "MeasureString" );

                return bounds;
            }
        }();

        return GenerateLinearGradientBrush( pFillGradient_->GetGradientData(), RectToPoints( rect ), globalAlpha_ );
    }();
    if ( pFillGradient_ && !pGradientBrush )
    { // means that gradient data is empty
        return;
    }

    if ( rectOpt )
    {
        auto gdiRet = pGraphics_->DrawString( cleanText.c_str(), cleanText.size(), gdiPlusFontData.pFont.get(), *rectOpt, pStringFormat.get(), ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ) );
        qwr::error::CheckGdi( gdiRet, "DrawString" );
    }
    else
    {
        auto gdiRet = pGraphics_->DrawString( cleanText.c_str(), cleanText.size(), gdiPlusFontData.pFont.get(), drawPoint, pStringFormat.get(), ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ) );
        qwr::error::CheckGdi( gdiRet, "DrawString" );
    }
}

void CanvasRenderingContext2D_Qwr::DrawPath_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );

    const auto drawPointY = GenerateTextOriginY_FillTextEx( text, y, gdiPlusFontData.ascentHeight, options );
    const Gdiplus::PointF drawPoint{ static_cast<float>( x ), drawPointY };
    std::optional<Gdiplus::RectF> rectOpt;
    if ( options.width && options.height )
    {
        rectOpt.emplace( static_cast<float>( x ),
                         drawPointY,
                         static_cast<float>( options.width ),
                         static_cast<float>( options.height + ( drawPointY - y ) ) );
    }
    const auto pStringFormat = GenerateStringFormat_FillTextEx( options );

    auto pGraphicsPath = std::make_unique<Gdiplus::GraphicsPath>();
    qwr::error::CheckGdiPlusObject( pGraphicsPath );

    if ( rectOpt )
    {
        auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                                cleanText.size(),
                                                gdiPlusFontData.pFontFamily.get(),
                                                gdiPlusFontData.pFont->GetStyle(),
                                                gdiPlusFontData.pFont->GetSize(),
                                                *rectOpt,
                                                pStringFormat.get() );
        qwr::error::CheckGdi( gdiRet, "AddString" );
    }
    else
    {
        auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                                cleanText.size(),
                                                gdiPlusFontData.pFontFamily.get(),
                                                gdiPlusFontData.pFont->GetStyle(),
                                                gdiPlusFontData.pFont->GetSize(),
                                                drawPoint,
                                                pStringFormat.get() );
        qwr::error::CheckGdi( gdiRet, "AddString" );
    }

    const auto pGradientBrush = [&]() -> std::unique_ptr<Gdiplus::Brush> {
        if ( !pFillGradient_ )
        {
            return nullptr;
        }

        const auto rect = [&] {
            if ( rectOpt )
            {
                return *rectOpt;
            }
            else
            {
                Gdiplus::RectF bounds;
                auto gdiRet = pGraphicsPath->GetBounds( &bounds, nullptr, nullptr );
                qwr::error::CheckGdi( gdiRet, "GetBounds" );

                return bounds;
            }
        }();

        return GenerateLinearGradientBrush( pFillGradient_->GetGradientData(), RectToPoints( rect ), globalAlpha_ );
    }();
    if ( pFillGradient_ && !pGradientBrush )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphics_->FillPath( ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ), pGraphicsPath.get() );
    qwr::error::CheckGdi( gdiRet, "FillPath" );
}

void CanvasRenderingContext2D_Qwr::DrawGdiString_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options )
{
    const auto& gdiFontData = FetchGdiFont( *pGraphics_, fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );

    const auto iX = static_cast<int32_t>( x );
    auto iY = static_cast<int32_t>( GenerateTextOriginY_FillTextEx( text, y, gdiFontData.ascentHeight, options ) );

    std::optional<RECT> rectOpt;
    if ( options.width && options.height )
    {
        rectOpt.emplace( iX,
                         iY,
                         iX + static_cast<int32_t>( options.width ),
                         iY + static_cast<int32_t>( options.height + ( iY - y ) ) );
    }

    const auto hDc = pGraphics_->GetHDC();
    qwr::final_action autoHdcReleaser( [&] { pGraphics_->ReleaseHDC( hDc ); } );
    smp::gdi::ObjectSelector autoFont( hDc, gdiFontData.pFont->m_hFont );

    Gdiplus::Color clr;
    auto gdiRet = pFillBrush_->GetColor( &clr );
    qwr::error::CheckGdi( gdiRet, "GetColor" );

    ::SetTextColor( hDc, smp::colour::ArgbToColorref( clr.GetValue() ) );

    auto iRet = ::SetBkMode( hDc, TRANSPARENT );
    qwr::error::CheckWinApi( iRet != CLR_INVALID, "SetBkMode" );

    if ( rectOpt )
    {
        const auto stringFormat = GenerateStringFormat_GdiEx_FillTextEx( options );

        auto uRet = ::SetTextAlign( hDc, TA_LEFT | TA_TOP | TA_NOUPDATECP );
        qwr::error::CheckWinApi( uRet != GDI_ERROR, "SetTextAlign" );

        iRet = ::DrawTextEx( hDc, const_cast<wchar_t*>( cleanText.c_str() ), cleanText.size(), &*rectOpt, stringFormat, nullptr );
        qwr::error::CheckWinApi( iRet, "DrawTextEx" );
    }
    else
    {
        const auto stringFormat = GenerateStringFormat_Gdi_FillTextEx( options );

        auto uRet = ::SetTextAlign( hDc, stringFormat );
        qwr::error::CheckWinApi( uRet != GDI_ERROR, "SetTextAlign" );

        iRet = ::TextOutW( hDc, iX, iY, const_cast<wchar_t*>( cleanText.c_str() ), cleanText.size() );
        qwr::error::CheckWinApi( iRet, "TextOutW" );
    }
}

} // namespace mozjs
