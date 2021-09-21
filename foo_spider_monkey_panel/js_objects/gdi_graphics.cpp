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

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( BeginContainer, JsGdiGraphics::BeginContainer, JsGdiGraphics::BeginContainerWithOpt, 8 )
MJS_DEFINE_JS_FN_FROM_NATIVE( Clear, JsGdiGraphics::Clear )
MJS_DEFINE_JS_FN_FROM_NATIVE( CalcTextHeight, JsGdiGraphics::CalcTextHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CalcTextWidth, JsGdiGraphics::CalcTextWidth, JsGdiGraphics::CalcTextWidthWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawEllipse, JsGdiGraphics::DrawEllipse )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawImage, JsGdiGraphics::DrawImage, JsGdiGraphics::DrawImageWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawLine, JsGdiGraphics::DrawLine )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawString, JsGdiGraphics::DrawString, JsGdiGraphics::DrawStringWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawPolygon, JsGdiGraphics::DrawPolygon )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawRect, JsGdiGraphics::DrawRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawRoundRect, JsGdiGraphics::DrawRoundRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( EndContainer, JsGdiGraphics::EndContainer )
MJS_DEFINE_JS_FN_FROM_NATIVE( EstimateLineWrap, JsGdiGraphics::EstimateLineWrap )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillEllipse, JsGdiGraphics::FillEllipse )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( FillGradRect, JsGdiGraphics::FillGradRect, JsGdiGraphics::FillGradRectWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillPolygon, JsGdiGraphics::FillPolygon )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillRoundRect, JsGdiGraphics::FillRoundRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillSolidRect, JsGdiGraphics::FillSolidRect )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlendWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GdiDrawBitmap, JsGdiGraphics::GdiDrawBitmap )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiDrawText, JsGdiGraphics::GdiDrawText, JsGdiGraphics::GdiDrawTextWithOpt, 1 )

MJS_DEFINE_JS_FN_FROM_NATIVE( GetClip, JsGdiGraphics::GetClip )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetCompositingMode, JsGdiGraphics::GetCompositingMode )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetCompositingQuality, JsGdiGraphics::GetCompositingQuality )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetDpiX, JsGdiGraphics::GetDpiX )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetDpiY, JsGdiGraphics::GetDpiY )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetInterpolationMode, JsGdiGraphics::GetInterpolationMode )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPixelOffsetMode, JsGdiGraphics::GetPixelOffsetMode )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSmoothingMode, JsGdiGraphics::GetSmoothingMode )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetTextContrast, JsGdiGraphics::GetTextContrast )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetTextRenderingHint, JsGdiGraphics::GetTextRenderingHint )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetTransform, JsGdiGraphics::GetTransform )

