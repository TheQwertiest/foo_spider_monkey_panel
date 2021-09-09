#include <stdafx.h>

#include "gdi_graphics.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/gdi_font.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <js_objects/measure_string_info.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/colour_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/text_helpers.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

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
    JsGdiGraphics::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiGraphics",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( CalcTextHeight, JsGdiGraphics::CalcTextHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CalcTextWidth, JsGdiGraphics::CalcTextWidth, JsGdiGraphics::CalcTextWidthWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawEllipse, JsGdiGraphics::DrawEllipse )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawImage, JsGdiGraphics::DrawImage, JsGdiGraphics::DrawImageWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawLine, JsGdiGraphics::DrawLine )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawPolygon, JsGdiGraphics::DrawPolygon )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawRect, JsGdiGraphics::DrawRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawRoundRect, JsGdiGraphics::DrawRoundRect )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawString, JsGdiGraphics::DrawString, JsGdiGraphics::DrawStringWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( EstimateLineWrap, JsGdiGraphics::EstimateLineWrap )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillEllipse, JsGdiGraphics::FillEllipse )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( FillGradRect, JsGdiGraphics::FillGradRect, JsGdiGraphics::FillGradRectWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillPolygon, JsGdiGraphics::FillPolygon )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillRoundRect, JsGdiGraphics::FillRoundRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillSolidRect, JsGdiGraphics::FillSolidRect )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlendWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GdiDrawBitmap, JsGdiGraphics::GdiDrawBitmap )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiDrawText, JsGdiGraphics::GdiDrawText, JsGdiGraphics::GdiDrawTextWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( MeasureString, JsGdiGraphics::MeasureString, JsGdiGraphics::MeasureStringWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetInterpolationMode, JsGdiGraphics::SetInterpolationMode, JsGdiGraphics::SetInterpolationModeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetSmoothingMode, JsGdiGraphics::SetSmoothingMode, JsGdiGraphics::SetSmoothingModeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetTextRenderingHint, JsGdiGraphics::SetTextRenderingHint, JsGdiGraphics::SetTextRenderingHintWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "CalcTextHeight", CalcTextHeight, 2, kDefaultPropsFlags ),
        JS_FN( "CalcTextWidth", CalcTextWidth, 2, kDefaultPropsFlags ),
        JS_FN( "DrawEllipse", DrawEllipse, 6, kDefaultPropsFlags ),
        JS_FN( "DrawImage", DrawImage, 9, kDefaultPropsFlags ),
        JS_FN( "DrawLine", DrawLine, 6, kDefaultPropsFlags ),
        JS_FN( "DrawPolygon", DrawPolygon, 3, kDefaultPropsFlags ),
        JS_FN( "DrawRect", DrawRect, 6, kDefaultPropsFlags ),
        JS_FN( "DrawRoundRect", DrawRoundRect, 8, kDefaultPropsFlags ),
        JS_FN( "DrawString", DrawString, 7, kDefaultPropsFlags ),
        JS_FN( "EstimateLineWrap", EstimateLineWrap, 3, kDefaultPropsFlags ),
        JS_FN( "FillEllipse", FillEllipse, 5, kDefaultPropsFlags ),
        JS_FN( "FillGradRect", FillGradRect, 7, kDefaultPropsFlags ),
        JS_FN( "FillPolygon", FillPolygon, 3, kDefaultPropsFlags ),
        JS_FN( "FillRoundRect", FillRoundRect, 7, kDefaultPropsFlags ),
        JS_FN( "FillSolidRect", FillSolidRect, 5, kDefaultPropsFlags ),
        JS_FN( "GdiAlphaBlend", GdiAlphaBlend, 9, kDefaultPropsFlags ),
        JS_FN( "GdiDrawBitmap", GdiDrawBitmap, 9, kDefaultPropsFlags ),
        JS_FN( "GdiDrawText", GdiDrawText, 7, kDefaultPropsFlags ),
        JS_FN( "MeasureString", MeasureString, 6, kDefaultPropsFlags ),
        JS_FN( "SetInterpolationMode", SetInterpolationMode, 0, kDefaultPropsFlags ),
        JS_FN( "SetSmoothingMode", SetSmoothingMode, 0, kDefaultPropsFlags ),
        JS_FN( "SetTextRenderingHint", SetTextRenderingHint, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsGdiGraphics::JsClass = jsClass;
const JSFunctionSpec* JsGdiGraphics::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsGdiGraphics::JsProperties = jsProperties.data();
const JsPrototypeId JsGdiGraphics::PrototypeId = JsPrototypeId::GdiGraphics;

JsGdiGraphics::JsGdiGraphics( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<JsGdiGraphics>
JsGdiGraphics::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsGdiGraphics>( new JsGdiGraphics( cx ) );
}

size_t JsGdiGraphics::GetInternalSize()
{
    return 0;
}

Gdiplus::Graphics* JsGdiGraphics::GetGraphicsObject() const
{
    return pGdi_;
}

void JsGdiGraphics::SetGraphicsObject( Gdiplus::Graphics* graphics )
{
    pGdi_ = graphics;
}

uint32_t JsGdiGraphics::CalcTextHeight( const std::wstring& str, JsGdiFont* font )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const auto hDc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [hDc, pGdi = pGdi_] { pGdi->ReleaseHDC( hDc ); } );
    gdi::ObjectSelector autoFont( hDc, font->GetHFont() );

    return smp::utils::GetTextHeight( hDc, str );
}

