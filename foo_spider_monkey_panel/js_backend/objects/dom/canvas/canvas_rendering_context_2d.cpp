#include <stdafx.h>

#include "canvas_rendering_context_2d.h"

#include <dom/axis_adjuster.h>
#include <dom/css_colours.h>
#include <dom/css_fonts.h>
#include <dom/double_helpers.h>
#include <graphics/gdiplus/bitmap_generator.h>
#include <graphics/gdiplus/gradient_clamp.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas.h>
#include <js_backend/objects/dom/canvas/canvas_gradient.h>
#include <js_backend/objects/dom/canvas/image_bitmap.h>
#include <js_backend/objects/dom/canvas/image_data.h>
#include <js_backend/objects/dom/window/image.h>
#include <js_backend/utils/js_property_helper.h>
#include <utils/colour_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/string_utils.h>

#include <js/experimental/TypedData.h>
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

struct GdiPlusFontData
{
    std::unique_ptr<const Gdiplus::Font> pFont;
    std::unique_ptr<const Gdiplus::FontFamily> pFontFamily;
    float ascentHeight;
    float descentHeight;
    float lineHeight;
};

struct GdiFontData
{
    std::unique_ptr<CFont> pFont;
    int32_t ascentHeight;
    int32_t descentHeight;
    int32_t lineHeight;
    // TODO: fix this, probably caused by leading paddings
    int32_t magicLineHeight;
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
    smp::graphics::ClampGradient( p0, p1, blendPositions, drawArea );

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
    smp::error::CheckGdiPlusObject( pBrush );

    auto gdiRet = pBrush->SetInterpolationColors( presetColors.data(), blendPositions.data(), blendPositions.size() );
    smp::error::CheckGdi( gdiRet, "SetInterpolationColors" );

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
    // TODO: lower case css font
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
    smp::error::CheckGdiPlusObject( pFamily );

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
    smp::error::CheckGdiPlusObject( pFont );

    // TODO: handle dpi and units here
    const auto fontStyle = pFont->GetStyle();
    const auto lineHeight = fontDescription.size * pFamily->GetLineSpacing( fontStyle ) / pFamily->GetEmHeight( fontStyle );
    const auto ascentHeight = fontDescription.size * pFamily->GetCellAscent( fontStyle ) / pFamily->GetEmHeight( fontStyle );
    const auto descentHeight = fontDescription.size * pFamily->GetCellDescent( fontStyle ) / pFamily->GetEmHeight( fontStyle );

    const auto [it, isEmplaced] = cssStrToFontData.try_emplace(
        cssFont,
        GdiPlusFontData{
            std::move( pFont ),
            std::move( pFamily ),
            static_cast<float>( ascentHeight ),
            static_cast<float>( descentHeight ),
            static_cast<float>( lineHeight ) } );
    return it->second;
}

/// @throw qwr::QwrException
const GdiFontData& FetchGdiFont( Gdiplus::Graphics& graphics, const smp::dom::FontDescription& fontDescription, bool isUnderlined = false, bool isStrikeout = false )
{
    // TODO: lower case css font
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

    const auto trueLineHeight = metrics.tmAscent + metrics.tmInternalLeading + metrics.tmExternalLeading + metrics.tmDescent;
    const auto magicLineHeight = fontDescription.size * trueLineHeight / metrics.tmHeight;

    const auto [it, isEmplaced] = cssStrToFontData.try_emplace(
        cssFont,
        GdiFontData{
            std::move( pFont ),
            static_cast<int32_t>( metrics.tmAscent ),
            static_cast<int32_t>( metrics.tmDescent ),
            static_cast<int32_t>( trueLineHeight ),
            static_cast<int32_t>( magicLineHeight ) } );
    return it->second;
}

inline void SwapRBColours( uint8_t* data, size_t size )
{
    for ( size_t i = 0; i < size; i += 4 )
    {
        std::swap( data[i], data[i + 2] );
    }
}

Gdiplus::Rect IntersectRects( const Gdiplus::Rect& src, const Gdiplus::Rect& dst )
{
    auto result = src;
    result.Intersect( dst );
    return result;
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
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( beginPath, CanvasRenderingContext2D_Qwr::BeginPath );
MJS_DEFINE_JS_FN_FROM_NATIVE( createLinearGradient, CanvasRenderingContext2D_Qwr::CreateLinearGradient );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( drawImage, CanvasRenderingContext2D_Qwr::DrawImage3, CanvasRenderingContext2D_Qwr::DrawImageWithOpt, 6 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ellipse, CanvasRenderingContext2D_Qwr::Ellipse, CanvasRenderingContext2D_Qwr::EllipseWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( fill, CanvasRenderingContext2D_Qwr::Fill, CanvasRenderingContext2D_Qwr::FillWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( fillRect, CanvasRenderingContext2D_Qwr::FillRect );
MJS_DEFINE_JS_FN_FROM_NATIVE( fillText, CanvasRenderingContext2D_Qwr::FillText );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( fillTextEx, CanvasRenderingContext2D_Qwr::FillTextEx, CanvasRenderingContext2D_Qwr::FillTextExWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( getImageData, CanvasRenderingContext2D_Qwr::GetImageData );
MJS_DEFINE_JS_FN_FROM_NATIVE( lineTo, CanvasRenderingContext2D_Qwr::LineTo );
MJS_DEFINE_JS_FN_FROM_NATIVE( measureText, CanvasRenderingContext2D_Qwr::MeasureText );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( measureTextEx, CanvasRenderingContext2D_Qwr::MeasureTextEx, CanvasRenderingContext2D_Qwr::MeasureTextExWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( moveTo, CanvasRenderingContext2D_Qwr::MoveTo );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( putImageData, CanvasRenderingContext2D_Qwr::PutImageData, CanvasRenderingContext2D_Qwr::PutImageDataWithOpt, 4 );
MJS_DEFINE_JS_FN_FROM_NATIVE( roundRect, CanvasRenderingContext2D_Qwr::RoundRect );
MJS_DEFINE_JS_FN_FROM_NATIVE( stroke, CanvasRenderingContext2D_Qwr::Stroke );
MJS_DEFINE_JS_FN_FROM_NATIVE( strokeRect, CanvasRenderingContext2D_Qwr::StrokeRect );
MJS_DEFINE_JS_FN_FROM_NATIVE( strokeText, CanvasRenderingContext2D_Qwr::StrokeText );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "beginPath", beginPath, 0, kDefaultPropsFlags ),
        JS_FN( "createLinearGradient", createLinearGradient, 4, kDefaultPropsFlags ),
        JS_FN( "drawImage", drawImage, 3, kDefaultPropsFlags ),
        JS_FN( "ellipse", ellipse, 7, kDefaultPropsFlags ),
        JS_FN( "fill", fill, 0, kDefaultPropsFlags ),
        JS_FN( "fillRect", fillRect, 4, kDefaultPropsFlags ),
        JS_FN( "fillText", fillText, 3, kDefaultPropsFlags ),
        JS_FN( "fillTextEx", fillTextEx, 3, kDefaultPropsFlags ),
        JS_FN( "getImageData", getImageData, 4, kDefaultPropsFlags ),
        JS_FN( "lineTo", lineTo, 2, kDefaultPropsFlags ),
        JS_FN( "measureText", measureText, 1, kDefaultPropsFlags ),
        JS_FN( "measureTextEx", measureTextEx, 1, kDefaultPropsFlags ),
        JS_FN( "moveTo", moveTo, 2, kDefaultPropsFlags ),
        JS_FN( "putImageData", putImageData, 3, kDefaultPropsFlags ),
        JS_FN( "roundRect", roundRect, 5, kDefaultPropsFlags ),
        JS_FN( "stroke", stroke, 0, kDefaultPropsFlags ),
        JS_FN( "strokeRect", strokeRect, 4, kDefaultPropsFlags ),
        JS_FN( "strokeText", strokeText, 3, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_fillStyle, mozjs::CanvasRenderingContext2D_Qwr::get_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_font, mozjs::CanvasRenderingContext2D_Qwr::get_Font )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_globalAlpha, mozjs::CanvasRenderingContext2D_Qwr::get_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_globalCompositeOperation, mozjs::CanvasRenderingContext2D_Qwr::get_GlobalCompositeOperation )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_strokeStyle, mozjs::CanvasRenderingContext2D_Qwr::get_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_lineJoin, mozjs::CanvasRenderingContext2D_Qwr::get_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_lineWidth, mozjs::CanvasRenderingContext2D_Qwr::get_LineWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_textAlign, mozjs::CanvasRenderingContext2D_Qwr::get_TextAlign )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_textBaseline, mozjs::CanvasRenderingContext2D_Qwr::get_TextBaseline )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_fillStyle, mozjs::CanvasRenderingContext2D_Qwr::put_FillStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_font, mozjs::CanvasRenderingContext2D_Qwr::put_Font )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_globalAlpha, mozjs::CanvasRenderingContext2D_Qwr::put_GlobalAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_globalCompositeOperation, mozjs::CanvasRenderingContext2D_Qwr::put_GlobalCompositeOperation )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_strokeStyle, mozjs::CanvasRenderingContext2D_Qwr::put_StrokeStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_lineJoin, mozjs::CanvasRenderingContext2D_Qwr::put_LineJoin )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_lineWidth, mozjs::CanvasRenderingContext2D_Qwr::put_LineWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_textAlign, mozjs::CanvasRenderingContext2D_Qwr::put_TextAlign )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_textBaseline, mozjs::CanvasRenderingContext2D_Qwr::put_TextBaseline )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "fillStyle", get_fillStyle, put_fillStyle, kDefaultPropsFlags ),
        JS_PSGS( "font", get_font, put_font, kDefaultPropsFlags ),
        JS_PSGS( "globalAlpha", get_globalAlpha, put_globalAlpha, kDefaultPropsFlags ),
        JS_PSGS( "globalCompositeOperation", get_globalCompositeOperation, put_globalCompositeOperation, kDefaultPropsFlags ),
        JS_PSGS( "lineJoin", get_lineJoin, put_lineJoin, kDefaultPropsFlags ),
        JS_PSGS( "lineWidth", get_lineWidth, put_lineWidth, kDefaultPropsFlags ),
        JS_PSGS( "strokeStyle", get_strokeStyle, put_strokeStyle, kDefaultPropsFlags ),
        JS_PSGS( "textAlign", get_textAlign, put_textAlign, kDefaultPropsFlags ),
        JS_PSGS( "textBaseline", get_textBaseline, put_textBaseline, kDefaultPropsFlags ),
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

