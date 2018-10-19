#include <stdafx.h>

#include "gdi_graphics.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_font.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <js_objects/measure_string_info.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/scope_helper.h>

#include <helpers.h>

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
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( CalcTextHeight, JsGdiGraphics::CalcTextHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( CalcTextWidth, JsGdiGraphics::CalcTextWidth )
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

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "CalcTextHeight", CalcTextHeight, 2, DefaultPropsFlags() ),
    JS_FN( "CalcTextWidth", CalcTextWidth, 2, DefaultPropsFlags() ),
    JS_FN( "DrawEllipse", DrawEllipse, 6, DefaultPropsFlags() ),
    JS_FN( "DrawImage", DrawImage, 9, DefaultPropsFlags() ),
    JS_FN( "DrawLine", DrawLine, 6, DefaultPropsFlags() ),
    JS_FN( "DrawPolygon", DrawPolygon, 3, DefaultPropsFlags() ),
    JS_FN( "DrawRect", DrawRect, 6, DefaultPropsFlags() ),
    JS_FN( "DrawRoundRect", DrawRoundRect, 8, DefaultPropsFlags() ),
    JS_FN( "DrawString", DrawString, 7, DefaultPropsFlags() ),
    JS_FN( "EstimateLineWrap", EstimateLineWrap, 3, DefaultPropsFlags() ),
    JS_FN( "FillEllipse", FillEllipse, 5, DefaultPropsFlags() ),
    JS_FN( "FillGradRect", FillGradRect, 7, DefaultPropsFlags() ),
    JS_FN( "FillPolygon", FillPolygon, 3, DefaultPropsFlags() ),
    JS_FN( "FillRoundRect", FillRoundRect, 7, DefaultPropsFlags() ),
    JS_FN( "FillSolidRect", FillSolidRect, 5, DefaultPropsFlags() ),
    JS_FN( "GdiAlphaBlend", GdiAlphaBlend, 9, DefaultPropsFlags() ),
    JS_FN( "GdiDrawBitmap", GdiDrawBitmap, 9, DefaultPropsFlags() ),
    JS_FN( "GdiDrawText", GdiDrawText, 7, DefaultPropsFlags() ),
    JS_FN( "MeasureString", MeasureString, 6, DefaultPropsFlags() ),
    JS_FN( "SetInterpolationMode", SetInterpolationMode, 0, DefaultPropsFlags() ),
    JS_FN( "SetSmoothingMode", SetSmoothingMode, 0, DefaultPropsFlags() ),
    JS_FN( "SetTextRenderingHint", SetTextRenderingHint, 0, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsGdiGraphics::JsClass = jsClass;
const JSFunctionSpec* JsGdiGraphics::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiGraphics::JsProperties = jsProperties;
const JsPrototypeId JsGdiGraphics::PrototypeId = JsPrototypeId::GdiGraphics;

JsGdiGraphics::JsGdiGraphics( JSContext* cx )
    : pJsCtx_( cx )
    , pGdi_( nullptr )
{
}

JsGdiGraphics::~JsGdiGraphics()
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
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !font )
    {
        throw smp::SmpException( "font argument is null" );
    }

    HFONT hFont = font->GetHFont();
    HDC dc = pGdi_->GetHDC();
    HFONT oldfont = SelectFont( dc, hFont );

    uint32_t textH = helpers::get_text_height( dc, str );

    SelectFont( dc, oldfont );
    pGdi_->ReleaseHDC( dc );

    return textH;
}

uint32_t JsGdiGraphics::CalcTextWidth( const std::wstring& str, JsGdiFont* font )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !font )
    {
        throw smp::SmpException( "font argument is null" );
    }

    HFONT hFont = font->GetHFont();
    HDC dc = pGdi_->GetHDC();
    assert( dc );
    HFONT oldfont = SelectFont( dc, hFont );

    uint32_t textW = helpers::get_text_width( dc, str );

    SelectFont( dc, oldfont );
    pGdi_->ReleaseHDC( dc );

    return textW;
}