uint32_t JsGdiGraphics::CalcTextWidth( const std::wstring& str, JsGdiFont* font, boolean use_exact )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const auto hDc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [hDc, pGdi = pGdi_] { pGdi->ReleaseHDC( hDc ); } );
    gdi::ObjectSelector autoFont( hDc, font->GetHFont() );

    return smp::utils::GetTextWidth( hDc, str, use_exact );
}

uint32_t JsGdiGraphics::CalcTextWidthWithOpt( size_t optArgCount, const std::wstring& str,
                                              JsGdiFont* font, boolean use_exact )
{
    switch ( optArgCount )
    {
    case 0:
        return CalcTextWidth( str, font, use_exact );
    case 1:
        return CalcTextWidth( str, font );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawEllipse( &pen, x, y, w, h );
    qwr::error::CheckGdi( gdiRet, "DrawEllipse" );
}

void JsGdiGraphics::DrawImage( JsGdiBitmap* image,
                               float dstX, float dstY, float dstW, float dstH,
                               float srcX, float srcY, float srcW, float srcH,
                               float angle, uint8_t alpha )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( image, "image argument is null" );

    Gdiplus::Bitmap* img = image->GdiBitmap();
    assert( img );
    Gdiplus::Matrix oldMatrix;

    Gdiplus::Status gdiRet;
    if ( angle != 0.0 )
    {
        Gdiplus::Matrix m;
        gdiRet = m.RotateAt( angle, Gdiplus::PointF{ dstX + dstW / 2, dstY + dstH / 2 } );
        qwr::error::CheckGdi( gdiRet, "RotateAt" );

        gdiRet = pGdi_->GetTransform( &oldMatrix );
        qwr::error::CheckGdi( gdiRet, "GetTransform" );

        gdiRet = pGdi_->SetTransform( &m );
        qwr::error::CheckGdi( gdiRet, "SetTransform" );
    }

    if ( alpha < 255 )
    {
        Gdiplus::ImageAttributes ia;
        Gdiplus::ColorMatrix cm{};

        cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
        cm.m[3][3] = static_cast<float>( alpha ) / 255;

        gdiRet = ia.SetColorMatrix( &cm );
        qwr::error::CheckGdi( gdiRet, "SetColorMatrix" );

        gdiRet = pGdi_->DrawImage( img, Gdiplus::RectF( dstX, dstY, dstW, dstH ), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, &ia );
        qwr::error::CheckGdi( gdiRet, "DrawImage" );
    }
    else
    {
        gdiRet = pGdi_->DrawImage( img, Gdiplus::RectF( dstX, dstY, dstW, dstH ), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel );
        qwr::error::CheckGdi( gdiRet, "DrawImage" );
    }

    if ( angle != 0.0 )
    {
        gdiRet = pGdi_->SetTransform( &oldMatrix );
        qwr::error::CheckGdi( gdiRet, "SetTransform" );
    }
}