CanvasRenderingContext2D_Qwr::CanvasRenderingContext2D_Qwr( JSContext* cx, JS::HandleObject jsCanvas, ICanvasSurface& surface )
    : pJsCtx_( cx )
    , jsCanvas_( jsCanvas )
    , surface_( surface )
    , pGraphics_( &surface.GetGraphics() )
    , defaultStringFormat_( Gdiplus::StringFormat::GenericTypographic() )
{
    smp::error::CheckGdi( defaultStringFormat_.GetLastStatus(), "GenericTypographic" );

    Reinitialize();
}

CanvasRenderingContext2D_Qwr::~CanvasRenderingContext2D_Qwr()
{
}

std::unique_ptr<mozjs::CanvasRenderingContext2D_Qwr>
CanvasRenderingContext2D_Qwr::CreateNative( JSContext* cx, JS::HandleObject jsCanvas, ICanvasSurface& surface )
{
    return std::unique_ptr<CanvasRenderingContext2D_Qwr>( new CanvasRenderingContext2D_Qwr( cx, jsCanvas, surface ) );
}

size_t CanvasRenderingContext2D_Qwr::GetInternalSize() const
{
    return 0;
}

void CanvasRenderingContext2D_Qwr::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<CanvasRenderingContext2D_Qwr>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsCanvas_, "Heap: CanvasRenderingContext2D: canvas" );
    JS::TraceEdge( trc, &pNative->jsFillGradient_, "Heap: CanvasRenderingContext2D: fill gradient" );
    JS::TraceEdge( trc, &pNative->jsStrokeGradient_, "Heap: CanvasRenderingContext2D: stroke gradient" );
}

void CanvasRenderingContext2D_Qwr::Reinitialize()
{
    pGraphics_ = &surface_.GetGraphics();

    pFillBrush_ = std::make_unique<Gdiplus::SolidBrush>( Gdiplus ::Color{} );
    smp::error::CheckGdiPlusObject( pFillBrush_ );

    pStrokePen_ = std::make_unique<Gdiplus::Pen>( Gdiplus ::Color{} );
    smp::error::CheckGdiPlusObject( pStrokePen_ );

    pGraphicsPath_ = std::make_unique<Gdiplus::GraphicsPath>();
    smp::error::CheckGdiPlusObject( pGraphicsPath_ );

    globalAlpha_ = 1.0;
    originalFillColour_ = 0;
    originalStrokeColour_ = 0;
    jsFillGradient_ = nullptr;
    pFillGradient_ = nullptr;
    jsStrokeGradient_ = nullptr;
    pStrokeGradient_ = nullptr;
    lastPathPosOpt_.reset();
    fontDescription_ = smp::dom::FontDescription{};
    textAlign_ = TextAlign::start;
    textBaseline_ = TextBaseline::alphabetic;

    // TODO: fill with transparent black, if needed
}

void CanvasRenderingContext2D_Qwr::BeginPath()
{
    auto gdiRet = pGraphicsPath_->Reset();
    smp::error::CheckGdi( gdiRet, "Reset" );

    lastPathPosOpt_.reset();
}

JSObject* CanvasRenderingContext2D_Qwr::CreateLinearGradient( double x0, double y0, double x1, double y1 )
{
    qwr::QwrException::ExpectTrue( smp::dom::IsValidDouble( x0 ) && smp::dom::IsValidDouble( y0 )
                                       && smp::dom::IsValidDouble( x1 ) && smp::dom::IsValidDouble( y1 ),
                                   "Coordinate is not a finite floating-point value" );

    return JsObjectBase<CanvasGradient_Qwr>::CreateJs( pJsCtx_, x0, y0, x1, y1 );
}

void CanvasRenderingContext2D_Qwr::DrawImage1( JS::HandleValue image, double dx, double dy )
{
    if ( !smp::dom::IsValidDouble( dx ) || !smp::dom::IsValidDouble( dy ) )
    {
        return;
    }

    DrawImageImpl( image, dx, dy, {}, {}, {}, {}, {}, {} );
}

void CanvasRenderingContext2D_Qwr::DrawImage2( JS::HandleValue image, double dx, double dy, double dw, double dh )
{
    if ( !smp::dom::IsValidDouble( dx ) || !smp::dom::IsValidDouble( dy )
         || !smp::dom::IsValidDouble( dw ) || !smp::dom::IsValidDouble( dh ) )
    {
        return;
    }

    DrawImageImpl( image, dx, dy, dw, dh, {}, {}, {}, {} );
}

void CanvasRenderingContext2D_Qwr::DrawImage3( JS::HandleValue image, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh )
{
    if ( !smp::dom::IsValidDouble( sx ) || !smp::dom::IsValidDouble( sy )
         || !smp::dom::IsValidDouble( sw ) || !smp::dom::IsValidDouble( sh )
         || !smp::dom::IsValidDouble( dx ) || !smp::dom::IsValidDouble( dy )
         || !smp::dom::IsValidDouble( dw ) || !smp::dom::IsValidDouble( dh ) )
    {
        return;
    }

    DrawImageImpl( image, dx, dy, dw, dh, sx, sy, sw, sh );
}