MJS_DEFINE_JS_FN_FROM_NATIVE( IsClipEmpty, JsGdiGraphics::IsClipEmpty )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsPointVisible, JsGdiGraphics::IsPointVisible )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsRectVisible, JsGdiGraphics::IsRectVisible )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsVisibleClipEmpty, JsGdiGraphics::IsVisibleClipEmpty )

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( MeasureString, JsGdiGraphics::MeasureString, JsGdiGraphics::MeasureStringWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( MultiplyTransform, JsGdiGraphics::MultiplyTransform, JsGdiGraphics::MultiplyTransformWithOpt, 1 )

MJS_DEFINE_JS_FN_FROM_NATIVE( Save, JsGdiGraphics::Save )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ScaleTransform, JsGdiGraphics::ScaleTransform, JsGdiGraphics::ScaleTransformWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( ResetClip, JsGdiGraphics::ResetClip )
MJS_DEFINE_JS_FN_FROM_NATIVE( ResetTransform, JsGdiGraphics::ResetTransform )
MJS_DEFINE_JS_FN_FROM_NATIVE( Restore, JsGdiGraphics::Restore )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( RotateTransform, JsGdiGraphics::RotateTransform, JsGdiGraphics::RotateTransformWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetClip, JsGdiGraphics::SetClip )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetCompositingMode, JsGdiGraphics::SetCompositingMode, JsGdiGraphics::SetCompositingModeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetCompositingQuality, JsGdiGraphics::SetCompositingQuality, JsGdiGraphics::SetCompositingQualityWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetInterpolationMode, JsGdiGraphics::SetInterpolationMode, JsGdiGraphics::SetInterpolationModeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetPixelOffsetMode, JsGdiGraphics::SetPixelOffsetMode, JsGdiGraphics::SetPixelOffsetModeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetSmoothingMode, JsGdiGraphics::SetSmoothingMode, JsGdiGraphics::SetSmoothingModeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetTextContrast, JsGdiGraphics::SetTextContrast )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetTextRenderingHint, JsGdiGraphics::SetTextRenderingHint, JsGdiGraphics::SetTextRenderingHintWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetTransform, JsGdiGraphics::SetTransform )

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( TranslateTransform, JsGdiGraphics::TranslateTransform, JsGdiGraphics::TranslateTransformWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( TransformPoint, JsGdiGraphics::TransformPoint )
MJS_DEFINE_JS_FN_FROM_NATIVE( TransformRect, JsGdiGraphics::TransformRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( UnTransformPoint, JsGdiGraphics::UnTransformPoint )
MJS_DEFINE_JS_FN_FROM_NATIVE( UnTransformRect, JsGdiGraphics::UnTransformRect )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "BeginContainer", BeginContainer, 0, kDefaultPropsFlags ),
        JS_FN( "CalcTextHeight", CalcTextHeight, 2, kDefaultPropsFlags ),
        JS_FN( "CalcTextWidth", CalcTextWidth, 2, kDefaultPropsFlags ),
        JS_FN( "Clear", Clear, 1, kDefaultPropsFlags ),
        JS_FN( "DrawEllipse", DrawEllipse, 6, kDefaultPropsFlags ),
        JS_FN( "DrawLine", DrawLine, 6, kDefaultPropsFlags ),
        JS_FN( "DrawImage", DrawImage, 9, kDefaultPropsFlags ),
        JS_FN( "DrawRect", DrawRect, 6, kDefaultPropsFlags ),
        JS_FN( "DrawRoundRect", DrawRoundRect, 8, kDefaultPropsFlags ),
        JS_FN( "DrawPolygon", DrawPolygon, 3, kDefaultPropsFlags ),
        JS_FN( "DrawString", DrawString, 7, kDefaultPropsFlags ),
        JS_FN( "EndContainer", EndContainer, 1, kDefaultPropsFlags ),
        JS_FN( "EstimateLineWrap", EstimateLineWrap, 3, kDefaultPropsFlags ),
        JS_FN( "FillEllipse", FillEllipse, 5, kDefaultPropsFlags ),
        JS_FN( "FillGradRect", FillGradRect, 7, kDefaultPropsFlags ),
        JS_FN( "FillRoundRect", FillRoundRect, 7, kDefaultPropsFlags ),
        JS_FN( "FillPolygon", FillPolygon, 3, kDefaultPropsFlags ),
        JS_FN( "FillSolidRect", FillSolidRect, 5, kDefaultPropsFlags ),
        JS_FN( "GdiAlphaBlend", GdiAlphaBlend, 9, kDefaultPropsFlags ),
        JS_FN( "GdiDrawBitmap", GdiDrawBitmap, 9, kDefaultPropsFlags ),
        JS_FN( "GdiDrawText", GdiDrawText, 7, kDefaultPropsFlags ),
        JS_FN( "GetClip", GetClip, 0, kDefaultPropsFlags ),
        JS_FN( "GetDpiX", GetDpiX, 0, kDefaultPropsFlags ),
        JS_FN( "GetDpiY", GetDpiY, 0, kDefaultPropsFlags ),
        JS_FN( "GetCompositingMode", GetCompositingMode, 0, kDefaultPropsFlags ),
        JS_FN( "GetCompositingQuality", GetCompositingQuality, 0, kDefaultPropsFlags ),
        JS_FN( "GetInterpolationMode", GetInterpolationMode, 0, kDefaultPropsFlags ),
        JS_FN( "GetPixelOffsetMode", SetPixelOffsetMode, 0, kDefaultPropsFlags ),
        JS_FN( "GetSmoothingMode", GetSmoothingMode, 0, kDefaultPropsFlags ),
        JS_FN( "GetTextContrast", GetTextContrast, 0, kDefaultPropsFlags ),
        JS_FN( "GetTextRenderingHint", GetTextRenderingHint, 0, kDefaultPropsFlags ),
        JS_FN( "GetTransform", GetTransform, 0, kDefaultPropsFlags ),
        JS_FN( "IsClipEmpty", IsClipEmpty, 0, kDefaultPropsFlags ),
        JS_FN( "IsPointVisible", IsPointVisible, 2, kDefaultPropsFlags ),
        JS_FN( "IsRectVisible", IsRectVisible, 4, kDefaultPropsFlags ),
        JS_FN( "IsVisibleClipEmpty", IsVisibleClipEmpty, 0, kDefaultPropsFlags ),
        JS_FN( "MeasureString", MeasureString, 6, kDefaultPropsFlags ),
        JS_FN( "MultiplyTransform", MultiplyTransform, 6, kDefaultPropsFlags ),
        JS_FN( "ResetClip", ResetClip, 0, kDefaultPropsFlags ),
        JS_FN( "ResetTransform", ResetTransform, 0, kDefaultPropsFlags ),
        JS_FN( "Restore", Restore, 1, kDefaultPropsFlags ),
        JS_FN( "RotateTransform", RotateTransform, 1, kDefaultPropsFlags ),
        JS_FN( "Save", Save, 0, kDefaultPropsFlags ),
        JS_FN( "ScaleTransform", ScaleTransform, 2, kDefaultPropsFlags ),
        JS_FN( "SetClip", SetClip, 4, kDefaultPropsFlags ),
        JS_FN( "SetCompositingMode", SetCompositingMode, 0, kDefaultPropsFlags ),
        JS_FN( "SetCompositingQuality", SetCompositingQuality, 0, kDefaultPropsFlags ),
        JS_FN( "SetInterpolationMode", SetInterpolationMode, 0, kDefaultPropsFlags ),
        JS_FN( "SetSmoothingMode", SetSmoothingMode, 0, kDefaultPropsFlags ),
        JS_FN( "SetPixelOffsetMode", SetPixelOffsetMode, 0, kDefaultPropsFlags ),
        JS_FN( "SetTextContrast", SetTextContrast, 1, kDefaultPropsFlags ),
        JS_FN( "SetTextRenderingHint", SetTextRenderingHint, 0, kDefaultPropsFlags ),
        JS_FN( "SetTransform", SetTransform, 6, kDefaultPropsFlags ),
        JS_FN( "TransformPoint", TransformPoint, 2, kDefaultPropsFlags ),
        JS_FN( "TransformRect", TransformRect, 4, kDefaultPropsFlags ),
        JS_FN( "TranslateTransform", TranslateTransform, 2, kDefaultPropsFlags ),
        JS_FN( "UnTransformPoint", UnTransformPoint, 2, kDefaultPropsFlags ),
        JS_FN( "UnTransformRect", UnTransformRect, 4, kDefaultPropsFlags ),
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

void JsGdiGraphics::TransferGdiplusClipTransformFor( std::function<void( HDC dc )> const& function )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    // get current clip region and transform
    // we need to do this before getting the dc
    Gdiplus::Region region;
    qwr::error::CheckGdi( pGdi_->GetClip( &region ), "GetClip" );
    HRGN hrgn{ region.GetHRGN( pGdi_ ) };

    Gdiplus::Matrix matrix;
    qwr::error::CheckGdi( pGdi_->GetTransform( &matrix ), "GetTransform" );

    XFORM xform;
    qwr::error::CheckGdi( matrix.GetElements( (Gdiplus::REAL*)( &xform ) ), "GetElements" );

    // get the device context
    const HDC dc = pGdi_->GetHDC();
    qwr::final_action autoReleaseDc( [graphics = pGdi_, dc]() { graphics->ReleaseHDC( dc ); } );

    // set clip and transform
    HRGN oldhrgn = ::CreateRectRgn( 0, 0, 0, 0 ); // dummy region for getting the current clip region into
    qwr::error::CheckWinApi( -1 != ::GetClipRgn( dc, oldhrgn ), "GetClipRgn" );
    qwr::final_action autoReleaseRegion( [dc, oldhrgn]() { ::SelectClipRgn( dc, oldhrgn ); ::DeleteObject( oldhrgn ); } );

    qwr::error::CheckWinApi( ERROR != ::SelectClipRgn( dc, hrgn ), "SelectClipRgn" );
    ::DeleteObject( hrgn ); // see remarks at https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectcliprgn

    XFORM oldxform;
    qwr::error::CheckWinApi( ::GetWorldTransform( dc, &oldxform ), "GetWorldTransform" );
    qwr::error::CheckWinApi( ::SetWorldTransform( dc, &xform ), "SetWorldTransform" );
    qwr::final_action autoResetTansform( [dc, oldxform]() { ::SetWorldTransform( dc, &oldxform ); } );

    function( dc );
}