void JsGdiGraphics::DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawEllipse( &pen, x, y, w, h );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawEllipse" );
}

void JsGdiGraphics::DrawImage( JsGdiBitmap* image,
                               float dstX, float dstY, float dstW, float dstH,
                               float srcX, float srcY, float srcW, float srcH,
                               float angle, uint8_t alpha )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !image )
    {
        throw smp::SmpException( "image argument is null" );
    }

    Gdiplus::Bitmap* img = image->GdiBitmap();
    assert( img );
    Gdiplus::Matrix oldMatrix;

    Gdiplus::Status gdiRet;
    if ( angle != 0.0 )
    {
        Gdiplus::Matrix m;
        Gdiplus::RectF rect;
        Gdiplus::PointF pt;

        pt.X = dstX + dstW / 2;
        pt.Y = dstY + dstH / 2;

        gdiRet = m.RotateAt( angle, pt );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "RotateAt" );

        gdiRet = pGdi_->GetTransform( &oldMatrix );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "GetTransform" );

        gdiRet = pGdi_->SetTransform( &m );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetTransform" );
    }

    if ( alpha < 255 )
    {
        Gdiplus::ImageAttributes ia;
        Gdiplus::ColorMatrix cm = { 0.0f };

        cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
        cm.m[3][3] = static_cast<float>( alpha ) / 255;

        gdiRet = ia.SetColorMatrix( &cm );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetColorMatrix" );

        gdiRet = pGdi_->DrawImage( img, Gdiplus::RectF( dstX, dstY, dstW, dstH ), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, &ia );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawImage" );
    }
    else
    {
        gdiRet = pGdi_->DrawImage( img, Gdiplus::RectF( dstX, dstY, dstW, dstH ), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawImage" );
    }

    if ( angle != 0.0 )
    {
        gdiRet = pGdi_->SetTransform( &oldMatrix );
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetTransform" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawLine( &pen, x1, y1, x2, y2 );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawLine" );
}

void JsGdiGraphics::DrawPolygon( uint32_t colour, float line_width, JS::HandleValue points )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawPolygon( &pen, gdiPoints.data(), gdiPoints.size() );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawPolygon" );
}

void JsGdiGraphics::DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = pGdi_->DrawRectangle( &pen, x, y, w, h );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawRectangle" );
}

void JsGdiGraphics::DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( 2 * arc_width > w || 2 * arc_height > h )
    {
        throw smp::SmpException( "Arc argument has invalid value" );
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::GraphicsPath gp;
    Gdiplus::RectF rect( x, y, w, h );
    GetRoundRectPath( gp, rect, arc_width, arc_height );

    Gdiplus::Status gdiRet = pen.SetStartCap( Gdiplus::LineCapRound );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "SetStartCap" );

    gdiRet = pen.SetEndCap( Gdiplus::LineCapRound );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "SetEndCap" );

    gdiRet = pGdi_->DrawPath( &pen, &gp );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawPath" );
}

