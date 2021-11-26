#include <stdafx.h>

#include "gdi_graphics.h"

#include <js_engine/js_to_native_invoker.h>

#include <js_objects/gdi_bitmap.h>
#include <js_objects/gdi_font.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <js_objects/measure_string_info.h>

#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>

#include <utils/colour_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/text_helpers.h>

#include <utils/dwrite_renderer.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

using namespace smp;

using smp::dwrite::GdiTextRenderer;
using smp::gdi::WrapGdiCalls;

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
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CalcWriteTextHeight, JsGdiGraphics::CalcWriteTextHeight, JsGdiGraphics::CalcWriteTextHeightWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CalcWriteTextWidth, JsGdiGraphics::CalcWriteTextWidth, JsGdiGraphics::CalcWriteTextWidthWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawEllipse, JsGdiGraphics::DrawEllipse )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawFrameControl, JsGdiGraphics::DrawFrameControl )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawFocusRect, JsGdiGraphics::DrawFocusRect )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawImage, JsGdiGraphics::DrawImage, JsGdiGraphics::DrawImageWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawLine, JsGdiGraphics::DrawLine )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawString, JsGdiGraphics::DrawString, JsGdiGraphics::DrawStringWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawPolygon, JsGdiGraphics::DrawPolygon )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawRect, JsGdiGraphics::DrawRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( DrawRoundRect, JsGdiGraphics::DrawRoundRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( EndContainer, JsGdiGraphics::EndContainer )
MJS_DEFINE_JS_FN_FROM_NATIVE( EstimateLineWrap, JsGdiGraphics::EstimateLineWrap )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillEllipse, JsGdiGraphics::FillEllipse )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( FillGradRect, JsGdiGraphics::FillGradRect, JsGdiGraphics::FillGradRectWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillPolygon, JsGdiGraphics::FillPolygon )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillRoundRect, JsGdiGraphics::FillRoundRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( FillSolidRect, JsGdiGraphics::FillSolidRect )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlendWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GdiDrawBitmap, JsGdiGraphics::GdiDrawBitmap )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiDrawText, JsGdiGraphics::GdiDrawText, JsGdiGraphics::GdiDrawTextWithOpt, 1 )

MJS_DEFINE_JS_FN_FROM_NATIVE( GetClip, JsGdiGraphics::GetClip )
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
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( WriteString, JsGdiGraphics::WriteString, JsGdiGraphics::WriteStringWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( WriteText, JsGdiGraphics::WriteText, JsGdiGraphics::WriteTextWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "BeginContainer", BeginContainer, 0, kDefaultPropsFlags ),
        JS_FN( "CalcTextHeight", CalcTextHeight, 2, kDefaultPropsFlags ),
        JS_FN( "CalcTextWidth", CalcTextWidth, 2, kDefaultPropsFlags ),
        JS_FN( "CalcWriteTextHeight", CalcWriteTextHeight, 2, kDefaultPropsFlags ),
        JS_FN( "CalcWriteTextWidth", CalcWriteTextWidth, 2, kDefaultPropsFlags ),
        JS_FN( "Clear", Clear, 1, kDefaultPropsFlags ),
        JS_FN( "DrawEllipse", DrawEllipse, 6, kDefaultPropsFlags ),
        JS_FN( "DrawFrameControl", DrawFrameControl, 4, kDefaultPropsFlags ),
        JS_FN( "DrawFocusRect", DrawFocusRect, 6, kDefaultPropsFlags ),
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
        JS_FN( "WriteString", WriteString, 7, kDefaultPropsFlags ),
        JS_FN( "WriteText", WriteText, 7, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "CompositingQuality", GetCompositingQuality, SetCompositingQuality, kDefaultPropsFlags ),
        JS_PSG( "DpiX", GetDpiX, kDefaultPropsFlags ),
        JS_PSG( "DpiY", GetDpiY, kDefaultPropsFlags ),
        JS_PSGS( "InterpolationMode", GetInterpolationMode, SetInterpolationMode, kDefaultPropsFlags ),
        JS_PSGS( "PixelOffsetMode", GetPixelOffsetMode, SetPixelOffsetMode, kDefaultPropsFlags ),
        JS_PSGS( "SmoothingMode", GetSmoothingMode, SetSmoothingMode, kDefaultPropsFlags ),
        JS_PSGS( "TextContrast", GetTextContrast, SetTextContrast, kDefaultPropsFlags ),
        JS_PSGS( "TextRenderingHint", GetTextRenderingHint, SetTextRenderingHint, kDefaultPropsFlags ),
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
JsGdiGraphics::CreateNative( JSContext* ctx )
{
    return std::unique_ptr<JsGdiGraphics>( new JsGdiGraphics( ctx ) );
}

size_t JsGdiGraphics::GetInternalSize()
{
    return 0;
}

Gdiplus::Graphics* JsGdiGraphics::GetGraphicsObject() const
{
    return pGraphics;
}

void JsGdiGraphics::SetGraphicsObject( Gdiplus::Graphics* graphics )
{
    pGraphics = graphics;
}

uint32_t JsGdiGraphics::BeginContainer( float dstX, float dstY, float dstW, float dstH,
                                        float srcX, float srcY, float srcW, float srcH )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::RectF dst{ dstX, dstY, dstW, dstH };
    Gdiplus::RectF src{ srcX, srcY, srcW, srcH };

    if ( !dst.IsEmptyArea() && !src.IsEmptyArea() )
    {
        return pGraphics->BeginContainer( dst, src, Gdiplus::UnitPixel );
    }
    else
    {
        return pGraphics->BeginContainer();
    }
}