uint32_t JsGdiGraphics::BeginContainer( float dst_x, float dst_y, float dst_w, float dst_h, float src_x, float src_y, float src_w, float src_h )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::RectF dst_rect{ dst_x, dst_y, dst_w, dst_h };
    Gdiplus::RectF src_rect{ src_x, src_y, src_w, src_h };

    if ( !dst_rect.IsEmptyArea() && !src_rect.IsEmptyArea() )
    {
        return pGdi_->BeginContainer( dst_rect, src_rect, Gdiplus::UnitPixel );
    }
    else
    {
        return pGdi_->BeginContainer();
    }
}

uint32_t JsGdiGraphics::BeginContainerWithOpt( size_t optArgCount, float dst_x, float dst_y, float dst_w, float dst_h, float src_x, float src_y, float src_w, float src_h )
{
    switch ( optArgCount )
    {
    case 0:
        return BeginContainer( dst_x, dst_y, dst_w, dst_h, src_x, src_y, src_w, src_h );
    case 1:
        return BeginContainer( dst_x, dst_y, dst_w, dst_h, src_x, src_y, src_w );
    case 2:
        return BeginContainer( dst_x, dst_y, dst_w, dst_h, src_x, src_y );
    case 3:
        return BeginContainer( dst_x, dst_y, dst_w, dst_h, src_x );
    case 4:
        return BeginContainer( dst_x, dst_y, dst_w, dst_h );
    case 5:
        return BeginContainer( dst_x, dst_y, dst_w );
    case 6:
        return BeginContainer( dst_x, dst_y );
    case 7:
        return BeginContainer( dst_x );
    case 8:
        return BeginContainer();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}
uint32_t JsGdiGraphics::CalcTextHeight( const std::wstring& text, JsGdiFont* font )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const HDC dc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [dc, pGdi = pGdi_] { pGdi->ReleaseHDC( dc ); } );
    gdi::ObjectSelector autoFont( dc, font->GetHFont() );

    return smp::utils::GetTextHeight( dc, text );
}