void CanvasRenderingContext2D_Qwr::DrawImageWithOpt( size_t optArgCount, JS::HandleValue image,
                                                     double arg1, double arg2,
                                                     double arg3, double arg4,
                                                     double arg5, double arg6,
                                                     double arg7, double arg8 )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawImage3( image, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 );
    case 4:
        return DrawImage2( image, arg1, arg2, arg3, arg4 );
    case 6:
        return DrawImage1( image, arg1, arg2 );
    default:
        throw qwr::QwrException( "{} is not a valid argument count for any overload", 9 - optArgCount );
    }
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
    smp::error::CheckGdiPlusObject( pTmpGraphicsPath );

    // TODO: AddArc fails when startAngleInDegrees == sweepAngleInDegrees,
    // canvas API does not (and it sets last point at the corresponding angle)
    auto gdiRet = pTmpGraphicsPath->AddArc( rect, startAngleInDegrees, sweepAngleInDegrees );
    smp::error::CheckGdi( gdiRet, "AddArc" );

    if ( rotation )
    {
        Gdiplus::Matrix matrix;
        gdiRet = matrix.RotateAt( convertRadiansToDegrees( rotation ), centerPoint );
        smp::error::CheckGdi( gdiRet, "RotateAt" );

        gdiRet = pTmpGraphicsPath->Transform( &matrix );
        smp::error::CheckGdi( gdiRet, "Transform" );
    }

    gdiRet = pGraphicsPath_->AddPath( pTmpGraphicsPath.get(), true );
    smp::error::CheckGdi( gdiRet, "AddPath" );

    Gdiplus::PointF lastPoint;
    gdiRet = pGraphicsPath_->GetLastPoint( &lastPoint );
    smp::error::CheckGdi( gdiRet, "GetLastPoint" );

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

void CanvasRenderingContext2D_Qwr::Fill( const qwr::u8string& fillRule )
{
    qwr::QwrException::ExpectTrue( fillRule == "nonzero" || fillRule == "evenodd", "'{}' is not a valid value for enumeration CanvasWindingRule", fillRule );

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
        smp::error::CheckGdi( gdiRet, "GetBounds" );

        return GenerateLinearGradientBrush( pFillGradient_->GetGradientData(), RectToPoints( bounds ), globalAlpha_ );
    }();
    if ( pFillGradient_ && !pGradientBrush )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphicsPath_->SetFillMode( fillRule == "nonzero" ? Gdiplus::FillModeWinding : Gdiplus::FillModeAlternate );
    smp::error::CheckGdi( gdiRet, "SetFillMode" );

    gdiRet = pGraphics_->FillPath( ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ), pGraphicsPath_.get() );
    smp::error::CheckGdi( gdiRet, "FillPath" );
}