uint32_t JsGdiGraphics::BeginContainerWithOpt( size_t optArgCount,
                                               float dstX, float dstY, float dstW, float dstH,
                                               float srcX, float srcY, float srcW, float srcH )
{
    switch ( optArgCount )
    {
    case 0:
        return BeginContainer( dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH );
    case 1:
        return BeginContainer( dstX, dstY, dstW, dstH, srcX, srcY, srcW );
    case 2:
        return BeginContainer( dstX, dstY, dstW, dstH, srcX, srcY );
    case 3:
        return BeginContainer( dstX, dstY, dstW, dstH, srcX );
    case 4:
        return BeginContainer( dstX, dstY, dstW, dstH );
    case 5:
        return BeginContainer( dstX, dstY, dstW );
    case 6:
        return BeginContainer( dstX, dstY );
    case 7:
        return BeginContainer( dstX );
    case 8:
        return BeginContainer();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t JsGdiGraphics::CalcTextHeight( const std::wstring& text, JsGdiFont* font )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const HDC dc = pGraphics->GetHDC();
    qwr::final_action autoHdcReleaser( [dc, pGdi = pGraphics] { pGdi->ReleaseHDC( dc ); } );
    gdi::ObjectSelector autoFont( dc, font->HFont() );

    return smp::utils::GetTextHeight( dc, text );
}

uint32_t JsGdiGraphics::CalcTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const HDC dc = pGraphics->GetHDC();
    qwr::final_action autoHdcReleaser( [dc, pGdi = pGraphics] { pGdi->ReleaseHDC( dc ); } );
    gdi::ObjectSelector autoFont( dc, font->HFont() );

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

uint32_t JsGdiGraphics::CalcWriteTextHeight( const std::wstring& text, JsGdiFont* font, boolean use_exact )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const HDC dc = pGraphics->GetHDC();
    qwr::final_action autoHdcReleaser( [dc, pGdi = pGraphics] { pGdi->ReleaseHDC( dc ); } );

    CComPtr<GdiTextRenderer> renderer = new GdiTextRenderer( dc );
    CComPtr<IDWriteTextFormat> textFormat = renderer->GetTextFormat( font->HFont() );
    CComPtr<IDWriteTextLayout> textLayout = renderer->GetTextLayout( textFormat, text, 4096.0f, 4096.0f );

    DWRITE_TEXT_METRICS1 textMetrics;
    CComQIPtr<IDWriteTextLayout2>( textLayout )->GetMetrics( &textMetrics );

    return static_cast<uint32_t>( use_exact ? textMetrics.heightIncludingTrailingWhitespace : textMetrics.height );
}

uint32_t JsGdiGraphics::CalcWriteTextHeightWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, boolean use_exact )
{
    switch ( optArgCount )
    {
    case 0:
        return CalcWriteTextHeight( text, font, use_exact );
    case 1:
        return CalcWriteTextHeight( text, font );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t JsGdiGraphics::CalcWriteTextWidth( const std::wstring& text, JsGdiFont* font, boolean use_exact )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    const HDC dc = pGraphics->GetHDC();
    qwr::final_action autoHdcReleaser( [dc, pGdi = pGraphics] { pGdi->ReleaseHDC( dc ); } );

    CComPtr<GdiTextRenderer> renderer = new GdiTextRenderer( dc );
    CComPtr<IDWriteTextFormat> textFormat = renderer->GetTextFormat( font->HFont() );
    CComPtr<IDWriteTextLayout> textLayout = renderer->GetTextLayout( textFormat, text, 4096.0f, 4096.0f );

    DWRITE_TEXT_METRICS textMetrics;
    textLayout->GetMetrics( &textMetrics );

    FLOAT width = textMetrics.width;

    return static_cast<uint32_t>( use_exact ? textMetrics.widthIncludingTrailingWhitespace : textMetrics.width );
}

uint32_t JsGdiGraphics::CalcWriteTextWidthWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font, boolean use_exact )
{
    switch ( optArgCount )
    {
    case 0:
        return CalcWriteTextWidth( text, font, use_exact );
    case 1:
        return CalcWriteTextWidth( text, font );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::Clear( uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->Clear( colour ), "Clear" );
}

JS::Value JsGdiGraphics::DrawEdge( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                   uint32_t edge, uint32_t flags )
{
    JS::RootedValue jsValue( pJsCtx_ );

    WrapGdiCalls( pGraphics, [&]( HDC dc )
    {
        makeRECT( rect );

        qwr::error::CheckWinApi( ::DrawEdge( dc, &rect, edge, flags ), "DrawEdge" );

        if ( flags & BF_ADJUST )
        {
            std::vector<int32_t> content{ rect.top, rect.left, RECT_CX(rect), RECT_CY(rect) };
            convert::to_js::ToArrayValue( pJsCtx_, content, &jsValue );
        }
    } );

    return jsValue;
}

void JsGdiGraphics::DrawEllipse( float x, float y, float w, float h,
                                  float lineWidth, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, lineWidth );
    qwr::error::CheckGdi( pGraphics->DrawEllipse( &pen, x, y, w, h ), "DrawEllipse" );
}

void JsGdiGraphics::DrawFrameControl( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                      uint32_t type, uint32_t state )
{
    WrapGdiCalls( pGraphics, [&]( HDC dc ) {
        makeRECT( rect );
        qwr::error::CheckWinApi( ::DrawFrameControl( dc, &rect, type, state ), "DrawFocusRect" );
    } );
}

void JsGdiGraphics::DrawFocusRect( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH)
{
    WrapGdiCalls( pGraphics, [&]( HDC dc )
    {
        makeRECT( rect );
        qwr::error::CheckWinApi( ::DrawFocusRect( dc, &rect ), "DrawFocusRect" );
    } );
}