uint32_t JsGdiGraphics::CalcTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const HDC dc = pGdi_->GetHDC();
    qwr::final_action autoHdcReleaser( [dc, pGdi = pGdi_] { pGdi->ReleaseHDC( dc ); } );
    gdi::ObjectSelector autoFont( dc, font->GetHFont() );

    return smp::utils::GetTextWidth( dc, text, use_exact );
}

uint32_t JsGdiGraphics::CalcTextWidthWithOpt( size_t optArgCount, const std::wstring& text,
                                              JsGdiFont* font, boolean use_exact )
{
    switch ( optArgCount )
    {
    case 0:
        return CalcTextWidth( text, font, use_exact );
    case 1:
        return CalcTextWidth( text, font );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::Clear( uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->Clear( colour ), "Clear" );
}

void JsGdiGraphics::DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, line_width );
    qwr::error::CheckGdi( pGdi_->DrawEllipse( &pen, x, y, w, h ), "DrawEllipse" );
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

    if ( angle != 0.0 )
    {
        Gdiplus::Matrix m;
        qwr::error::CheckGdi( m.RotateAt( angle, Gdiplus::PointF{ dstX + dstW / 2, dstY + dstH / 2 } ), "RotateAt" );
        qwr::error::CheckGdi( pGdi_->GetTransform( &oldMatrix ), "GetTransform" );
        qwr::error::CheckGdi( pGdi_->SetTransform( &m ), "SetTransform" );
    }

    Gdiplus::RectF dst{ dstX, dstY, dstW, dstH };
    Gdiplus::ImageAttributes* pia = nullptr;

    if ( alpha < 255 )
    {
        Gdiplus::ImageAttributes ia;
        Gdiplus::ColorMatrix cm{};

        cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
        cm.m[3][3] = static_cast<float>( alpha ) / 255;

        qwr::error::CheckGdi( ia.SetColorMatrix( &cm ), "SetColorMatrix" );
        pia = &ia;
    }

    qwr::error::CheckGdi( pGdi_->DrawImage( img, dst, srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, pia ), "DrawImage" );

    if ( angle != 0.0 )
    {
        qwr::error::CheckGdi( pGdi_->SetTransform( &oldMatrix ), "SetTransform" );
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
    qwr::error::CheckGdi( pGdi_->DrawLine( &pen, x1, y1, x2, y2 ), "DrawLine" );
}

void JsGdiGraphics::DrawPolygon( uint32_t colour, float line_width, JS::HandleValue points )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::Pen pen( colour, line_width );
    qwr::error::CheckGdi( pGdi_->DrawPolygon( &pen, gdiPoints.data(), gdiPoints.size() ), "DrawPolygon" );
}