void CanvasRenderingContext2D_Qwr::FillWithOpt( size_t optArgCount, const qwr::u8string& fillRule )
{
    switch ( optArgCount )
    {
    case 0:
        return Fill( fillRule );
    case 1:
        return Fill();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void CanvasRenderingContext2D_Qwr::FillRect( double x, double y, double w, double h )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( w ) || !smp::dom::IsValidDouble( h )
         || !w || !h )
    {
        return;
    }

    smp::dom::AdjustAxis( x, w );
    smp::dom::AdjustAxis( y, h );

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
    smp::error::CheckGdi( gdiRet, "FillRectangle" );
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

JSObject* CanvasRenderingContext2D_Qwr::GetImageData( int32_t sx, int32_t sy, int32_t sw, int32_t sh )
{
    qwr::QwrException::ExpectTrue( sw, "The source width is 0" );
    qwr::QwrException::ExpectTrue( sw, "The source height is 0" );

    smp::dom::AdjustAxis( sx, sw );
    smp::dom::AdjustAxis( sy, sh );

    const Gdiplus::Rect dstRect{ 0, 0, sw, sh };
    const auto dstSize = dstRect.Width * dstRect.Height * 4;
    JS::RootedObject jsArray( pJsCtx_, JS_NewUint8ClampedArray( pJsCtx_, dstSize ) );
    smp::JsException::ExpectTrue( jsArray );

    const Gdiplus::Rect requestedImageRect{ sx, sy, sw, sh };
    const auto imageRect = IntersectRects( requestedImageRect,
                                           { 0, 0, static_cast<int32_t>( surface_.GetWidth() ), static_cast<int32_t>( surface_.GetHeight() ) } );
    if ( imageRect.IsEmptyArea() )
    {
        return ImageData::CreateJs( pJsCtx_, sw, sh, jsArray );
    }

    std::vector<uint8_t> imageData( imageRect.Width * imageRect.Height * 4 );

    // TODO: cleanup this mess, hide it in surface interface somehow
    if ( surface_.IsDevice() )
    {
        CDCHandle cDc = pGraphics_->GetHDC();
        qwr::final_action autoHdcReleaser( [&] { pGraphics_->ReleaseHDC( cDc ); } );

        CBitmap bitmap{ ::CreateCompatibleBitmap( cDc, imageRect.Width, imageRect.Height ) };
        qwr::error::CheckWinApi( bitmap, "CreateCompatibleBitmap" );

        {
            CDC memDc{ ::CreateCompatibleDC( cDc ) };
            qwr::error::CheckWinApi( memDc, "CreateCompatibleDC" );

            gdi::ObjectSelector autoBmp( memDc, bitmap.m_hBitmap );

            auto bRet = memDc.BitBlt( 0, 0, imageRect.Width, imageRect.Height, cDc, imageRect.X, imageRect.Y, SRCCOPY );
            qwr::error::CheckWinApi( bitmap, "BitBlt" );
        }

        BITMAPINFOHEADER bmpInfoHeader{};
        bmpInfoHeader.biSize = sizeof( BITMAPINFOHEADER );
        bmpInfoHeader.biWidth = imageRect.Width;
        bmpInfoHeader.biHeight = -imageRect.Height; // negative height for top-down DIB
        bmpInfoHeader.biPlanes = 1;
        bmpInfoHeader.biBitCount = 32;
        bmpInfoHeader.biCompression = BI_RGB;

        auto iRet = bitmap.GetDIBits( cDc, 0, imageRect.Height, imageData.data(), reinterpret_cast<BITMAPINFO*>( &bmpInfoHeader ), DIB_RGB_COLORS );
        qwr::error::CheckWinApi( iRet, "GetBitmap" );
    }
    else
    {
        Gdiplus::BitmapData bmpdata{};
        bmpdata.Width = imageRect.Width;
        bmpdata.Height = imageRect.Height;
        bmpdata.Stride = imageRect.Width * 4;
        bmpdata.Scan0 = imageData.data();

        auto gdiRet = surface_.GetBmp()->LockBits( &imageRect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bmpdata );
        smp::error::CheckGdi( gdiRet, "LockBits" );

        gdiRet = surface_.GetBmp()->UnlockBits( &bmpdata );
        smp::error::CheckGdi( gdiRet, "UnlockBits" );
    }

    {
        JS::AutoCheckCannotGC nogc;

        bool isShared;
        uint8_t* dstData = JS_GetUint8ClampedArrayData( jsArray, &isShared, nogc );
        assert( !isShared );

        if ( dstRect.Equals( requestedImageRect ) )
        {
            memcpy( dstData, imageData.data(), imageData.size() );
        }
        else
        {
            const auto srcStride = imageRect.Width * 4;
            const auto dstStride = dstRect.Width * 4;
            auto src = imageData.data();
            auto dst = dstData
                       + ( ( requestedImageRect.Y < imageRect.Y ) - ( requestedImageRect.Y > imageRect.Y ) )
                             * ( requestedImageRect.Height - imageRect.Height )
                             * dstStride
                       + ( ( requestedImageRect.X < imageRect.X ) - ( requestedImageRect.X > imageRect.X ) )
                             * ( requestedImageRect.Width - imageRect.Width )
                             * 4;

            for ( int32_t i = 0; i < imageRect.Height; ++i )
            {
                memcpy( dst, src, srcStride );

                src += srcStride;
                dst += dstStride;
            }
        }

        SwapRBColours( dstData, dstSize );
    }

    return ImageData::CreateJs( pJsCtx_, sw, sh, jsArray );
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
    smp::error::CheckGdi( gdiRet, "AddLine" );

    Gdiplus::PointF lastPoint;
    gdiRet = pGraphicsPath_->GetLastPoint( &lastPoint );
    smp::error::CheckGdi( gdiRet, "GetLastPoint" );

    lastPathPosOpt_ = pointTo;
}

JSObject* CanvasRenderingContext2D_Qwr::MeasureText( const std::wstring& text )
{
    FillTextExOptions options;
    options.shouldCollapseNewLines = true;
    options.shouldCollapseSpaces = true;
    options.shouldUseCanvasCollapseRules = true;

    const auto metricsData = MeasureString_FillTextEx( text, options );
    return JsObjectBase<TextMetrics>::CreateJs( pJsCtx_, metricsData );
}

JSObject* CanvasRenderingContext2D_Qwr::MeasureTextEx( const std::wstring& text, JS::HandleValue options )
{
    const auto parsedOptions = ParseOptions_FillTextEx( options );
    if ( !smp::dom::IsValidDouble( parsedOptions.width ) || !smp::dom::IsValidDouble( parsedOptions.height ) )
    {
        return JsObjectBase<TextMetrics>::CreateJs( pJsCtx_, TextMetrics::MetricsData{} );
    }

    const auto metricsData = [&] {
        if ( parsedOptions.renderMode == "clarity" )
        {
            return MeasureGdiString_FillTextEx( text, parsedOptions );
        }
        else if ( parsedOptions.renderMode == "stroke-compat" )
        {
            return MeasurePath_FillTextEx( text, parsedOptions );
        }
        else
        {
            return MeasureString_FillTextEx( text, parsedOptions );
        }
    }();
    return JsObjectBase<TextMetrics>::CreateJs( pJsCtx_, metricsData );
}

JSObject* CanvasRenderingContext2D_Qwr::MeasureTextExWithOpt( size_t optArgCount, const std::wstring& text, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return MeasureTextEx( text, options );
    case 1:
        return MeasureTextEx( text );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
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

void CanvasRenderingContext2D_Qwr::PutImageData( ImageData* imagedata, int32_t dx, int32_t dy, int32_t dirtyX, int32_t dirtyY, int32_t dirtyWidth, int32_t dirtyHeight )
{
    assert( imagedata );

    smp::dom::AdjustAxis( dirtyX, dirtyWidth );
    smp::dom::AdjustAxis( dirtyY, dirtyHeight );

    const auto imageWidth = static_cast<int32_t>( imagedata->GetWidth() );
    const auto imageHeight = static_cast<int32_t>( imagedata->GetHeight() );
    const Gdiplus::Rect imageRect{ 0, 0, imageWidth, imageHeight };
    const Gdiplus::Rect requestedImageRect{ dirtyX, dirtyY, dirtyWidth, dirtyHeight };
    const auto availableImageRect = IntersectRects( requestedImageRect, imageRect );
    const Gdiplus::Rect requestedDstRect{ dx, dy, availableImageRect.Width, availableImageRect.Height };
    const auto dstRect = IntersectRects( requestedDstRect,
                                         { 0, 0, static_cast<int32_t>( surface_.GetWidth() ), static_cast<int32_t>( surface_.GetHeight() ) } );
    const Gdiplus::Rect clippedImageRect{ availableImageRect.X, availableImageRect.Y, dstRect.Width, dstRect.Height };
    if ( dstRect.IsEmptyArea() )
    {
        return;
    }

    auto imageData = imagedata->GetDataCopy();

    std::vector<uint8_t> dstData( dstRect.Width * dstRect.Height * 4 );
    if ( imageRect.Equals( clippedImageRect ) && dstRect.Equals( imageRect ) )
    {
        memcpy( dstData.data(), imageData.data(), imageData.size() );
    }
    else
    {
        const auto srcStride = imageRect.Width * 4;
        const auto dstStride = dstRect.Width * 4;
        auto src = imageData.data()
                   + ( ( requestedDstRect.Y < dstRect.Y ) - ( requestedDstRect.Y > dstRect.Y ) )
                         * ( requestedDstRect.Height - dstRect.Height )
                         * srcStride
                   + ( ( requestedDstRect.X < dstRect.X ) - ( requestedDstRect.X > dstRect.X ) )
                         * ( requestedDstRect.Width - dstRect.Width )
                         * 4;
        auto dst = dstData.data();

        for ( int32_t i = 0; i < dstRect.Height; ++i )
        {
            memcpy( dst, src, dstStride );

            src += srcStride;
            dst += dstStride;
        }
    }

    SwapRBColours( dstData.data(), dstData.size() );

    // TODO: cleanup this mess, hide it in surface interface somehow
    if ( surface_.IsDevice() )
    {
        CDCHandle cDc = pGraphics_->GetHDC();
        qwr::final_action autoHdcReleaser( [&] { pGraphics_->ReleaseHDC( cDc ); } );

        CBitmap bitmap{ ::CreateCompatibleBitmap( cDc, dstRect.Width, dstRect.Height ) };
        qwr::error::CheckWinApi( bitmap, "CreateCompatibleBitmap" );

        CDC memDc{ ::CreateCompatibleDC( cDc ) };
        qwr::error::CheckWinApi( memDc, "CreateCompatibleDC" );

        BITMAPINFOHEADER bmpInfoHeader{};
        bmpInfoHeader.biSize = sizeof( BITMAPINFOHEADER );
        bmpInfoHeader.biWidth = dstRect.Width;
        bmpInfoHeader.biHeight = -dstRect.Height; // negative height for top-down DIB
        bmpInfoHeader.biPlanes = 1;
        bmpInfoHeader.biBitCount = 32;
        bmpInfoHeader.biCompression = BI_RGB;

        auto iRet = bitmap.SetDIBits( memDc, 0, dstRect.Height, dstData.data(), reinterpret_cast<BITMAPINFO*>( &bmpInfoHeader ), DIB_RGB_COLORS );
        qwr::error::CheckWinApi( iRet, "GetBitmap" );

        {
            gdi::ObjectSelector autoBmp( memDc, bitmap.m_hBitmap );

            auto bRet = cDc.BitBlt( dstRect.X, dstRect.Y, dstRect.Width, dstRect.Height, memDc, 0, 0, SRCCOPY );
            qwr::error::CheckWinApi( bitmap, "AlphaBlend" );
        }
    }
    else
    {
        Gdiplus::BitmapData bmpdata{};
        bmpdata.Width = dstRect.Width;
        bmpdata.Height = dstRect.Height;
        bmpdata.Stride = dstRect.Width * 4;
        bmpdata.Scan0 = dstData.data();

        auto gdiRet = surface_.GetBmp()->LockBits( &dstRect, Gdiplus::ImageLockModeWrite | Gdiplus::ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bmpdata );
        smp::error::CheckGdi( gdiRet, "LockBits" );

        gdiRet = surface_.GetBmp()->UnlockBits( &bmpdata );
        smp::error::CheckGdi( gdiRet, "UnlockBits" );
    }
}

void CanvasRenderingContext2D_Qwr::PutImageDataWithOpt( size_t optArgCount, ImageData* imagedata, int32_t dx, int32_t dy, int32_t dirtyX, int32_t dirtyY, int32_t dirtyWidth, int32_t dirtyHeight )
{
    qwr::QwrException::ExpectTrue( imagedata, "imagedata" );
    switch ( optArgCount )
    {
    case 0:
        return PutImageData( imagedata, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight );
    case 4:
        return PutImageData( imagedata, dx, dy, 0, 0, imagedata->GetWidth(), imagedata->GetHeight() );
    default:
        throw qwr::QwrException( "{} is not a valid argument count for any overload", 7 - optArgCount );
    }
}

void CanvasRenderingContext2D_Qwr::RoundRect( double x, double y, double w, double h, double radii )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( w ) || !smp::dom::IsValidDouble( h )
         || !smp::dom::IsValidDouble( radii ) )
    {
        return;
    }

    qwr::QwrException::ExpectTrue( radii >= 0, "Radius can not be negative" );

    smp::dom::AdjustAxis( x, w );
    smp::dom::AdjustAxis( y, h );

    auto upperLeft = radii;
    auto upperRight = radii;
    auto lowerLeft = radii;
    auto lowerRight = radii;
    const auto top = upperLeft + upperRight;
    const auto right = upperRight + lowerRight;
    const auto bottom = lowerRight + lowerLeft;
    const auto left = upperLeft + lowerLeft;
    const auto scale = std::min( { w / top, h / right, w / bottom, h / left } );
    if ( scale < 1 )
    {
        upperLeft *= scale;
        upperRight *= scale;
        lowerLeft *= scale;
        lowerRight *= scale;
    }

    auto generateArcRectFromPoints = []( const auto& from, const auto& to ) {
        const auto xDir = ( to.X > from.X ) - ( to.X < from.X );
        const auto yDir = ( to.Y > from.Y ) - ( to.Y < from.Y );

        Gdiplus::RectF rect{};
        rect.Width = 2 * std::abs( from.X - to.X );
        rect.Height = 2 * std::abs( from.Y - to.Y );

        if ( xDir > 0 && yDir > 0 )
        {
            rect.X = to.X - rect.Width;
            rect.Y = from.Y;
        }
        else if ( xDir < 0 && yDir > 0 )
        {
            rect.X = from.X - rect.Width;
            rect.Y = to.Y - rect.Height;
        }
        else if ( xDir < 0 && yDir < 0 )
        {
            rect.X = to.X;
            rect.Y = from.Y - rect.Height;
        }
        else if ( xDir > 0 && yDir < 0 )
        {
            rect.X = from.X;
            rect.Y = to.Y;
        }

        return rect;
    };

    auto gdiRet = pGraphicsPath_->StartFigure();
    smp::error::CheckGdi( gdiRet, "StartFigure" );

    Gdiplus::PointF curPoint{ static_cast<float>( x + w - upperRight ), static_cast<float>( y ) };
    Gdiplus::PointF nextPoint{ static_cast<float>( x + w ), static_cast<float>( y + upperRight ) };
    gdiRet = pGraphicsPath_->AddArc( generateArcRectFromPoints( curPoint, nextPoint ), 270.0f, 90.0f );
    smp::error::CheckGdi( gdiRet, "AddArc" );

    curPoint = { static_cast<float>( x + w ), static_cast<float>( y + h - lowerRight ) };
    nextPoint = { static_cast<float>( x + w - lowerRight ), static_cast<float>( y + h ) };
    gdiRet = pGraphicsPath_->AddArc( generateArcRectFromPoints( curPoint, nextPoint ), 0.0f, 90.0f );
    smp::error::CheckGdi( gdiRet, "AddArc" );

    curPoint = { static_cast<float>( x + lowerLeft ), static_cast<float>( y + h ) };
    nextPoint = { static_cast<float>( x ), static_cast<float>( y + h - lowerLeft ) };
    gdiRet = pGraphicsPath_->AddArc( generateArcRectFromPoints( curPoint, nextPoint ), 90.0f, 90.0f );
    smp::error::CheckGdi( gdiRet, "AddArc" );

    curPoint = { static_cast<float>( x ), static_cast<float>( y + upperLeft ) };
    nextPoint = { static_cast<float>( x + upperLeft ), static_cast<float>( y ) };
    gdiRet = pGraphicsPath_->AddArc( generateArcRectFromPoints( curPoint, nextPoint ), 180.0f, 90.0f );
    smp::error::CheckGdi( gdiRet, "AddArc" );

    gdiRet = pGraphicsPath_->CloseFigure();
    smp::error::CheckGdi( gdiRet, "CloseFigure" );

    lastPathPosOpt_ = nextPoint;
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
        smp::error::CheckGdi( gdiRet, "GetBounds" );

        return GenerateGradientStrokePen( RectToPoints( bounds ) );
    }();
    if ( pStrokeGradient_ && !pGradientPen )
    { // means that gradient data is empty
        return;
    }

    auto gdiRet = pGraphics_->DrawPath( ( pGradientPen ? pGradientPen.get() : pStrokePen_.get() ), pGraphicsPath_.get() );
    smp::error::CheckGdi( gdiRet, "DrawPath" );
}