void JsGdiGraphics::DrawImage( JsGdiBitmap* image,
                               float dstX, float dstY, float dstW, float dstH,
                               float srcX, float srcY, float srcW, float srcH,
                               float angle, uint8_t alpha )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( image, "image argument is null" );

    Gdiplus::Bitmap* img = image->GdiBitmap();
    assert( img );

    qwr::final_action autoStateRestore
    (
        [pGraphics = pGraphics, state = pGraphics->BeginContainer()]
        {
            pGraphics->EndContainer( state );
        }
    );

    Gdiplus::RectF src{ dstX, dstY, dstW, dstH };
    Gdiplus::RectF dst{ srcX, srcY, srcW, srcH };

    if ( fabs( fmod( 360.0f, angle ) ) > 1e3 )
    {
        Gdiplus::Matrix rotate;
        qwr::error::CheckGdi( rotate.RotateAt( angle, Gdiplus::PointF{ dstX + dstW / 2, dstY + dstH / 2 } ), "RotateAt" );
    }

    Gdiplus::ImageAttributes imgAttrs = {};

    if ( alpha < 0xff )
    {
        Gdiplus::ColorMatrix cm{};

        cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
        cm.m[3][3] = static_cast<float>( alpha ) / 255;

        qwr::error::CheckGdi( imgAttrs.SetColorMatrix( &cm ), "SetColorMatrix" );
    }

    qwr::error::CheckGdi( pGraphics->DrawImage( img, dst, src, Gdiplus::UnitPixel, &imgAttrs ), "DrawImage" );
}

void JsGdiGraphics::DrawImageWithOpt( size_t optArgCount, JsGdiBitmap* image,
                                      float dstX, float dstY, float dstW, float dstH,
                                      float srcX, float srcY, float srcW, float srcH,
                                      float angle, uint8_t alpha )
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

void JsGdiGraphics::DrawLine( float x1, float y1, float x2, float y2, float lineWidth, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, lineWidth );
    qwr::error::CheckGdi( pGraphics->DrawLine( &pen, x1, y1, x2, y2 ), "DrawLine" );
}

void JsGdiGraphics::DrawPolygon( uint32_t colour, float lineWidth, JS::HandleValue points )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::Pen pen( colour, lineWidth );
    qwr::error::CheckGdi( pGraphics->DrawPolygon( &pen, gdiPoints.data(), gdiPoints.size() ), "DrawPolygon" );
}

void JsGdiGraphics::DrawRect( float rectX, float rectY, float rectW, float rectH, float lineWidth, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, lineWidth );
    qwr::error::CheckGdi( pGraphics->DrawRectangle( &pen, rectX, rectY, rectW, rectH ), "DrawRectangle" );
}

void JsGdiGraphics::DrawRoundRect( float rectX, float rectY, float rectW, float rectH,
                                   float arcW, float arcH, float lineWidth, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Pen pen( colour, lineWidth );
    Gdiplus::GraphicsPath path;
    GetRoundRectPath
    (
        path,
        Gdiplus::RectF{ rectX, rectY, rectW, rectH },
        std::min( rectW / 2, arcW ), std::min( rectH / 2, arcH )
    );

    qwr::error::CheckGdi( pGraphics->DrawPath( &pen, &path ), "DrawPath" );
}

void JsGdiGraphics::DrawString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                float rectX, float rectY, float rectW, float rectH,
                                uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    HDC dc = pGraphics->GetHDC();
    Gdiplus::Font* gdipFont = new Gdiplus::Font( dc, font->HFont() );
    pGraphics->ReleaseHDC( dc );

    qwr::QwrException::ExpectTrue( gdipFont, "Internal error: GdiFont is null" );

    Gdiplus::SolidBrush brush( colour );

    qwr::error::CheckGdi( pGraphics->DrawString
    (
        text.c_str(), text.length(), gdipFont,
        Gdiplus::RectF( rectX, rectY, rectW, rectH ),
        UnpackStringFormat( flags ),
        &brush
    ), "DrawString" );
}

void JsGdiGraphics::DrawStringWithOpt( size_t optArgCount,
                                       const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                       float rectX, float rectY, float rectW, float rectH,
                                       uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawString( text, font, colour, rectX, rectY, rectW, rectH, flags );
    case 1:
        return DrawString( text, font, colour, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::EndContainer( uint32_t state )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->EndContainer( state ), "EndContainer" );
}

JSObject* JsGdiGraphics::EstimateLineWrap( const std::wstring& str, JsGdiFont* font, uint32_t max_width )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    std::vector<smp::utils::WrappedTextLine> result;

    {
        const HDC dc = pGraphics->GetHDC();
        qwr::final_action autoHdcReleaser( [dc, pGdi = pGraphics] { pGdi->ReleaseHDC( dc ); } );
        gdi::ObjectSelector autoFont( dc, font->HFont() );

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
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::SolidBrush brush( colour );
    qwr::error::CheckGdi( pGraphics->FillEllipse( &brush, x, y, w, h ), "FillEllipse" );
}

void JsGdiGraphics::FillGradRect( float rectX, float rectY, float rectW, float rectH,
                                  float angle, uint32_t colour1, uint32_t colour2,
                                  float focus, float scale )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    const Gdiplus::RectF rect{ rectX, rectY, rectW, rectH };
    Gdiplus::LinearGradientBrush brush( rect, colour1, colour2, angle, TRUE );
    qwr::error::CheckGdi( brush.SetBlendTriangularShape( focus, scale ), "SetBlendTriangularShape" );

    qwr::error::CheckGdi( pGraphics->FillRectangle( &brush, rect ), "FillRectangle" );
}