void JsGdiGraphics::DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, line_width );
    qwr::error::CheckGdi( pGdi_->DrawRectangle( &pen, x, y, w, h ), "DrawRectangle" );
}

void JsGdiGraphics::DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( 2 * arc_width <= w && 2 * arc_height <= h, "Arc argument has invalid value" );

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::GraphicsPath path;
    GetRoundRectPath( path, Gdiplus::RectF{ x, y, w, h }, arc_width, arc_height );

    qwr::error::CheckGdi( pen.SetStartCap( Gdiplus::LineCapRound ), "SetStartCap" );
    qwr::error::CheckGdi( pen.SetEndCap( Gdiplus::LineCapRound ), "SetEndCap" );
    qwr::error::CheckGdi( pGdi_->DrawPath( &pen, &path ), "DrawPath" );
}

void JsGdiGraphics::DrawString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                float x, float y, float w, float h,
                                uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    Gdiplus::Font* pGdiFont = font->GdiFont();
    qwr::QwrException::ExpectTrue( pGdiFont, "Internal error: GdiFont is null" );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::StringFormat fmt( Gdiplus::StringFormat::GenericTypographic() );

    if ( flags != 0 )
    {
        qwr::error::CheckGdi( fmt.SetAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 28 ) & 0x3 ) ), "SetAlignment" );         //0xf0000000
        qwr::error::CheckGdi( fmt.SetLineAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 24 ) & 0x3 ) ), "SetLineAlignment" ); //0x0f000000
        qwr::error::CheckGdi( fmt.SetTrimming( static_cast<Gdiplus::StringTrimming>( ( flags >> 20 ) & 0x7 ) ), "SetTrimming" );            //0x00f00000
        qwr::error::CheckGdi( fmt.SetFormatFlags( static_cast<Gdiplus::StringAlignment>( flags & 0x7FFF ) ), "SetFormatFlags" );            //0x0000ffff
    }

    qwr::error::CheckGdi( pGdi_->DrawString( text.c_str(), -1, pGdiFont, Gdiplus::RectF( x, y, w, h ), &fmt, &br ), "DrawString" );
}

void JsGdiGraphics::DrawStringWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                       float x, float y, float w, float h,
                                       uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawString( text, font, colour, x, y, w, h, flags );
    case 1:
        return DrawString( text, font, colour, x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::EndContainer( uint32_t state )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->EndContainer( state ), "EndContainer" );
}

JSObject* JsGdiGraphics::EstimateLineWrap( const std::wstring& str, JsGdiFont* font, uint32_t max_width )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    std::vector<smp::utils::WrappedTextLine> result;
    {
        const HDC dc = pGdi_->GetHDC();
        qwr::final_action autoHdcReleaser( [dc, pGdi = pGdi_] { pGdi->ReleaseHDC( dc ); } );
        gdi::ObjectSelector autoFont( dc, font->GetHFont() );

        result = smp::utils::WrapText( dc, str, max_width );
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
    qwr::error::CheckGdi( pGdi_->FillEllipse( &br, x, y, w, h ), "FillEllipse" );
}

void JsGdiGraphics::FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    const Gdiplus::RectF rect{ x, y, w, h };
    Gdiplus::LinearGradientBrush brush( rect, colour1, colour2, angle, TRUE );
    qwr::error::CheckGdi( brush.SetBlendTriangularShape( focus ), "SetBlendTriangularShape" );

    qwr::error::CheckGdi( pGdi_->FillRectangle( &brush, rect ), "FillRectangle" );
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
    qwr::error::CheckGdi( pGdi_->FillPolygon( &br, gdiPoints.data(), gdiPoints.size(), static_cast<Gdiplus::FillMode>( fillmode ) ), "FillPolygon" );
}

void JsGdiGraphics::FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( 2 * arc_width <= w && 2 * arc_height <= h, "Arc argument has invalid value" );

    Gdiplus::SolidBrush br( colour );
    Gdiplus::GraphicsPath path;
    const Gdiplus::RectF rect{ x, y, w, h };
    GetRoundRectPath( path, rect, arc_width, arc_height );

    qwr::error::CheckGdi( pGdi_->FillPath( &br, &path ), "FillPath" );
}

void JsGdiGraphics::FillSolidRect( float x, float y, float w, float h, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::SolidBrush brush( colour );
    qwr::error::CheckGdi( pGdi_->FillRectangle( &brush, x, y, w, h ), "FillRectangle" );
}