void CanvasRenderingContext2D_Qwr::StrokeRect( double x, double y, double w, double h )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y )
         || !smp::dom::IsValidDouble( w ) || !smp::dom::IsValidDouble( h ) )
    {
        return;
    }

    smp::dom::AdjustAxis( x, w );
    smp::dom::AdjustAxis( y, h );

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
    smp::error::CheckGdi( gdiRet, "DrawRectangle" );
}

void CanvasRenderingContext2D_Qwr::StrokeText( const std::wstring& text, double x, double y )
{
    if ( !smp::dom::IsValidDouble( x ) || !smp::dom::IsValidDouble( y ) )
    {
        return;
    }

    auto cleanText = PrepareText( text );

    auto pGraphicsPath = std::make_unique<Gdiplus::GraphicsPath>();
    smp::error::CheckGdiPlusObject( pGraphicsPath );

    const auto drawPoint = GenerateTextOriginPoint( text, x, y );
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_ );

    auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                            cleanText.size(),
                                            gdiPlusFontData.pFontFamily.get(),
                                            gdiPlusFontData.pFont->GetStyle(),
                                            gdiPlusFontData.pFont->GetSize(),
                                            drawPoint,
                                            &defaultStringFormat_ );
    smp::error::CheckGdi( gdiRet, "AddString" );

    const auto pGradientPen = [&]() -> std::unique_ptr<Gdiplus::Pen> {
        if ( !pStrokeGradient_ )
        {
            return nullptr;
        }

        Gdiplus::RectF bounds;
        auto gdiRet = pGraphicsPath->GetBounds( &bounds, nullptr, nullptr );
        smp::error::CheckGdi( gdiRet, "GetBounds" );

        return GenerateGradientStrokePen( RectToPoints( bounds ) );
    }();
    if ( pStrokeGradient_ && !pGradientPen )
    { // means that gradient data is empty
        return;
    }

    gdiRet = pGraphics_->DrawPath( ( pGradientPen ? pGradientPen.get() : pStrokePen_.get() ), pGraphicsPath.get() );
    smp::error::CheckGdi( gdiRet, "DrawPath" );
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