void JsGdiGraphics::FillGradRectWithOpt( size_t optArgCount,
                                         float rectX, float rectY, float rectW, float rectH,
                                         float angle, uint32_t colour1, uint32_t colour2,
                                         float focus, float scale )
{
    switch ( optArgCount )
    {
    case 0:
        return FillGradRect( rectX, rectY, rectW, rectH, angle, colour1, colour2, focus, scale );
    case 1:
        return FillGradRect( rectX, rectY, rectW, rectH, angle, colour1, colour2, focus );
    case 2:
        return FillGradRect( rectX, rectY, rectW, rectH, angle, colour1, colour2 );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::FillPolygon( uint32_t colour, uint32_t fillmode, JS::HandleValue points )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    std::vector<Gdiplus::PointF> gdiPoints;
    ParsePoints( points, gdiPoints );

    Gdiplus::SolidBrush brush( colour );
    qwr::error::CheckGdi( pGraphics->FillPolygon
    (
        &brush,
        gdiPoints.data(),
        gdiPoints.size(),
        static_cast<Gdiplus::FillMode>( fillmode )
    ), "FillPolygon" );
}

void JsGdiGraphics::FillRoundRect( float rectX, float rectY, float rectW, float rectH,
                                   float arcW, float arcH, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::SolidBrush brush( colour );
    Gdiplus::GraphicsPath path;
    const Gdiplus::RectF rect{ rectX, rectY, rectW, rectH };
    GetRoundRectPath( path, rect, std::max( rectW / 2, arcW ), std::max( rectH / 2, arcH ) );

    qwr::error::CheckGdi( pGraphics->FillPath( &brush, &path ), "FillPath" );
}

void JsGdiGraphics::FillSolidRect( float rectX, float rectY, float rectW, float rectH, uint32_t colour )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::SolidBrush brush( colour );
    qwr::error::CheckGdi( pGraphics->FillRectangle( &brush, rectX, rectY, rectW, rectH ), "FillRectangle" );
}

void JsGdiGraphics::GdiAlphaBlend( JsGdiRawBitmap* bitmap,
                                   int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
                                   int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
                                   uint8_t alpha )
{
    qwr::QwrException::ExpectTrue( bitmap, "bitmap argument is null" );

    WrapGdiCalls( pGraphics, [&]( HDC dstDC )
    {
        const HDC srcDC = bitmap->GetHDC();
        assert( srcDC );

        qwr::error::CheckWinApi( ::GdiAlphaBlend
        (
            dstDC, dstX, dstY, dstW, dstH,
            srcDC, srcX, srcY, srcW, srcH,
            { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA }
        ), "GdiAlphaBlend" );
    } );
}

void JsGdiGraphics::GdiAlphaBlendWithOpt( size_t optArgCount,
                                          JsGdiRawBitmap* bitmap,
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

    WrapGdiCalls( pGraphics, [&]( HDC dstDC )
    {
        const HDC srcDC = bitmap->GetHDC();
        assert( srcDC );

        // commented because possible scaling/rotation differences
        /*
        if ( dstW == srcW && dstH == srcH )
        {
            qwr::error::CheckWinApi( BitBlt( dc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, SRCCOPY ), "BitBlt" );
            return;
        }
        */

        qwr::error::CheckWinApi( SetStretchBltMode( dstDC, HALFTONE ), "SetStretchBltMode" );
        qwr::error::CheckWinApi( SetBrushOrgEx( dstDC, 0, 0, nullptr ), "SetBrushOrgEx" );
        qwr::error::CheckWinApi( StretchBlt
        (
            dstDC, dstX, dstY, dstW, dstH,
            srcDC, srcX, srcY, srcW, srcH,
            SRCCOPY
        ), "StretchBlt" );
    } );
}

void JsGdiGraphics::GdiDrawText( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                 int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                 uint32_t format )
{
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    WrapGdiCalls( pGraphics, [&]( HDC dc )
    {
        gdi::ObjectSelector autoFont( dc, font->HFont() );

        makeRECT( rect );

        DRAWTEXTPARAMS dpt = { sizeof( DRAWTEXTPARAMS ), 4, 0, 0, 0 };

        SetTextColor( dc, smp::colour::ArgbToColorref( colour ) );

        qwr::error::CheckWinApi( CLR_INVALID != SetBkMode( dc, TRANSPARENT ), "SetBkMode" );
        qwr::error::CheckWinApi( GDI_ERROR != SetTextAlign( dc, TA_LEFT | TA_TOP | TA_NOUPDATECP ), "SetTextAlign" );

        format &= ~DT_MODIFYSTRING;

        // Well, magic :P
        if ( format & DT_CALCRECT )
        {
            const RECT oldrect = rect;
            RECT calcrect = rect;

            qwr::error::CheckWinApi( DrawTextW( dc, text.c_str(), text.length(), &calcrect, format ), "DrawText" );

            format &= ~DT_CALCRECT;

            // adjust vertical align
            if ( format & DT_VCENTER )
            {
                rect.top = oldrect.top + ( ( ( oldrect.bottom - oldrect.top ) - ( calcrect.bottom - calcrect.top ) ) >> 1 );
                rect.bottom = rect.top + ( calcrect.bottom - calcrect.top );
            }
            else if ( format & DT_BOTTOM )
            {
                rect.top = oldrect.bottom - ( calcrect.bottom - calcrect.top );
            }
        }

        qwr::error::CheckWinApi( DrawTextExW
        (
            dc, const_cast<wchar_t*>( text.c_str() ), text.length(), &rect, format, &dpt
        ), "DrawTextEx" );
    } );
}