void JsGdiGraphics::DrawImageWithOpt( size_t optArgCount, JsGdiBitmap* image,
                                      float dstX, float dstY, float dstW, float dstH,
                                      float srcX, float srcY, float srcW, float srcH, float angle,
                                      uint8_t alpha )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawImage( image, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, angle, alpha );
    case 1:
        return DrawImage( image, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, angle );
    case 2:
        return DrawImage( image, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawLine( &pen, x1, y1, x2, y2 );
    qwr::error::CheckGdi( gdiRet, "DrawLine" );
}

void JsGdiGraphics::DrawPolygon( uint32_t colour, float line_width, JS::HandleValue points )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawPolygon( &pen, gdiPoints.data(), gdiPoints.size() );
    qwr::error::CheckGdi( gdiRet, "DrawPolygon" );
}

void JsGdiGraphics::DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawRectangle( &pen, x, y, w, h );
    qwr::error::CheckGdi( gdiRet, "DrawRectangle" );
}

void JsGdiGraphics::DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( 2 * arc_width <= w && 2 * arc_height <= h, "Arc argument has invalid value" );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::GraphicsPath gp;
    GetRoundRectPath( gp, Gdiplus::RectF{ x, y, w, h }, arc_width, arc_height );

    Gdiplus::Status gdiRet = pen.SetStartCap( Gdiplus::LineCapRound );
    qwr::error::CheckGdi( gdiRet, "SetStartCap" );

    gdiRet = pen.SetEndCap( Gdiplus::LineCapRound );
    qwr::error::CheckGdi( gdiRet, "SetEndCap" );

    gdiRet = pGdi_->DrawPath( &pen, &gp );
    qwr::error::CheckGdi( gdiRet, "DrawPath" );
}

void JsGdiGraphics::DrawString( const std::wstring& str, JsGdiFont* font, uint32_t colour, float x, float y, float w, float h, uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    Gdiplus::Font* pGdiFont = font->GdiFont();
    qwr::QwrException::ExpectTrue( pGdiFont, "Internal error: GdiFont is null" );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::StringFormat fmt( Gdiplus::StringFormat::GenericTypographic() );

    if ( flags != 0 )
    {
        Gdiplus::Status gdiRet = fmt.SetAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 28 ) & 0x3 ) ); //0xf0000000
        qwr::error::CheckGdi( gdiRet, "SetAlignment" );

        gdiRet = fmt.SetLineAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 24 ) & 0x3 ) ); //0x0f000000
        qwr::error::CheckGdi( gdiRet, "SetLineAlignment" );

        gdiRet = fmt.SetTrimming( static_cast<Gdiplus::StringTrimming>( ( flags >> 20 ) & 0x7 ) ); //0x00f00000
        qwr::error::CheckGdi( gdiRet, "SetTrimming" );

        gdiRet = fmt.SetFormatFlags( static_cast<Gdiplus::StringAlignment>( flags & 0x7FFF ) ); //0x0000ffff
        qwr::error::CheckGdi( gdiRet, "SetFormatFlags" );
    }

    Gdiplus::Status gdiRet = pGdi_->DrawString( str.c_str(), -1, pGdiFont, Gdiplus::RectF( x, y, w, h ), &fmt, &br );
    qwr::error::CheckGdi( gdiRet, "DrawString" );
}

void JsGdiGraphics::DrawStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
                                       float x, float y, float w, float h,
                                       uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawString( str, font, colour, x, y, w, h, flags );
    case 1:
        return DrawString( str, font, colour, x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* JsGdiGraphics::EstimateLineWrap( const std::wstring& str, JsGdiFont* font, uint32_t max_width )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    std::vector<smp::utils::WrappedTextLine> result;
    {
        const auto hDc = pGdi_->GetHDC();
        qwr::final_action autoHdcReleaser( [hDc, pGdi = pGdi_] { pGdi->ReleaseHDC( hDc ); } );
        gdi::ObjectSelector autoFont( hDc, font->GetHFont() );

        result = smp::utils::WrapText( hDc, str, max_width );
    }

    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, result.size() * 2 ) );
    JsException::ExpectTrue( jsArray );

    JS::RootedValue jsValue( pJsCtx_ );
    size_t i = 0;
    for ( const auto& [text, width]: result )
    {
        convert::to_js::ToValue( pJsCtx_, text, &jsValue );

        if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
        {
            throw JsException();
        }

        jsValue.setNumber( (uint32_t)width );
        if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
        {
            throw JsException();
        }
    }

    return jsArray;
}