JS::Value CanvasRenderingContext2D_Qwr::get_FillStyle() const
{
    if ( pFillGradient_ )
    {
        assert( jsFillGradient_.get() );
        return JS::ObjectValue( *jsFillGradient_ );
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

JS::Value CanvasRenderingContext2D_Qwr::get_StrokeStyle() const
{
    if ( pStrokeGradient_ )
    {
        assert( jsStrokeGradient_.get() );
        return JS::ObjectValue( *jsStrokeGradient_ );
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
    smp::error::CheckGdi( gdiRet, "SetCompositingMode" );
}

void CanvasRenderingContext2D_Qwr::put_FillStyle( JS::HandleValue jsValue )
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
        smp::error::CheckGdi( gdiRet, "SetColor" );

        originalFillColour_ = newColour;
        pFillGradient_ = nullptr;
        jsFillGradient_ = nullptr;
    }
    else
    {
        try
        {
            // first, verify with ExtractNative
            pFillGradient_ = JsObjectBase<CanvasGradient_Qwr>::ExtractNative( pJsCtx_, jsValue );
            qwr::QwrException::ExpectTrue( pFillGradient_, "style is not a supported object type" );
            // and only then save
            jsFillGradient_.set( &jsValue.toObject() );
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
    smp::error::CheckGdi( gdiRet, "SetColor" );

    gdiRet = pStrokePen_->SetColor( ApplyAlpha( originalStrokeColour_, globalAlpha_ ) );
    smp::error::CheckGdi( gdiRet, "SetColor" );
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
    smp::error::CheckGdi( gdiRet, "SetLineJoin" );
}

void CanvasRenderingContext2D_Qwr::put_LineWidth( double value )
{
    if ( !smp::dom::IsValidDouble( value ) || value <= 0 )
    {
        return;
    }

    auto gdiRet = pStrokePen_->SetWidth( static_cast<Gdiplus::REAL>( value ) );
    smp::error::CheckGdi( gdiRet, "SetWidth" );
}

void CanvasRenderingContext2D_Qwr::put_StrokeStyle( JS::HandleValue jsValue )
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
        smp::error::CheckGdi( gdiRet, "SetColor" );

        originalStrokeColour_ = newColour;
        pStrokeGradient_ = nullptr;
        jsStrokeGradient_ = nullptr;
    }
    else
    {
        try
        {
            // first, verify with ExtractNative
            pStrokeGradient_ = JsObjectBase<CanvasGradient_Qwr>::ExtractNative( pJsCtx_, jsValue );
            qwr::QwrException::ExpectTrue( pStrokeGradient_, "style is not a supported object type" );
            // and only then save
            jsStrokeGradient_.set( &jsValue.toObject() );
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

void CanvasRenderingContext2D_Qwr::DrawImageImpl( JS::HandleValue image,
                                                  double& dx, double dy,
                                                  const std::optional<double>& dh, const std::optional<double>& dw,
                                                  const std::optional<double>& sx, const std::optional<double>& sy,
                                                  const std::optional<double>& sw, const std::optional<double>& sh )
{
    qwr::QwrException::ExpectTrue( image.isObject(), "image argument is not an object" );
    JS::RootedObject jsObject( pJsCtx_, &image.toObject() );

    const auto drawBitmap = [&]( auto& bitmap ) {
        const auto bitmapW = bitmap.GetWidth();
        const auto bitmapH = bitmap.GetHeight();
        if ( !bitmapW || !bitmapH )
        {
            return;
        }

        auto srcX = static_cast<int32_t>( sx.value_or( 0 ) );
        auto srcY = static_cast<int32_t>( sy.value_or( 0 ) );
        auto srcW = static_cast<int32_t>( sw.value_or( bitmapW ) );
        auto srcH = static_cast<int32_t>( sh.value_or( bitmapH ) );
        auto dstX = static_cast<int32_t>( dx );
        auto dstY = static_cast<int32_t>( dy );
        auto dstW = static_cast<int32_t>( dw.value_or( srcW ) );
        auto dstH = static_cast<int32_t>( dh.value_or( srcH ) );
        if ( !srcW || !srcH )
        {
            return;
        }

        smp::dom::AdjustAxis( srcX, srcW );
        smp::dom::AdjustAxis( srcY, srcH );
        smp::dom::AdjustAxis( dstX, dstW );
        smp::dom::AdjustAxis( dstY, dstH );

        auto pImageAttributes = std::make_unique<Gdiplus::ImageAttributes>();
        smp::error::CheckGdiPlusObject( pImageAttributes );

        Gdiplus::ColorMatrix cm{};
        cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
        cm.m[3][3] = static_cast<float>( globalAlpha_ );

        auto gdiRet = pImageAttributes->SetColorMatrix( &cm );
        smp::error::CheckGdi( gdiRet, "SetColorMatrix" );

        Gdiplus::Rect dstRect{ dstX, dstY, dstW, dstH };
        gdiRet = pGraphics_->DrawImage( &bitmap, dstRect, srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, pImageAttributes.get() );
        smp::error::CheckGdi( gdiRet, "DrawImage" );
    };

    if ( auto pCanvas = Canvas::ExtractNative( pJsCtx_, jsObject ) )
    {
        auto& bitmap = pCanvas->GetBitmap();
        drawBitmap( bitmap );
    }
    else if ( auto pImage = JsObjectBase<Image>::ExtractNative( pJsCtx_, jsObject ) )
    {
        auto pWicBitmap = pImage->GetDecodedBitmap();
        if ( !pWicBitmap )
        {
            qwr::QwrException::ExpectTrue( pImage->GetStatus() != Image::CompleteStatus::broken, "Passed-in image is \"broken\"" );
            return;
        }

        auto pBitmap = smp::graphics::GenerateGdiBitmap( *pWicBitmap );
        drawBitmap( *pBitmap );
    }
    else if ( auto pImageBitmap = JsObjectBase<ImageBitmap>::ExtractNative( pJsCtx_, jsObject ) )
    {
        auto pBitmap = pImageBitmap->GetBitmap();
        if ( !pBitmap )
        {
            return;
        }

        drawBitmap( *pBitmap );
    }
    else
    {
        throw qwr::QwrException( "image argument can't be converted to supported object type" );
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
    smp::error::CheckGdiPlusObject( pPen );

    auto gdiRet = pPen->SetBrush( pBrush.get() );
    smp::error::CheckGdi( gdiRet, "SetBrush" );

    return pPen;
}

Gdiplus::PointF CanvasRenderingContext2D_Qwr::GenerateTextOriginPoint( const std::wstring& text, double x, double y )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_ );

    Gdiplus::PointF drawPoint{ static_cast<float>( x ), static_cast<float>( y ) };
    if ( textBaseline_ == TextBaseline::alphabetic )
    {
        const auto descentOffset = static_cast<int32_t>( gdiPlusFontData.ascentHeight + 0.5 );
        drawPoint.Y -= descentOffset;
    }
    else
    { // this is the only way I've found that makes it look the same as in browsers...
        drawPoint.Y -= gdiPlusFontData.lineHeight / 2;
        if ( textBaseline_ == TextBaseline::top )
        {
            drawPoint.Y += static_cast<float>( fontDescription_.size / 2 );
        }
        else if ( textBaseline_ == TextBaseline::bottom )
        {
            drawPoint.Y -= static_cast<float>( fontDescription_.size / 2 );
        }
    }

    if ( textAlign_ != TextAlign::left && textAlign_ != TextAlign::start )
    {
        Gdiplus::RectF bounds;
        auto gdiRet = pGraphics_->MeasureString( text.c_str(),
                                                 text.size(),
                                                 gdiPlusFontData.pFont.get(),
                                                 drawPoint,
                                                 &defaultStringFormat_,
                                                 &bounds );
        smp::error::CheckGdi( gdiRet, "MeasureString" );

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
    else
    {
        parsedOptions.shouldClipByRect = true;
        utils::OptionalPropertyTo( pJsCtx_, jsOptions, "text-overflow", parsedOptions.textOverflow );
    }

    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "render-mode", parsedOptions.renderMode );
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "width", parsedOptions.width );
    utils::OptionalPropertyTo( pJsCtx_, jsOptions, "height", parsedOptions.height );

    if ( !IsRect_FillTextEx( parsedOptions ) )
    {
        parsedOptions.shouldWrapText = false;
    }

    return parsedOptions;
}

float CanvasRenderingContext2D_Qwr::GenerateTextOriginY_FillTextEx( const std::wstring& text, double y, double ascentHeight, double lineHeight, const FillTextExOptions& options )
{
    const auto ascentOffset = static_cast<int32_t>( ascentHeight + 0.5 );
    auto drawPointY = y;

    if ( IsSingleLine_FillTextEx( options ) )
    {
        if ( textBaseline_ == TextBaseline::alphabetic )
        {
            drawPointY -= ascentOffset;
        }
        else
        { // this is the only way I've found that makes it look the same as in browsers...
            drawPointY -= lineHeight / 2;
            if ( textBaseline_ == TextBaseline::top )
            {
                drawPointY += static_cast<float>( fontDescription_.size / 2 );
            }
            else if ( textBaseline_ == TextBaseline::bottom )
            {
                drawPointY -= static_cast<float>( fontDescription_.size / 2 );
            }
        }
    }
    else if ( IsSingleLineRect_FillTextEx( options ) )
    {
        if ( textBaseline_ == TextBaseline::alphabetic )
        {
            drawPointY -= static_cast<float>( ascentOffset );
            drawPointY += static_cast<float>( options.height );
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
    auto pStringFormat = std::make_unique<Gdiplus::StringFormat>( &defaultStringFormat_ );
    smp::error::CheckGdiPlusObject( pStringFormat );

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
        stringFormatValue |= Gdiplus::StringFormatFlagsLineLimit;
    }
    else
    {
        stringFormatValue |= Gdiplus::StringFormatFlagsNoClip;
        stringFormatValue &= ~Gdiplus::StringFormatFlagsLineLimit;
    }

    auto gdiRet = pStringFormat->SetFormatFlags( stringFormatValue );
    smp::error::CheckGdi( gdiRet, "SetFormatFlags" );

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
    smp::error::CheckGdi( gdiRet, "SetTrimming" );

    const auto lineAlign = [&] {
        if ( IsSingleLine_FillTextEx( options ) )
        {
            return Gdiplus::StringAlignmentNear;
        }

        switch ( textBaseline_ )
        {
        case TextBaseline::alphabetic:
            return ( IsSingleLineRect_FillTextEx( options ) ? Gdiplus::StringAlignmentNear : Gdiplus::StringAlignmentFar );
        case TextBaseline::top:
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
    smp::error::CheckGdi( gdiRet, "SetLineAlignment" );

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
    smp::error::CheckGdi( gdiRet, "SetAlignment" );

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

    if ( IsSingleLine_FillTextEx( options ) )
    {
        stringFormat |= DT_TOP;
    }
    else
    {
        switch ( textBaseline_ )
        {
        case TextBaseline::alphabetic:
        {
            stringFormat |= ( IsSingleLineRect_FillTextEx( options ) ? DT_TOP : DT_BOTTOM );
            break;
        }
        case TextBaseline::top:
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
    }

    if ( !options.shouldClipByRect )
    {
        stringFormat |= DT_NOCLIP;
    }

    if ( options.shouldWrapText )
    {
        stringFormat |= DT_WORDBREAK | DT_EDITCONTROL;
    }

    if ( options.textOverflow == "ellipsis" || options.textOverflow == "ellipsis-char" )
    { // DT_END_ELLIPSIS only truncates the end of the final line
        stringFormat |= ( options.shouldWrapText ? DT_END_ELLIPSIS : DT_WORD_ELLIPSIS );
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

    if ( IsSingleLine_FillTextEx( options ) )
    {
        stringFormat |= TA_TOP;
    }
    else
    {
        switch ( textBaseline_ )
        {
        case TextBaseline::alphabetic:
        {
            stringFormat |= ( IsSingleLineRect_FillTextEx( options ) ? TA_TOP : TA_BOTTOM );
            break;
        }
        case TextBaseline::top:
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
    }

    return stringFormat;
}

void CanvasRenderingContext2D_Qwr::DrawString_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );
    const auto pStringFormat = GenerateStringFormat_FillTextEx( options );
    const auto drawPointY = GenerateTextOriginY_FillTextEx( text, y, gdiPlusFontData.ascentHeight, gdiPlusFontData.lineHeight, options );

    const Gdiplus::PointF drawPoint{ static_cast<float>( x ), drawPointY };
    std::optional<Gdiplus::RectF> rectOpt;
    if ( IsRect_FillTextEx( options ) )
    {
        rectOpt.emplace( static_cast<float>( x ),
                         drawPointY,
                         static_cast<float>( options.width ),
                         static_cast<float>( options.height + ( drawPointY - y ) ) );
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
                auto gdiRet = pGraphics_->MeasureString( cleanText.c_str(),
                                                         cleanText.size(),
                                                         gdiPlusFontData.pFont.get(),
                                                         drawPoint,
                                                         pStringFormat.get(),
                                                         &bounds );
                smp::error::CheckGdi( gdiRet, "MeasureString" );

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
        smp::error::CheckGdi( gdiRet, "DrawString" );
    }
    else
    {
        auto gdiRet = pGraphics_->DrawString( cleanText.c_str(), cleanText.size(), gdiPlusFontData.pFont.get(), drawPoint, pStringFormat.get(), ( pGradientBrush ? pGradientBrush.get() : pFillBrush_.get() ) );
        smp::error::CheckGdi( gdiRet, "DrawString" );
    }
}

void CanvasRenderingContext2D_Qwr::DrawPath_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );
    const auto pStringFormat = GenerateStringFormat_FillTextEx( options );
    const auto drawPointY = GenerateTextOriginY_FillTextEx( text, y, gdiPlusFontData.ascentHeight, gdiPlusFontData.lineHeight, options );

    const Gdiplus::PointF drawPoint{ static_cast<float>( x ), drawPointY };
    std::optional<Gdiplus::RectF> rectOpt;
    if ( IsRect_FillTextEx( options ) )
    {
        rectOpt.emplace( static_cast<float>( x ),
                         drawPointY,
                         static_cast<float>( options.width ),
                         static_cast<float>( options.height + ( drawPointY - y ) ) );
    }

    auto pGraphicsPath = std::make_unique<Gdiplus::GraphicsPath>();
    smp::error::CheckGdiPlusObject( pGraphicsPath );

    if ( rectOpt )
    {
        auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                                cleanText.size(),
                                                gdiPlusFontData.pFontFamily.get(),
                                                gdiPlusFontData.pFont->GetStyle(),
                                                gdiPlusFontData.pFont->GetSize(),
                                                *rectOpt,
                                                pStringFormat.get() );
        smp::error::CheckGdi( gdiRet, "AddString" );
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
        smp::error::CheckGdi( gdiRet, "AddString" );
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
                smp::error::CheckGdi( gdiRet, "GetBounds" );

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
    smp::error::CheckGdi( gdiRet, "FillPath" );
}

void CanvasRenderingContext2D_Qwr::DrawGdiString_FillTextEx( const std::wstring& text, double x, double y, const FillTextExOptions& options )
{
    const auto& gdiFontData = FetchGdiFont( *pGraphics_, fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );

    const auto iX = static_cast<int32_t>( x );
    auto iY = static_cast<int32_t>( GenerateTextOriginY_FillTextEx( text, y, gdiFontData.ascentHeight, gdiFontData.magicLineHeight, options ) );

    std::optional<CRect> rectOpt;
    if ( IsRect_FillTextEx( options ) )
    {
        rectOpt.emplace( iX,
                         iY,
                         iX + static_cast<int32_t>( options.width ),
                         iY + static_cast<int32_t>( options.height + ( iY - y ) ) );
    }

    CDCHandle cDc = pGraphics_->GetHDC();
    qwr::final_action autoHdcReleaser( [&] { pGraphics_->ReleaseHDC( cDc ); } );
    smp::gdi::ObjectSelector autoFont( cDc, gdiFontData.pFont->m_hFont );

    Gdiplus::Color clr;
    auto gdiRet = pFillBrush_->GetColor( &clr );
    smp::error::CheckGdi( gdiRet, "GetColor" );

    cDc.SetTextColor( smp::colour::ArgbToColorref( clr.GetValue() ) );

    auto iRet = cDc.SetBkMode( TRANSPARENT );
    qwr::error::CheckWinApi( iRet != CLR_INVALID, "SetBkMode" );

    if ( rectOpt )
    {
        const auto stringFormat = GenerateStringFormat_GdiEx_FillTextEx( options );

        auto uRet = cDc.SetTextAlign( TA_LEFT | TA_TOP | TA_NOUPDATECP );
        qwr::error::CheckWinApi( uRet != GDI_ERROR, "SetTextAlign" );

        { // vertical alignment is not applied for multiline text, so have to adjust it manually
            auto& rect = *rectOpt;
            const auto rectOld = rect;

            auto bounds = rect;
            iRet = cDc.DrawText( const_cast<wchar_t*>( cleanText.c_str() ), cleanText.size(), bounds, stringFormat | DT_CALCRECT );
            qwr::error::CheckWinApi( iRet, "DrawText" );

            if ( stringFormat & DT_VCENTER )
            {
                rect.top = rectOld.top + ( ( rectOld.bottom - rectOld.top ) - ( bounds.bottom - bounds.top ) ) / 2;
                rect.bottom = rect.top + ( bounds.bottom - bounds.top );
            }
            else if ( stringFormat & DT_BOTTOM )
            {
                rect.top = rectOld.bottom - ( bounds.bottom - bounds.top );
            }
        }

        iRet = cDc.DrawText( const_cast<wchar_t*>( cleanText.c_str() ), cleanText.size(), *rectOpt, stringFormat );
        qwr::error::CheckWinApi( iRet, "DrawText" );
    }
    else
    {
        const auto stringFormat = GenerateStringFormat_Gdi_FillTextEx( options );

        auto uRet = cDc.SetTextAlign( stringFormat );
        qwr::error::CheckWinApi( uRet != GDI_ERROR, "SetTextAlign" );

        iRet = cDc.TextOutW( iX, iY, const_cast<wchar_t*>( cleanText.c_str() ), cleanText.size() );
        qwr::error::CheckWinApi( iRet, "TextOutW" );
    }
}

TextMetrics::MetricsData
CanvasRenderingContext2D_Qwr::MeasureString_FillTextEx( const std::wstring& text, const FillTextExOptions& options )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );
    const auto pStringFormat = GenerateStringFormat_FillTextEx( options );

    std::optional<Gdiplus::RectF> rectOpt;
    if ( IsRect_FillTextEx( options ) )
    {
        rectOpt.emplace( 0.0f,
                         0.0f,
                         static_cast<float>( options.width ),
                         static_cast<float>( options.height ) );
    }

    Gdiplus::RectF bounds;
    if ( rectOpt )
    {
        auto gdiRet = pGraphics_->MeasureString( cleanText.c_str(),
                                                 cleanText.size(),
                                                 gdiPlusFontData.pFont.get(),
                                                 *rectOpt,
                                                 pStringFormat.get(),
                                                 &bounds );
        smp::error::CheckGdi( gdiRet, "MeasureString" );

        bounds.Width = std::min( bounds.Width, rectOpt->Width );
        bounds.Height = std::min( bounds.Height, rectOpt->Height );
    }
    else
    {
        auto gdiRet = pGraphics_->MeasureString( cleanText.c_str(),
                                                 cleanText.size(),
                                                 gdiPlusFontData.pFont.get(),
                                                 Gdiplus::PointF{},
                                                 pStringFormat.get(),
                                                 &bounds );
        smp::error::CheckGdi( gdiRet, "MeasureString" );
    }

    TextMetrics::MetricsData data;
    data.width = bounds.Width;
    data.height = bounds.Height;

    return data;
}

TextMetrics::MetricsData
CanvasRenderingContext2D_Qwr::MeasurePath_FillTextEx( const std::wstring& text, const FillTextExOptions& options )
{
    const auto& gdiPlusFontData = FetchGdiPlusFont( fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );
    const auto pStringFormat = GenerateStringFormat_FillTextEx( options );

    std::optional<Gdiplus::RectF> rectOpt;
    if ( IsRect_FillTextEx( options ) )
    {
        rectOpt.emplace( 0.0f,
                         0.0f,
                         static_cast<float>( options.width ),
                         static_cast<float>( options.height ) );
    }

    auto pGraphicsPath = std::make_unique<Gdiplus::GraphicsPath>();
    smp::error::CheckGdiPlusObject( pGraphicsPath );

    if ( rectOpt )
    {
        auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                                cleanText.size(),
                                                gdiPlusFontData.pFontFamily.get(),
                                                gdiPlusFontData.pFont->GetStyle(),
                                                gdiPlusFontData.pFont->GetSize(),
                                                *rectOpt,
                                                pStringFormat.get() );
        smp::error::CheckGdi( gdiRet, "AddString" );
    }
    else
    {
        auto gdiRet = pGraphicsPath->AddString( cleanText.c_str(),
                                                cleanText.size(),
                                                gdiPlusFontData.pFontFamily.get(),
                                                gdiPlusFontData.pFont->GetStyle(),
                                                gdiPlusFontData.pFont->GetSize(),
                                                Gdiplus::PointF{},
                                                pStringFormat.get() );
        smp::error::CheckGdi( gdiRet, "AddString" );
    }

    const auto pTempPen = std::make_unique<Gdiplus::Pen>( Gdiplus::Color{}, 0.00001f );
    smp::error::CheckGdiPlusObject( pTempPen );

    Gdiplus::RectF bounds;
    auto gdiRet = pGraphicsPath->GetBounds( &bounds, nullptr, pTempPen.get() );
    smp::error::CheckGdi( gdiRet, "GetBounds" );

    TextMetrics::MetricsData data;
    data.width = bounds.Width;
    data.height = bounds.Height;
    if ( rectOpt )
    {
        data.width = std::min<double>( data.width, rectOpt->Width );
        data.height = std::min<double>( data.height, rectOpt->Height );
    }

    return data;
}

TextMetrics::MetricsData
CanvasRenderingContext2D_Qwr::MeasureGdiString_FillTextEx( const std::wstring& text, const FillTextExOptions& options )
{
    const auto& gdiFontData = FetchGdiFont( *pGraphics_, fontDescription_, options.hasUnderline, options.hasLineThrough );
    const auto cleanText = PrepareText_FillTextEx( text, options );

    std::optional<CRect> rectOpt;
    if ( IsRect_FillTextEx( options ) )
    {
        rectOpt.emplace( 0,
                         0,
                         static_cast<int32_t>( options.width ),
                         static_cast<int32_t>( options.height ) );
    }

    CDCHandle cDc = pGraphics_->GetHDC();
    qwr::final_action autoHdcReleaser( [&] { pGraphics_->ReleaseHDC( cDc ); } );
    smp::gdi::ObjectSelector autoFont( cDc, gdiFontData.pFont->m_hFont );

    if ( rectOpt )
    {
        const auto stringFormat = DT_CALCRECT | GenerateStringFormat_GdiEx_FillTextEx( options );

        auto uRet = cDc.SetTextAlign( TA_LEFT | TA_TOP | TA_NOUPDATECP );
        qwr::error::CheckWinApi( uRet != GDI_ERROR, "SetTextAlign" );

        auto bounds = *rectOpt;
        auto iRet = cDc.DrawText( const_cast<wchar_t*>( cleanText.c_str() ), cleanText.size(), bounds, stringFormat );
        qwr::error::CheckWinApi( iRet, "DrawText" );

        TextMetrics::MetricsData data;
        data.width = std::min( bounds.Width(), rectOpt->Width() );
        data.height = std::min( bounds.Height(), rectOpt->Height() );
        return data;
    }
    else
    {
        const auto stringFormat = GenerateStringFormat_Gdi_FillTextEx( options );

        auto uRet = cDc.SetTextAlign( stringFormat );
        qwr::error::CheckWinApi( uRet != GDI_ERROR, "SetTextAlign" );

        SIZE bounds{};
        auto iRet = cDc.GetTextExtent( cleanText.c_str(), cleanText.size(), &bounds );
        qwr::error::CheckWinApi( iRet, "TextOutW" );

        TextMetrics::MetricsData data;
        data.width = bounds.cx;
        data.height = bounds.cy;
        return data;
    }
}

bool CanvasRenderingContext2D_Qwr::IsRect_FillTextEx( const FillTextExOptions& options )
{
    return options.width && options.height;
}

bool CanvasRenderingContext2D_Qwr::IsSingleLine_FillTextEx( const FillTextExOptions& options )
{
    return ( !IsRect_FillTextEx( options ) && !options.shouldWrapText && options.shouldCollapseNewLines && options.shouldCollapseSpaces );
}

bool CanvasRenderingContext2D_Qwr::IsSingleLineRect_FillTextEx( const FillTextExOptions& options )
{
    return ( IsRect_FillTextEx( options ) && !options.shouldWrapText && options.shouldCollapseNewLines && options.shouldCollapseSpaces );
}

} // namespace mozjs