void JsGdiGraphics::GdiAlphaBlend( JsGdiRawBitmap* bitmap,
                                   int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                   int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                                   uint8_t alpha )
{
    qwr::QwrException::ExpectTrue( bitmap, "bitmap argument is null" );

    const auto draw = [&]( HDC dc ) {
        const HDC srcDc = bitmap->GetHDC();
        assert( srcDc );

        qwr::error::CheckWinApi( ::GdiAlphaBlend( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, BLENDFUNCTION{ AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA } ), "GdiAlphaBlend" );
    };

    TransferGdiplusClipTransformFor( draw );
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
    qwr::QwrException::ExpectTrue( bitmap, "bitmap argument is null" );

    const auto draw = [&]( HDC dc ) {
        const HDC srcDc = bitmap->GetHDC();
        assert( srcDc );

        // commented out because possible scaling differences
        /*
        if ( dstW == srcW && dstH == srcH )
        {
            qwr::error::CheckWinApi( BitBlt( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, SRCCOPY ), "BitBlt" );
            return;
        }
        */

        qwr::error::CheckWinApi( SetStretchBltMode( dc, HALFTONE ), "SetStretchBltMode" );
        qwr::error::CheckWinApi( SetBrushOrgEx( dc, 0, 0, nullptr ), "SetBrushOrgEx" );
        qwr::error::CheckWinApi( StretchBlt( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, SRCCOPY ), "StretchBlt" );
    };

    TransferGdiplusClipTransformFor( draw );
}

void JsGdiGraphics::GdiDrawText( const std::wstring& str, JsGdiFont* font, uint32_t colour, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t format )
{
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const auto draw = [&]( HDC dc ) {
        gdi::ObjectSelector autoFont( dc, font->GetHFont() );

        RECT rect{ x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };
        DRAWTEXTPARAMS dpt = { sizeof( DRAWTEXTPARAMS ), 4, 0, 0, 0 };

        SetTextColor( dc, smp::colour::ArgbToColorref( colour ) );

        qwr::error::CheckWinApi( CLR_INVALID != SetBkMode( dc, TRANSPARENT ), "SetBkMode" );
        qwr::error::CheckWinApi( GDI_ERROR != SetTextAlign( dc, TA_LEFT | TA_TOP | TA_NOUPDATECP ), "SetTextAlign" );

        format &= ~DT_MODIFYSTRING;

        // Well, magic :P
        if ( format & DT_CALCRECT )
        {
            const RECT oldrect = rect;
            RECT rc_calc = rect;

            qwr::error::CheckWinApi( DrawText( dc, str.c_str(), -1, &rc_calc, format ), "DrawText" );

            format &= ~DT_CALCRECT;

            // adjust vertical align
            if ( format & DT_VCENTER )
            {
                rect.top = oldrect.top + ( ( ( oldrect.bottom - oldrect.top ) - ( rc_calc.bottom - rc_calc.top ) ) >> 1 );
                rect.bottom = rect.top + ( rc_calc.bottom - rc_calc.top );
            }
            else if ( format & DT_BOTTOM )
            {
                rect.top = oldrect.bottom - ( rc_calc.bottom - rc_calc.top );
            }
        }

        qwr::error::CheckWinApi( DrawTextEx( dc, const_cast<wchar_t*>( str.c_str() ), -1, &rect, format, &dpt ), "DrawTextEx" );
    };

    TransferGdiplusClipTransformFor( draw );
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

JS::Value JsGdiGraphics::GetClip()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::RectF value;
    qwr::error::CheckGdi( pGdi_->GetClipBounds( &value ), "SetClip" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ value.X, value.Y, value.Width, value.Height }, &jsValue );
    return jsValue;
}

uint32_t JsGdiGraphics::GetCompositingMode()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetCompositingMode();
}

uint32_t JsGdiGraphics::GetCompositingQuality()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetCompositingQuality();
}

float JsGdiGraphics::GetDpiX()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetDpiX();
}

float JsGdiGraphics::GetDpiY()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetDpiY();
}

uint32_t JsGdiGraphics::GetInterpolationMode()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetInterpolationMode();
}

uint32_t JsGdiGraphics::GetPixelOffsetMode()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetPixelOffsetMode();
}

uint32_t JsGdiGraphics::GetSmoothingMode()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetSmoothingMode();
}

uint32_t JsGdiGraphics::GetTextContrast()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetTextContrast();
}

uint32_t JsGdiGraphics::GetTextRenderingHint()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->GetTextRenderingHint();
}