void JsGdiGraphics::FillEllipse( float x, float y, float w, float h, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::Status gdiRet = pGdi_->FillEllipse( &br, x, y, w, h );
    qwr::error::CheckGdi( gdiRet, "FillEllipse" );
}

void JsGdiGraphics::FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    const Gdiplus::RectF rect{ x, y, w, h };
    Gdiplus::LinearGradientBrush brush( rect, colour1, colour2, angle, TRUE );
    Gdiplus::Status gdiRet = brush.SetBlendTriangularShape( focus );
    qwr::error::CheckGdi( gdiRet, "SetBlendTriangularShape" );

    gdiRet = pGdi_->FillRectangle( &brush, rect );
    qwr::error::CheckGdi( gdiRet, "FillRectangle" );
}

void JsGdiGraphics::FillGradRectWithOpt( size_t optArgCount, float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus )
{
    switch ( optArgCount )
    {
    case 0:
        return FillGradRect( x, y, w, h, angle, colour1, colour2, focus );
    case 1:
        return FillGradRect( x, y, w, h, angle, colour1, colour2 );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::FillPolygon( uint32_t colour, uint32_t fillmode, JS::HandleValue points )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::Status gdiRet = pGdi_->FillPolygon( &br, gdiPoints.data(), gdiPoints.size(), static_cast<Gdiplus::FillMode>( fillmode ) );
    qwr::error::CheckGdi( gdiRet, "FillPolygon" );
}

void JsGdiGraphics::FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::QwrException::ExpectTrue( 2 * arc_width <= w && 2 * arc_height <= h, "Arc argument has invalid value" );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::GraphicsPath gp;
    const Gdiplus::RectF rect{ x, y, w, h };
    GetRoundRectPath( gp, rect, arc_width, arc_height );

    Gdiplus::Status gdiRet = pGdi_->FillPath( &br, &gp );
    qwr::error::CheckGdi( gdiRet, "FillPath" );
}

void JsGdiGraphics::FillSolidRect( float x, float y, float w, float h, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::SolidBrush brush( colour );
    Gdiplus::Status gdiRet = pGdi_->FillRectangle( &brush, x, y, w, h );
    qwr::error::CheckGdi( gdiRet, "FillRectangle" );
}

void JsGdiGraphics::GdiAlphaBlend( JsGdiRawBitmap* bitmap,
                                   int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                   int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                                   uint8_t alpha )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( bitmap, "bitmap argument is null" );

    const auto srcDc = bitmap->GetHDC();
    assert( srcDc );

    const auto hDc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [pGdi = pGdi_, hDc]() { pGdi->ReleaseHDC( hDc ); } );

    BOOL bRet = ::GdiAlphaBlend( hDc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, BLENDFUNCTION{ AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA } );
    qwr::error::CheckWinApi( bRet, "GdiAlphaBlend" );
}