void JsGdiGraphics::GdiDrawTextWithOpt( size_t optArgCount,
                                        const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                        int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                        uint32_t format )
{
    switch ( optArgCount )
    {
    case 0:
        return GdiDrawText( text, font, colour, rectX, rectY, rectW, rectH, format );
    case 1:
        return GdiDrawText( text, font, colour, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsGdiGraphics::GetClip()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::RectF value;
    qwr::error::CheckGdi( pGraphics->GetClipBounds( &value ), "SetClip" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ value.X, value.Y, value.Width, value.Height }, &jsValue );
    return jsValue;
}

uint32_t JsGdiGraphics::GetCompositingQuality()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetCompositingQuality();
}

float JsGdiGraphics::GetDpiX()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetDpiX();
}

float JsGdiGraphics::GetDpiY()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetDpiY();
}

uint32_t JsGdiGraphics::GetInterpolationMode()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetInterpolationMode();
}

uint32_t JsGdiGraphics::GetPixelOffsetMode()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetPixelOffsetMode();
}

uint32_t JsGdiGraphics::GetSmoothingMode()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetSmoothingMode();
}

uint32_t JsGdiGraphics::GetTextContrast()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetTextContrast();
}

uint32_t JsGdiGraphics::GetTextRenderingHint()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->GetTextRenderingHint();
}

JS::Value JsGdiGraphics::GetTransform()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Matrix matrix;
    qwr::error::CheckGdi( pGraphics->GetTransform( &matrix ), "GetTransform" );

    Gdiplus::REAL xform[6] = {};
    qwr::error::CheckGdi( matrix.GetElements( (Gdiplus::REAL*)&xform ), "GetElements" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>( std::begin( xform ), std::end( xform ) ), &jsValue );

    return jsValue;
}

bool JsGdiGraphics::IsClipEmpty()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->IsClipEmpty();
}

bool JsGdiGraphics::IsPointVisible( float x, float y )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->IsVisible( x, y );
}

bool JsGdiGraphics::IsRectVisible( float rectX, float rectY, float rectW, float rectH )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->IsVisible( rectX, rectY, rectW, rectH );
}

bool JsGdiGraphics::IsVisibleClipEmpty()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->IsClipEmpty();
}

JSObject* JsGdiGraphics::MeasureString( const std::wstring& text, JsGdiFont* font,
                                        float rectX, float rectY, float rectW, float rectH,
                                        uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    HDC dc = pGraphics->GetHDC();
    Gdiplus::Font* gdipFont = new Gdiplus::Font( dc, font->HFont() );
    pGraphics->ReleaseHDC( dc );

    qwr::QwrException::ExpectTrue( gdipFont, "Internal error: GdiFont is null" );

    Gdiplus::RectF bounds;
    INT lines;
    INT chars;

    qwr::error::CheckGdi( pGraphics->MeasureString
    (
        text.c_str(), -1, gdipFont,
        Gdiplus::RectF( rectX, rectY, rectW, rectH ), UnpackStringFormat( flags ),
        &bounds, &chars, &lines
    ), "MeasureString" );

    return JsMeasureStringInfo::CreateJs( pJsCtx_, bounds.X, bounds.Y, bounds.Width, bounds.Height, lines, chars );
}

JSObject* JsGdiGraphics::MeasureStringWithOpt( size_t optArgCount, const std::wstring& text, JsGdiFont* font,
                                               float rectX, float rectY, float rectW, float rectH,
                                               uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return MeasureString( text, font, rectX, rectY, rectW, rectH, flags );
    case 1:
        return MeasureString( text, font, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::MultiplyTransform( float M11, float M12, float M21, float M22, float dX, float dY,
                                       uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    Gdiplus::Matrix matrix{ M11, M12, M21, M22, dX, dY };
    qwr::error::CheckGdi( pGraphics->MultiplyTransform
    (
        &matrix,  static_cast<Gdiplus::MatrixOrder>( matrixOrder )
    ), "MultiplyTransform" );
}

void JsGdiGraphics::MultiplyTransformWithOpt( size_t optArgCount,
                                              float M11, float M12, float M21, float M22, float dX, float dY,
                                              uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return MultiplyTransform( M11, M12, M21, M22, dX, dY, matrixOrder );
    case 1:
        return MultiplyTransform( M11, M12, M21, M22, dX, dY );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::ResetClip()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::error::CheckGdi( pGraphics->ResetClip(), "ResetClip" );
}

void JsGdiGraphics::ResetTransform()
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::error::CheckGdi( pGraphics->ResetTransform(), "ResetTransform" );
}

void JsGdiGraphics::Restore( uint32_t state )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::error::CheckGdi( pGraphics->Restore( state ), "Restore" );
}

void JsGdiGraphics::RotateTransform( float angle, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->RotateTransform
    (
        angle, static_cast<Gdiplus::MatrixOrder>( matrixOrder )
    ), "RotateTransform" );
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
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    return pGraphics->Save();
}

void JsGdiGraphics::ScaleTransform( float sX, float sY, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->ScaleTransform
    (
        sX, sY, static_cast<Gdiplus::MatrixOrder>( matrixOrder )
    ), "ScaleTransform" );
}

void JsGdiGraphics::ScaleTransformWithOpt( size_t optArgCount, float sX, float sY, uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return ScaleTransform( sX, sY, matrixOrder );
    case 1:
        return ScaleTransform( sX, sY );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::SetClip( float rectX, float rectY, float rectW, float rectH )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->SetClip
    (
        Gdiplus::RectF( rectX, rectY, rectW, rectH ), Gdiplus::CombineModeReplace
    ), "SetClip" );
}