void JsGdiGraphics::DrawString( const std::wstring& str, JsGdiFont* font, uint32_t colour, float x, float y, float w, float h, uint32_t flags )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !font )
    {
        throw smp::SmpException( "font argument is null" );
    }

    Gdiplus::Font* pGdiFont = font->GdiFont();
    if ( !pGdiFont )
    {
        throw smp::SmpException( "Internal error: GdiFont is null" );
    }

    Gdiplus::SolidBrush br( colour );
    Gdiplus::StringFormat fmt( Gdiplus::StringFormat::GenericTypographic() );

    if ( flags != 0 )
    {
        Gdiplus::Status gdiRet = fmt.SetAlignment( ( Gdiplus::StringAlignment )( ( flags >> 28 ) & 0x3 ) ); //0xf0000000
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetAlignment" );

        gdiRet = fmt.SetLineAlignment( ( Gdiplus::StringAlignment )( ( flags >> 24 ) & 0x3 ) ); //0x0f000000
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetLineAlignment" );

        gdiRet = fmt.SetTrimming( ( Gdiplus::StringTrimming )( ( flags >> 20 ) & 0x7 ) ); //0x00f00000
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetTrimming" );

        gdiRet = fmt.SetFormatFlags( ( Gdiplus::StringFormatFlags )( flags & 0x7FFF ) ); //0x0000ffff
        IF_GDI_FAILED_THROW_SMP( gdiRet, "SetFormatFlags" );
    }

    Gdiplus::Status gdiRet = pGdi_->DrawString( str.c_str(), -1, pGdiFont, Gdiplus::RectF( x, y, w, h ), &fmt, &br );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "DrawString" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

JSObject* JsGdiGraphics::EstimateLineWrap( const std::wstring& str, JsGdiFont* font, uint32_t max_width )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !font )
    {
        throw smp::SmpException( "font argument is null" );
    }

    std::list<helpers::wrapped_item> result;
    {
        HFONT hFont = font->GetHFont();
        assert( hFont );

        HDC dc = pGdi_->GetHDC();
        HFONT oldfont = SelectFont( dc, hFont );

        estimate_line_wrap( dc, str, max_width, result );

        SelectFont( dc, oldfont );
        pGdi_->ReleaseHDC( dc );
    }

    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, result.size() * 2 ) );
    if ( !jsArray )
    {
        throw smp::JsException();
    }

    JS::RootedValue jsValue( pJsCtx_ );
    size_t i = 0;
    for ( auto& [text, width] : result )
    {
        std::wstring tmpString( (const wchar_t*)text );
        convert::to_js::ToValue( pJsCtx_, tmpString, &jsValue );

        if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
        {
            throw smp::JsException();
        }

        jsValue.setNumber( (uint32_t)width );
        if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
        {
            throw smp::JsException();
        }
    }

    return jsArray;
}

void JsGdiGraphics::FillEllipse( float x, float y, float w, float h, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::SolidBrush br( colour );
    Gdiplus::Status gdiRet = pGdi_->FillEllipse( &br, x, y, w, h );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "FillEllipse" );
}

void JsGdiGraphics::FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::RectF rect( x, y, w, h );
    Gdiplus::LinearGradientBrush brush( rect, colour1, colour2, angle, TRUE );
    Gdiplus::Status gdiRet = brush.SetBlendTriangularShape( focus );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "SetBlendTriangularShape" );

    gdiRet = pGdi_->FillRectangle( &brush, rect );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "FillRectangle" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::FillPolygon( uint32_t colour, uint32_t fillmode, JS::HandleValue points )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::Status gdiRet = pGdi_->FillPolygon( &br, gdiPoints.data(), gdiPoints.size(), (Gdiplus::FillMode)fillmode );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "FillPolygon" );
}

void JsGdiGraphics::FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( 2 * arc_width > w || 2 * arc_height > h )
    {
        throw smp::SmpException( "Arc argument has invalid value" );
    }

    Gdiplus::SolidBrush br( colour );
    Gdiplus::GraphicsPath gp;
    Gdiplus::RectF rect( x, y, w, h );
    GetRoundRectPath( gp, rect, arc_width, arc_height );

    Gdiplus::Status gdiRet = pGdi_->FillPath( &br, &gp );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "FillPath" );
}

void JsGdiGraphics::FillSolidRect( float x, float y, float w, float h, uint32_t colour )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::SolidBrush brush( colour );
    Gdiplus::Status gdiRet = pGdi_->FillRectangle( &brush, x, y, w, h );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "FillRectangle" );
}