JS::Value JsGdiGraphics::GetTransform()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Matrix matrix;
    qwr::error::CheckGdi( pGdi_->GetTransform( &matrix ), "GetTransform" );

    Gdiplus::REAL xform[6];
    qwr::error::CheckGdi( matrix.GetElements( (Gdiplus::REAL*)&xform ), "GetElements" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>( std::begin( xform ), std::end( xform ) ), &jsValue );

    return jsValue;
}

bool JsGdiGraphics::IsClipEmpty()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->IsClipEmpty();
}

bool JsGdiGraphics::IsPointVisible( float x, float y )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->IsVisible( x, y );
}

bool JsGdiGraphics::IsRectVisible( float x, float y, float w, float h )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->IsVisible( x, y, w, h );
}

bool JsGdiGraphics::IsVisibleClipEmpty()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->IsClipEmpty();
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

    qwr::error::CheckGdi( pGdi_->MeasureString( str.c_str(), -1, fn, Gdiplus::RectF( x, y, w, h ), &fmt, &bound, &chars, &lines ), "MeasureString" );

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

void JsGdiGraphics::MultiplyTransform( float m11, float m12, float m21, float m22, float dx, float dy, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Matrix matrix{ m11, m12, m21, m22, dx, dy };

    qwr::error::CheckGdi( pGdi_->MultiplyTransform( &matrix, static_cast<Gdiplus::MatrixOrder>( matrixOrder ) ), "MultiplyTransform" );
}

void JsGdiGraphics::MultiplyTransformWithOpt( size_t optArgCount, float m11, float m12, float m21, float m22, float dx, float dy, uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return MultiplyTransform( m11, m12, m21, m22, dx, dy, matrixOrder );
    case 1:
        return MultiplyTransform( m11, m12, m21, m22, dx, dy );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::ResetClip()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->ResetClip(), "ResetClip" );
}

void JsGdiGraphics::ResetTransform()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->ResetTransform(), "ResetTransform" );
}

void JsGdiGraphics::Restore( uint32_t state )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->Restore( state ), "Restore" );
}

void JsGdiGraphics::RotateTransform( float angle, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->RotateTransform( angle, static_cast<Gdiplus::MatrixOrder>( matrixOrder ) ), "RotateTransform" );
}

void JsGdiGraphics::RotateTransformWithOpt( size_t optArgCount, float angle, uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return RotateTransform( angle, matrixOrder );
    case 1:
        return RotateTransform( angle );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t JsGdiGraphics::Save()
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    return pGdi_->Save();
}

void JsGdiGraphics::ScaleTransform( float sx, float sy, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->ScaleTransform( sx, sy, static_cast<Gdiplus::MatrixOrder>( matrixOrder ) ), "ScaleTransform" );
}

void JsGdiGraphics::ScaleTransformWithOpt( size_t optArgCount, float sx, float sy, uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return ScaleTransform( sx, sy, matrixOrder );
    case 1:
        return ScaleTransform( sx, sy );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetClip( float x, float y, float w, float h )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetClip( Gdiplus::RectF( x, y, w, h ), Gdiplus::CombineModeReplace ), "SetClip" );
}

void JsGdiGraphics::SetCompositingMode( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetCompositingMode( static_cast<Gdiplus::CompositingMode>( mode ) ), "SetCompositingMode" );
}

void JsGdiGraphics::SetCompositingModeWithOpt( size_t optArgCount, uint32_t mode )
{
    switch ( optArgCount )
    {
    case 0:
        return SetCompositingMode( mode );
    case 1:
        return SetCompositingMode();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetCompositingQuality( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetCompositingQuality( static_cast<Gdiplus::CompositingQuality>( mode ) ), "SetCompositingQuality" );
}

void JsGdiGraphics::SetCompositingQualityWithOpt( size_t optArgCount, uint32_t mode )
{
    switch ( optArgCount )
    {
    case 0:
        return SetCompositingQuality( mode );
    case 1:
        return SetCompositingQuality();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetInterpolationMode( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetInterpolationMode( static_cast<Gdiplus::InterpolationMode>( mode ) ), "SetInterpolationMode" );
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

void JsGdiGraphics::SetPixelOffsetMode( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetPixelOffsetMode( static_cast<Gdiplus::PixelOffsetMode>( mode ) ), "SetPixelOffsetMode" );
}

void JsGdiGraphics::SetPixelOffsetModeWithOpt( size_t optArgCount, uint32_t mode )
{
    switch ( optArgCount )
    {
    case 0:
        return SetPixelOffsetMode( mode );
    case 1:
        return SetPixelOffsetMode();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetSmoothingMode( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetSmoothingMode( static_cast<Gdiplus::SmoothingMode>( mode ) ), "SetSmoothingMode" );
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

void JsGdiGraphics::SetTextContrast( uint32_t constrast )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetTextContrast( constrast ), "SetTextContrast" );
}

void JsGdiGraphics::SetTextRenderingHint( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->SetTextRenderingHint( static_cast<Gdiplus::TextRenderingHint>( mode ) ), "SetTextRenderingHint" );
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

void JsGdiGraphics::SetTransform( float m11, float m12, float m21, float m22, float dx, float dy )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Matrix matrix{ m11, m12, m21, m22, dx, dy };

    qwr::error::CheckGdi( pGdi_->SetTransform( &matrix ), "SetTransform" );
}

JS::Value JsGdiGraphics::TransformPoint( float x, float y )
{
    Gdiplus::PointF point{ x, y };

    qwr::error::CheckGdi( pGdi_->TransformPoints( Gdiplus::CoordinateSpaceWorld, Gdiplus::CoordinateSpacePage, &point, 1 ), "TransformPoints" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ point.X, point.Y }, &jsValue );

    return jsValue;
}