void JsGdiGraphics::SetCompositingQuality( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->SetCompositingQuality(
        static_cast<Gdiplus::CompositingQuality>( mode )
    ), "SetCompositingQuality" );
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
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->SetInterpolationMode(
        static_cast<Gdiplus::InterpolationMode>( mode )
    ), "SetInterpolationMode" );
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
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->SetPixelOffsetMode(
        static_cast<Gdiplus::PixelOffsetMode>( mode )
    ), "SetPixelOffsetMode" );
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
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::error::CheckGdi( pGraphics->SetSmoothingMode( static_cast<Gdiplus::SmoothingMode>( mode ) ), "SetSmoothingMode" );
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
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::error::CheckGdi( pGraphics->SetTextContrast( constrast ), "SetTextContrast" );
}

void JsGdiGraphics::SetTextRenderingHint( uint32_t mode )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->SetTextRenderingHint(
        static_cast<Gdiplus::TextRenderingHint>( mode )
    ), "SetTextRenderingHint" );
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

void JsGdiGraphics::SetTransform( float M11, float M12, float M21, float M22, float dX, float dY )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    Gdiplus::Matrix matrix{ M11, M12, M21, M22, dX, dY };
    qwr::error::CheckGdi( pGraphics->SetTransform( &matrix ), "SetTransform" );
}

JS::Value JsGdiGraphics::TransformPoint( float x, float y )
{
    Gdiplus::PointF point{ x, y };

    qwr::error::CheckGdi( pGraphics->TransformPoints
    (
        Gdiplus::CoordinateSpaceWorld,
        Gdiplus::CoordinateSpacePage,
        &point, 1
    ), "TransformPoints" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ point.X, point.Y }, &jsValue );

    return jsValue;
}

JS::Value JsGdiGraphics::TransformRect( float rectX, float rectY, float rectW, float rectH )
{
    Gdiplus::PointF points[4]
    {
        { rectX,         rectY },
        { rectX + rectW, rectY },
        { rectX + rectW, rectY + rectH },
        { rectX,         rectY + rectH }
    };

    qwr::error::CheckGdi( pGraphics->TransformPoints
    (
        Gdiplus::CoordinateSpaceWorld,
        Gdiplus::CoordinateSpacePage,
        (Gdiplus::PointF*)&points, 4
    ), "TransformPoints" );

    float xMin = std::min<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float xMax = std::max<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float yMin = std::min<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );
    float yMax = std::max<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ xMin, yMin, xMax - xMin, yMax - yMin }, &jsValue );

    return jsValue;
}

void JsGdiGraphics::TranslateTransform( float dX, float dY, uint32_t matrixOrder )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );

    qwr::error::CheckGdi( pGraphics->TranslateTransform
    (
        dX, dY, static_cast<Gdiplus::MatrixOrder>( matrixOrder )
    ), "TranslateTransform" );
}

void JsGdiGraphics::TranslateTransformWithOpt( size_t optArgCount, float dX, float dY, uint32_t matrixOrder )
{
    switch ( optArgCount )
    {
    case 0:
        return TranslateTransform( dX, dY, matrixOrder );
    case 1:
        return TranslateTransform( dX, dY );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsGdiGraphics::UnTransformPoint( float x, float y )
{
   Gdiplus::PointF point{ x, y };

    qwr::error::CheckGdi( pGraphics->TransformPoints
    (
        Gdiplus::CoordinateSpacePage,
        Gdiplus::CoordinateSpaceWorld,
        &point, 1
    ), "TransformPoints" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ point.X, point.Y }, &jsValue );

    return jsValue;
}

JS::Value JsGdiGraphics::UnTransformRect( float rectX, float rectY, float rectW, float rectH )
{
    Gdiplus::PointF points[4]
    {
        { rectX,         rectY },
        { rectX + rectW, rectY },
        { rectX + rectW, rectY + rectH },
        { rectX,         rectY + rectH }
    };

    qwr::error::CheckGdi( pGraphics->TransformPoints
    (
        Gdiplus::CoordinateSpacePage,
        Gdiplus::CoordinateSpaceWorld,
        (Gdiplus::PointF*)&points, 4
    ), "TransformPoints" );

    float xMin = std::min<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float xMax = std::max<float>( { points[0].X, points[1].X, points[2].X, points[3].X } );
    float yMin = std::min<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );
    float yMax = std::max<float>( { points[0].Y, points[1].Y, points[2].Y, points[3].Y } );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<float>{ xMin, yMin, xMax - xMin, yMax - yMin }, &jsValue );

    return jsValue;
}