void JsGdiGraphics::GdiAlphaBlend( JsGdiRawBitmap* bitmap,
                                   int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                   int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                                   uint8_t alpha )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !bitmap )
    {
        throw smp::SmpException( "bitmap argument is null" );
    }

    HDC srcDc = bitmap->GetHDC();
    assert( srcDc );

    HDC dc = pGdi_->GetHDC();
    scope::final_action autoHdcReleaser( [pGdi = pGdi_, dc]() {
        pGdi->ReleaseHDC( dc );
    } );

    BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };

    BOOL bRet = ::GdiAlphaBlend( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, bf );
    IF_WINAPI_FAILED_THROW_SMP( bRet, "GdiAlphaBlend" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::GdiDrawBitmap( JsGdiRawBitmap* bitmap,
                                   int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                   int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !bitmap )
    {
        throw smp::SmpException( "bitmap argument is null" );
    }

    HDC srcDc = bitmap->GetHDC();
    assert( srcDc );

    HDC dc = pGdi_->GetHDC();
    scope::final_action autoHdcReleaser( [pGdi = pGdi_, dc]() {
        pGdi->ReleaseHDC( dc );
    } );

    BOOL bRet;
    if ( dstW == srcW && dstH == srcH )
    {
        bRet = BitBlt( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, SRCCOPY );
        IF_WINAPI_FAILED_THROW_SMP( bRet, "BitBlt" );
    }
    else
    {
        bRet = SetStretchBltMode( dc, HALFTONE );
        IF_WINAPI_FAILED_THROW_SMP( bRet, "SetStretchBltMode" );

        bRet = SetBrushOrgEx( dc, 0, 0, nullptr );
        IF_WINAPI_FAILED_THROW_SMP( bRet, "SetBrushOrgEx" );

        bRet = StretchBlt( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, SRCCOPY );
        IF_WINAPI_FAILED_THROW_SMP( bRet, "StretchBlt" );
    }
}