JS::Value JsGdiGraphics::TransformRect( float x, float y, float w, float h )
{
    Gdiplus::PointF points[4]{ { x, y }, { x + w, y }, { x + w, y + h }, { x, y + h } };

    qwr::error::CheckGdi( pGdi_->TransformPoints( Gdiplus::CoordinateSpaceWorld, Gdiplus::CoordinateSpacePage, (Gdiplus::PointF*)&points, 4 ), "TransformPoints" );

    float xmin = std::min<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float xmax = std::max<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float ymin = std::min<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );
    float ymax = std::max<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ xmin, ymin, xmax - xmin, ymax - ymin }, &jsValue );

    return jsValue;
}

void JsGdiGraphics::TranslateTransform( float dx, float dy, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGdi_, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGdi_->TranslateTransform( dx, dy, static_cast<Gdiplus::MatrixOrder>( matrixOrder ) ), "TranslateTransform" );
}

void JsGdiGraphics::TranslateTransformWithOpt( size_t optArgCount, float dx, float dy, uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return TranslateTransform( dx, dy, matrixOrder );
    case 1:
        return TranslateTransform( dx, dy );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsGdiGraphics::UnTransformPoint( float x, float y )
{
    Gdiplus::PointF point{ x, y };

    qwr::error::CheckGdi( pGdi_->TransformPoints( Gdiplus::CoordinateSpacePage, Gdiplus::CoordinateSpaceWorld, &point, 1 ), "TransformPoints" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ point.X, point.Y }, &jsValue );

    return jsValue;
}

JS::Value JsGdiGraphics::UnTransformRect( float x, float y, float w, float h )
{
    Gdiplus::PointF points[4]{ { x, y }, { x + w, y }, { x + w, y + h }, { x, y + h } };

    qwr::error::CheckGdi( pGdi_->TransformPoints( Gdiplus::CoordinateSpacePage, Gdiplus::CoordinateSpaceWorld, (Gdiplus::PointF*)&points, 2 ), "TransformPoints" );

    float xmin = std::min<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float xmax = std::max<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float ymin = std::min<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );
    float ymax = std::max<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ xmin, ymin, xmax - xmin, ymax - ymin }, &jsValue );

    return jsValue;
}

void JsGdiGraphics::GetRoundRectPath( Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, float arc_width, float arc_height ) const
{
    const float arc_dia_w = arc_width * 2;
    const float arc_dia_h = arc_height * 2;
    Gdiplus::RectF corner{ rect.X, rect.Y, arc_dia_w, arc_dia_h };

    qwr::error::CheckGdi( path.Reset(), "Reset" );

    // top left
    qwr::error::CheckGdi( path.AddArc( corner, 180, 90 ), "AddArc" );

    // top right
    corner.X += ( rect.Width - arc_dia_w );
    qwr::error::CheckGdi( path.AddArc( corner, 270, 90 ), "AddArc" );

    // bottom right
    corner.Y += ( rect.Height - arc_dia_h );
    qwr::error::CheckGdi( path.AddArc( corner, 0, 90 ), "AddArc" );

    // bottom left
    corner.X -= ( rect.Width - arc_dia_w );
    qwr::error::CheckGdi( path.AddArc( corner, 90, 90 ), "AddArc" );
    qwr::error::CheckGdi( path.CloseFigure(), "CloseFigure" );
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