void JsGdiGraphics::GdiAlphaBlendWithOpt( size_t optArgCount, JsGdiRawBitmap* bitmap,
                                          int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                          int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                                          uint8_t alpha )
{
    switch ( optArgCount )
    {
    case 0:
        return GdiAlphaBlend( bitmap, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, alpha );
    case 1:
        return GdiAlphaBlend( bitmap, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::GdiDrawBitmap( JsGdiRawBitmap* bitmap,
                                   int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                   int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( bitmap, "bitmap argument is null" );

    HDC srcDc = bitmap->GetHDC();
    assert( srcDc );

    HDC hDc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [pGdi = pGdi_, hDc]() { pGdi->ReleaseHDC( hDc ); } );

    BOOL bRet;
    if ( dstW == srcW && dstH == srcH )
    {
        bRet = BitBlt( hDc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, SRCCOPY );
        qwr::error::CheckWinApi( bRet, "BitBlt" );
    }
    else
    {
        bRet = SetStretchBltMode( hDc, HALFTONE );
        qwr::error::CheckWinApi( bRet, "SetStretchBltMode" );

        bRet = SetBrushOrgEx( hDc, 0, 0, nullptr );
        qwr::error::CheckWinApi( bRet, "SetBrushOrgEx" );

        bRet = StretchBlt( hDc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, SRCCOPY );
        qwr::error::CheckWinApi( bRet, "StretchBlt" );
    }
}

void JsGdiGraphics::GdiDrawText( const std::wstring& str, JsGdiFont* font, uint32_t colour, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t format )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const auto hDc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [pGdi = pGdi_, hDc] { pGdi->ReleaseHDC( hDc ); } );
    gdi::ObjectSelector autoFont( hDc, font->GetHFont() );

    RECT rc{ x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };
    DRAWTEXTPARAMS dpt = { sizeof( DRAWTEXTPARAMS ), 4, 0, 0, 0 };

    SetTextColor( hDc, smp::colour::ArgbToColorref( colour ) );

    int iRet = SetBkMode( hDc, TRANSPARENT );
    qwr::error::CheckWinApi( CLR_INVALID != iRet, "SetBkMode" );

    UINT uRet = SetTextAlign( hDc, TA_LEFT | TA_TOP | TA_NOUPDATECP );
    qwr::error::CheckWinApi( GDI_ERROR != uRet, "SetTextAlign" );

    if ( format & DT_MODIFYSTRING )
    {
        format &= ~DT_MODIFYSTRING;
    }

    // Well, magic :P
    if ( format & DT_CALCRECT )
    {
        const RECT rc_old = rc;

        RECT rc_calc = rc;
        iRet = DrawText( hDc, str.c_str(), -1, &rc_calc, format );
        qwr::error::CheckWinApi( iRet, "DrawText" );

        format &= ~DT_CALCRECT;

        // adjust vertical align
        if ( format & DT_VCENTER )
        {
            rc.top = rc_old.top + ( ( ( rc_old.bottom - rc_old.top ) - ( rc_calc.bottom - rc_calc.top ) ) >> 1 );
            rc.bottom = rc.top + ( rc_calc.bottom - rc_calc.top );
        }
        else if ( format & DT_BOTTOM )
        {
            rc.top = rc_old.bottom - ( rc_calc.bottom - rc_calc.top );
        }
    }

    iRet = DrawTextEx( hDc, const_cast<wchar_t*>( str.c_str() ), -1, &rc, format, &dpt );
    qwr::error::CheckWinApi( iRet, "DrawTextEx" );
}

void JsGdiGraphics::GdiDrawTextWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
                                        int32_t x, int32_t y, uint32_t w, uint32_t h,
                                        uint32_t format )
{
    switch ( optArgCount )
    {
    case 0:
        return GdiDrawText( str, font, colour, x, y, w, h, format );
    case 1:
        return GdiDrawText( str, font, colour, x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* JsGdiGraphics::MeasureString( const std::wstring& str, JsGdiFont* font, float x, float y, float w, float h, uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    Gdiplus::Font* fn = font->GdiFont();
    assert( fn );

    Gdiplus::StringFormat fmt = Gdiplus::StringFormat::GenericTypographic();
    if ( flags != 0 )
    {
        fmt.SetAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 28 ) & 0x3 ) );     //0xf0000000
        fmt.SetLineAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 24 ) & 0x3 ) ); //0x0f000000
        fmt.SetTrimming( static_cast<Gdiplus::StringTrimming>( ( flags >> 20 ) & 0x7 ) );       //0x00f00000
        fmt.SetFormatFlags( static_cast<Gdiplus::StringFormatFlags>( flags & 0x7FFF ) );        //0x0000ffff
    }

    Gdiplus::RectF bound;
    int chars;
    int lines;
    Gdiplus::Status gdiRet = pGdi_->MeasureString( str.c_str(), -1, fn, Gdiplus::RectF( x, y, w, h ), &fmt, &bound, &chars, &lines );
    qwr::error::CheckGdi( gdiRet, "MeasureString" );

    return JsMeasureStringInfo::CreateJs( pJsCtx_, bound.X, bound.Y, bound.Width, bound.Height, lines, chars );
}