void JsGdiGraphics::WriteString( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                 float rectX, float rectY, float rectW, float rectH,
                                 uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    WrapGdiCalls( pGraphics, [&]( HDC dc )
    {
        CComPtr<GdiTextRenderer> renderer = new GdiTextRenderer( dc );
        CComPtr<IDWriteTextFormat> textFormat = renderer->GetTextFormat( font->HFont() );
        CComPtr<IDWriteTextLayout> textLayout = renderer->GetTextLayout( textFormat, text, rectW, rectH );

        DWRITE_TEXT_METRICS textMetrics;
        textLayout->GetMetrics( &textMetrics );

        Gdiplus::StringAlignment halign = static_cast<Gdiplus::StringAlignment>( ( flags >> 28 ) & 0x3 ); //0xf0000000
        Gdiplus::StringAlignment valign = static_cast<Gdiplus::StringAlignment>( ( flags >> 24 ) & 0x3 ); //0x0f000000
        Gdiplus::StringTrimming strtrim = static_cast<Gdiplus::StringTrimming> ( ( flags >> 20 ) & 0x7 ); //0x00f00000
        Gdiplus::StringFormatFlags fmts = static_cast<Gdiplus::StringFormatFlags>( flags & 0x7FFF ); //0x0000ffff

        // valign
        if ( halign == Gdiplus::StringAlignmentNear ) // 0 => 0
            textLayout->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_LEADING );

        if ( halign == Gdiplus::StringAlignmentFar ) // 2 => 1
            textLayout->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_TRAILING );

        if ( halign == Gdiplus::StringAlignmentCenter ) // 1 => 2
            textLayout->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_CENTER );

        // halign
        if ( valign == Gdiplus::StringAlignmentNear ) // 0 => 0
            textLayout->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_NEAR );

        if ( valign == Gdiplus::StringAlignmentFar ) // 2 => 1
            textLayout->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_FAR );

        if ( valign == Gdiplus::StringAlignmentCenter ) // 1 => 2
            textLayout->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );

        // trim
        if ( !( fmts & Gdiplus::StringFormatFlagsNoClip ) )
        {
            DWRITE_TRIMMING dwTrim{ DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
            CComPtr<IDWriteInlineObject> trimSymbol = nullptr;

            if ( strtrim == Gdiplus::StringTrimmingNone )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_NONE;

            if ( strtrim == Gdiplus::StringTrimmingCharacter )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;

            if ( strtrim == Gdiplus::StringTrimmingWord )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;

            if ( strtrim == Gdiplus::StringTrimmingEllipsisCharacter )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;

            if ( strtrim == Gdiplus::StringTrimmingEllipsisWord )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;

            if ( strtrim == Gdiplus::StringTrimmingEllipsisPath )
            {
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;
                dwTrim.delimiter = '\\';
                dwTrim.delimiterCount = 1;
            }

            if ( strtrim >= Gdiplus::StringTrimmingEllipsisCharacter
                 || strtrim == Gdiplus::StringTrimmingEllipsisWord
                 || strtrim == Gdiplus::StringTrimmingEllipsisPath )
            {
                trimSymbol = renderer->CreateEllipsisTrimmingSign( textFormat );
            }

            textLayout->SetTrimming( &dwTrim, trimSymbol );
        }

        // fmts
        if ( fmts & Gdiplus::StringFormatFlagsDirectionRightToLeft )
            textLayout->SetReadingDirection( DWRITE_READING_DIRECTION_RIGHT_TO_LEFT );

        if ( fmts & Gdiplus::StringFormatFlagsDirectionVertical )
            textLayout->SetReadingDirection( DWRITE_READING_DIRECTION_TOP_TO_BOTTOM );

        if ( fmts & Gdiplus::StringFormatFlagsNoWrap )
            textLayout->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );
        else
            textFormat->SetWordWrapping( DWRITE_WORD_WRAPPING_WRAP );

        CComQIPtr<IDWriteTextLayout2> textLayout2( textLayout );
        textLayout2->SetLastLineWrapping( false );

        if ( font->get_Underline() )
            textLayout->SetUnderline( true, { 0, text.length() } );

        if ( font->get_Strikeout() )
            textLayout->SetStrikethrough( true, { 0, text.length() } );

        // render
        renderer->RenderLayout( textLayout, colour, rectX, rectX, fmts & Gdiplus::StringFormatFlagsNoClip );
    } );
}