void JsGdiGraphics::GdiDrawText( const std::wstring& str, JsGdiFont* font, uint32_t colour, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t format )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !font )
    {
        throw smp::SmpException( "font argument is null" );
    }

    HFONT hFont = font->GetHFont();
    assert( hFont );

    HDC dc = pGdi_->GetHDC();
    HFONT oldfont = SelectFont( dc, hFont );
    scope::final_action autoHdcReleaser( [pGdi = pGdi_, dc, oldfont]() {
        SelectFont( dc, oldfont );
        pGdi->ReleaseHDC( dc );
    } );

    RECT rc = { x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };
    DRAWTEXTPARAMS dpt = { sizeof( DRAWTEXTPARAMS ), 4, 0, 0, 0 };

    SetTextColor( dc, helpers::convert_argb_to_colorref( colour ) );

    int iRet = SetBkMode( dc, TRANSPARENT );
    IF_WINAPI_FAILED_THROW_SMP( CLR_INVALID != iRet, "SetBkMode" );

    UINT uRet = SetTextAlign( dc, TA_LEFT | TA_TOP | TA_NOUPDATECP );
    IF_WINAPI_FAILED_THROW_SMP( GDI_ERROR != uRet, "SetTextAlign" );

    if ( format & DT_MODIFYSTRING )
    {
        format &= ~DT_MODIFYSTRING;
    }

    // Well, magic :P
    if ( format & DT_CALCRECT )
    {
        RECT rc_calc = { 0 }, rc_old = { 0 };

        memcpy( &rc_calc, &rc, sizeof( RECT ) );
        memcpy( &rc_old, &rc, sizeof( RECT ) );

        iRet = DrawText( dc, str.c_str(), -1, &rc_calc, format );
        IF_WINAPI_FAILED_THROW_SMP( iRet, "DrawText" );

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

    iRet = DrawTextEx( dc, const_cast<wchar_t*>( str.c_str() ), -1, &rc, format, &dpt );
    IF_WINAPI_FAILED_THROW_SMP( iRet, "DrawTextEx" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

JSObject* JsGdiGraphics::MeasureString( const std::wstring& str, JsGdiFont* font, float x, float y, float w, float h, uint32_t flags )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    if ( !font )
    {
        throw smp::SmpException( "font argument is null" );
    }

    Gdiplus::Font* fn = font->GdiFont();
    assert( fn );

    Gdiplus::StringFormat fmt = Gdiplus::StringFormat::GenericTypographic();

    if ( flags != 0 )
    {
        fmt.SetAlignment( ( Gdiplus::StringAlignment )( ( flags >> 28 ) & 0x3 ) );     //0xf0000000
        fmt.SetLineAlignment( ( Gdiplus::StringAlignment )( ( flags >> 24 ) & 0x3 ) ); //0x0f000000
        fmt.SetTrimming( ( Gdiplus::StringTrimming )( ( flags >> 20 ) & 0x7 ) );       //0x00f00000
        fmt.SetFormatFlags( ( Gdiplus::StringFormatFlags )( flags & 0x7FFF ) );        //0x0000ffff
    }

    Gdiplus::RectF bound;
    int chars, lines;

    Gdiplus::Status gdiRet = pGdi_->MeasureString( str.c_str(), -1, fn, Gdiplus::RectF( x, y, w, h ), &fmt, &bound, &chars, &lines );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "MeasureString" );

    JS::RootedObject jsObject( pJsCtx_, JsMeasureStringInfo::CreateJs( pJsCtx_, bound.X, bound.Y, bound.Width, bound.Height, lines, chars ) );
    assert( jsObject );

    return jsObject;
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::SetInterpolationMode( uint32_t mode )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::Status gdiRet = pGdi_->SetInterpolationMode( (Gdiplus::InterpolationMode)mode );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "SetInterpolationMode" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::SetSmoothingMode( uint32_t mode )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::Status gdiRet = pGdi_->SetSmoothingMode( (Gdiplus::SmoothingMode)mode );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "SetSmoothingMode" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::SetTextRenderingHint( uint32_t mode )
{
    if ( !pGdi_ )
    {
        throw smp::SmpException( "Internal error: Gdiplus::Graphics object is null" );
    }

    Gdiplus::Status gdiRet = pGdi_->SetTextRenderingHint( (Gdiplus::TextRenderingHint)mode );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "SetTextRenderingHint" );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiGraphics::GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height )
{
    float arc_dia_w = arc_width * 2;
    float arc_dia_h = arc_height * 2;
    Gdiplus::RectF corner( rect.X, rect.Y, arc_dia_w, arc_dia_h );

    Gdiplus::Status gdiRet = gp.Reset();
    IF_GDI_FAILED_THROW_SMP( gdiRet, "Reset" );

    // top left
    gdiRet = gp.AddArc( corner, 180, 90 );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "AddArc" );

    // top right
    corner.X += ( rect.Width - arc_dia_w );
    gdiRet = gp.AddArc( corner, 270, 90 );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "AddArc" );

    // bottom right
    corner.Y += ( rect.Height - arc_dia_h );
    gdiRet = gp.AddArc( corner, 0, 90 );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "AddArc" );

    // bottom left
    corner.X -= ( rect.Width - arc_dia_w );
    gdiRet = gp.AddArc( corner, 90, 90 );
    IF_GDI_FAILED_THROW_SMP( gdiRet, "AddArc" );

    gdiRet = gp.CloseFigure();
    IF_GDI_FAILED_THROW_SMP( gdiRet, "CloseFigure" );
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
            gdiPoints.emplace_back( Gdiplus::PointF( x, coordinate ) );
        }

        isX = !isX;
    };

    gdiPoints.clear();
    convert::to_native::ProcessArray<float>( pJsCtx_, jsValue, pointParser );

    if ( !isX )
    {// Means that we were expecting `y` coordinate
        throw smp::SmpException( "Points count must be a multiple of two" );
    }
}

} // namespace mozjs