JSObject* JsGdiGraphics::MeasureStringWithOpt( size_t optArgCount, const std::wstring& str, JsGdiFont* font,
                                               float x, float y, float w, float h,
                                               uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return MeasureString( str, font, x, y, w, h, flags );
    case 1:
        return MeasureString( str, font, x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetInterpolationMode( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Status gdiRet = pGdi_->SetInterpolationMode( static_cast<Gdiplus::InterpolationMode>( mode ) );
    qwr::error::CheckGdi( gdiRet, "SetInterpolationMode" );
}

void JsGdiGraphics::SetInterpolationModeWithOpt( size_t optArgCount, uint32_t mode )
{
    switch ( optArgCount )
    {
    case 0:
        return SetInterpolationMode( mode );
    case 1:
        return SetInterpolationMode();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetSmoothingMode( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Status gdiRet = pGdi_->SetSmoothingMode( static_cast<Gdiplus::SmoothingMode>( mode ) );
    qwr::error::CheckGdi( gdiRet, "SetSmoothingMode" );
}

void JsGdiGraphics::SetSmoothingModeWithOpt( size_t optArgCount, uint32_t mode )
{
    switch ( optArgCount )
    {
    case 0:
        return SetSmoothingMode( mode );
    case 1:
        return SetSmoothingMode();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetTextRenderingHint( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Status gdiRet = pGdi_->SetTextRenderingHint( static_cast<Gdiplus::TextRenderingHint>( mode ) );
    qwr::error::CheckGdi( gdiRet, "SetTextRenderingHint" );
}

void JsGdiGraphics::SetTextRenderingHintWithOpt( size_t optArgCount, uint32_t mode )
{
    switch ( optArgCount )
    {
    case 0:
        return SetTextRenderingHint( mode );
    case 1:
        return SetTextRenderingHint();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::GetRoundRectPath( Gdiplus::GraphicsPath& gp, const Gdiplus::RectF& rect, float arc_width, float arc_height ) const
{
    const float arc_dia_w = arc_width * 2;
    const float arc_dia_h = arc_height * 2;
    Gdiplus::RectF corner{ rect.X, rect.Y, arc_dia_w, arc_dia_h };

    Gdiplus::Status gdiRet = gp.Reset();
    qwr::error::CheckGdi( gdiRet, "Reset" );

    // top left
    gdiRet = gp.AddArc( corner, 180, 90 );
    qwr::error::CheckGdi( gdiRet, "AddArc" );

    // top right
    corner.X += ( rect.Width - arc_dia_w );
    gdiRet = gp.AddArc( corner, 270, 90 );
    qwr::error::CheckGdi( gdiRet, "AddArc" );

    // bottom right
    corner.Y += ( rect.Height - arc_dia_h );
    gdiRet = gp.AddArc( corner, 0, 90 );
    qwr::error::CheckGdi( gdiRet, "AddArc" );

    // bottom left
    corner.X -= ( rect.Width - arc_dia_w );
    gdiRet = gp.AddArc( corner, 90, 90 );
    qwr::error::CheckGdi( gdiRet, "AddArc" );

    gdiRet = gp.CloseFigure();
    qwr::error::CheckGdi( gdiRet, "CloseFigure" );
}

void JsGdiGraphics::ParsePoints( JS::HandleValue jsValue, std::vector<Gdiplus::PointF>& gdiPoints )
{
    bool isX = true;
    float x = 0.0;
    auto pointParser = [&gdiPoints, &isX, &x]( float coordinate ) {
        if ( isX )
        {
            x = coordinate;
        }
        else
        {
            gdiPoints.emplace_back( Gdiplus::PointF{ x, coordinate } );
        }

        isX = !isX;
    };

    gdiPoints.clear();
    convert::to_native::ProcessArray<float>( pJsCtx_, jsValue, pointParser );

    qwr::QwrException::ExpectTrue( isX, "Points count must be a multiple of two" ); ///< Means that we were expecting `y` coordinate
}

} // namespace mozjs