void JsGdiGraphics::WriteStringWithOpt( size_t optArgCount,
                                        const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                        float rectX, float rectY, float rectW, float rectH,
                                        uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return WriteString( text, font, colour, rectX, rectY, rectW, rectH, flags );
    case 1:
        return WriteString( text, font, colour, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::WriteText( const std::wstring& text, JsGdiFont* font, uint32_t colour,
                               float rectX, float rectY, float rectW, float rectH,
                               uint32_t flags )
{
    qwr::QwrException::ExpectTrue( pGraphics, "Internal error: Gdiplus::Graphics object is null" );
    qwr::QwrException::ExpectTrue( font, "font argument is null" );

    WrapGdiCalls( pGraphics, [&]( HDC dc )
    {
        CComPtr<GdiTextRenderer> renderer = new GdiTextRenderer( dc );
        CComPtr<IDWriteTextFormat> textFormat = renderer->GetTextFormat( font->HFont() );

        std::wstring out = text;

        if ( flags & DT_SINGLELINE )
        {
            static const std::wregex singleline_re( L"[\n\r]+", std::regex::optimize | std::regex::extended );
            out = std::regex_replace( out, singleline_re, L"" );
        }

        UINT32 prefixpos = -1;
        if ( !( flags & DT_NOPREFIX ) )
        {
            static const std::wregex prefixconv_re( L"&|&&", std::regex::optimize | std::regex::extended );
            const std::wstring in = out;
            out.clear();
            std::wsmatch match;
            std::wstring::const_iterator begin = in.begin();

            while ( std::regex_search( begin, in.end(), match, prefixconv_re ) )
            {
                out.append( match.prefix() );

                switch ( match.str().length() )
                {
                case 1:
                    prefixpos = out.length();
                    break;
                case 2:
                    out.push_back( L'&' );
                    break;
                default:
                    break;
                }

                begin = match[0].second;
            }

            out.append( begin, in.end() );
        }

        // get layout
        CComPtr<IDWriteTextLayout> textLayout = renderer->GetTextLayout( textFormat, out, rectW, rectH );

        DWRITE_TEXT_METRICS textMetrics;
        textLayout->GetMetrics( &textMetrics );

        // halign
        // DT_LEFT is 0 (default)
        // if ( ( ( flags & DT_LEFT ) && !( flags & DT_RTLREADING ) )
        //     || (! ( flags & DT_LEFT ) && ( flags & DT_RTLREADING ) ) )
            textLayout->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_LEADING );

        if ( ( ( flags & DT_RIGHT ) && !( flags & DT_RTLREADING ) )
             || ( !( flags & DT_RIGHT ) && ( flags & DT_RTLREADING ) ) )
            textLayout->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_TRAILING );

        if ( flags & DT_CENTER )
            textLayout->SetTextAlignment( DWRITE_TEXT_ALIGNMENT_CENTER );

        // valign
        // DT_TOP is 0 (default)
        // if ( flags & DT_TOP )
            textLayout->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_NEAR );

        if ( ( flags & DT_BOTTOM ) && ( flags & ( DT_SINGLELINE | DT_CALCRECT ) ) )
            textLayout->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_FAR );

        if ( ( flags & DT_VCENTER ) && ( flags & ( DT_SINGLELINE | DT_CALCRECT ) ) )
            textLayout->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );

        // trim
        if ( !( flags & DT_NOCLIP ) )
        {
            DWRITE_TRIMMING dwTrim{ DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
            CComPtr<IDWriteInlineObject> trimSymbol = nullptr;

            if ( flags & DT_WORDBREAK )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;

            if ( flags & DT_END_ELLIPSIS )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;

            if ( flags & DT_WORD_ELLIPSIS )
                dwTrim.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;

            if ( flags & DT_PATH_ELLIPSIS )
            {
                dwTrim.granularity    = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
                dwTrim.delimiter      = '\\';
                dwTrim.delimiterCount = 1;
            }

            if ( flags & ( DT_END_ELLIPSIS | DT_WORD_ELLIPSIS | DT_PATH_ELLIPSIS ) )
                trimSymbol = renderer->CreateEllipsisTrimmingSign( textFormat );

            textLayout->SetTrimming( &dwTrim, trimSymbol );
        }

        // fmts
        if ( flags & DT_RTLREADING )
            textLayout->SetReadingDirection( DWRITE_READING_DIRECTION_RIGHT_TO_LEFT );

        textLayout->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

        if ( flags & DT_WORDBREAK )
            textLayout->SetWordWrapping( DWRITE_WORD_WRAPPING_WRAP );

        if ( !( flags & DT_HIDEPREFIX ) && prefixpos >= 0 )
            textLayout->SetUnderline( true, { prefixpos, 1 } );

        CComQIPtr<IDWriteTextLayout2> textLayout2( textLayout );
        textLayout2->SetLastLineWrapping( false );

        if ( font->get_Underline () )
            textLayout->SetUnderline( true, { 0, text.length() } );

        if ( font->get_Strikeout() )
            textLayout->SetStrikethrough( true, { 0, text.length() } );

        if (flags & DT_EDITCONTROL)
            textLayout->SetMaxHeight( std::floorf( textLayout->GetMaxHeight() / font->get_Height() ) * font->get_Height() );

        // render
        renderer->RenderLayout( textLayout, colour, rectX, rectY, flags & DT_NOCLIP );
    } );
}

void JsGdiGraphics::WriteTextWithOpt( size_t optArgCount,
                                      const std::wstring& text, JsGdiFont* font, uint32_t colour,
                                      float rectX, float rectY, float rectW, float rectH,
                                      uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return WriteText( text, font, colour, rectX, rectY, rectW, rectH, flags );
    case 1:
        return WriteText( text, font, colour, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiGraphics::GetRoundRectPath( Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect,
                                      float arcWidth, float arcHeight ) const
{
    const float arcDiaW = arcWidth * 2;
    const float arcDiaH = arcHeight * 2;
    Gdiplus::RectF corner{ rect.X, rect.Y, arcDiaW, arcDiaH };

    qwr::error::CheckGdi( path.Reset(), "Reset" );

    // top left
    qwr::error::CheckGdi( path.AddArc( corner, 180, 90 ), "AddArc" );

    // top right
    corner.X += ( rect.Width - arcDiaW );
    qwr::error::CheckGdi( path.AddArc( corner, 270, 90 ), "AddArc" );

    // bottom right
    corner.Y += ( rect.Height - arcDiaH );
    qwr::error::CheckGdi( path.AddArc( corner, 0, 90 ), "AddArc" );

    // bottom left
    corner.X -= ( rect.Width - arcDiaW );
    qwr::error::CheckGdi( path.AddArc( corner, 90, 90 ), "AddArc" );
    qwr::error::CheckGdi( path.CloseFigure(), "CloseFigure" );
}

Gdiplus::StringFormat* JsGdiGraphics::UnpackStringFormat( uint32_t flags )
{
    Gdiplus::StringFormat* format = Gdiplus::StringFormat::GenericTypographic()->Clone();

    if ( flags != 0 )
    {
        format->SetAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 28 ) & 0x3 ) );     //0xf0000000
        format->SetLineAlignment( static_cast<Gdiplus::StringAlignment>( ( flags >> 24 ) & 0x3 ) ); //0x0f000000
        format->SetTrimming( static_cast<Gdiplus::StringTrimming>( ( flags >> 20 ) & 0x7 ) );       //0x00f00000
        format->SetFormatFlags( static_cast<Gdiplus::StringFormatFlags>( flags & 0x7FFF ) );        //0x0000ffff
    }

    return format;
}

void JsGdiGraphics::ParsePoints( JS::HandleValue jsValue, std::vector<Gdiplus::PointF>& gdiPoints )
{
    bool isX = true;
    float x = 0.0;

    auto pointParser = [&]( float coordinate )
    {
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

    qwr::QwrException::ExpectTrue( isX, "Coordinate count must be a multiple of two - Missing one half of a coordinate pair" );
}

} // namespace mozjs
