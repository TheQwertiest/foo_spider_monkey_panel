#include <stdafx.h>
#include <qwr/winapi_error_helpers.h>

#include <utils/gdi_helpers.h>

#include "dwrite_renderer.h"

namespace smp::dwrite
{

GdiTextRenderer::GdiTextRenderer( HDC hdc, bool pixelSnapping )
    : targetDC (hdc)
    , enablePixelSnapping( pixelSnapping)
{
}

CComPtr<IDWriteFactory3> GdiTextRenderer::DWriteFactory()
{
    static CComPtr<IDWriteFactory3> dwriteFactory = nullptr;

    if ( !dwriteFactory )
    {
        qwr::error::CheckHR( DWriteCreateFactory
        (
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof( IDWriteFactory3 ),
            reinterpret_cast<IUnknown**>( &dwriteFactory )
        ), "DWriteCreateFactory" );
    }

    return dwriteFactory;
}

CComPtr<IDWriteGdiInterop> GdiTextRenderer::GdiInterop()
{
    static CComPtr<IDWriteGdiInterop> gdiInterop = nullptr;

    if ( !gdiInterop )
    {
        qwr::error::CheckHR( DWriteFactory()->GetGdiInterop
        (
            &gdiInterop
        ), "GetGdiInterop" );
    }

    return gdiInterop;
}

CComPtr<IDWriteFontCollection> GdiTextRenderer::SystemFontCollection()
{
    static CComPtr<IDWriteFontCollection> systemFontCollection = nullptr;

    if ( !systemFontCollection )
    {
        qwr::error::CheckHR( DWriteFactory()->GetSystemFontCollection
        (
            &systemFontCollection, TRUE
        ), "GetSystemFontCollection" );
    }

    return systemFontCollection;
}

CComPtr<IDWriteRenderingParams2> GdiTextRenderer::RenderingParams()
{
    if ( !renderingParams )
    {
        CComPtr<IDWriteRenderingParams> displayParams = nullptr;
        qwr::error::CheckHR( DWriteFactory()->CreateMonitorRenderingParams
        (
            MonitorFromWindow( WindowFromDC( targetDC ), MONITOR_DEFAULTTOPRIMARY ),
            &displayParams
        ), "CreateMonitorRenderingParams" );

        CComQIPtr<IDWriteRenderingParams2> displayParams2( displayParams );

        qwr::error::CheckHR( DWriteFactory()->CreateCustomRenderingParams
        (
            displayParams->GetGamma(),
            displayParams->GetEnhancedContrast(),
            displayParams2 ? displayParams2->GetGrayscaleEnhancedContrast() : displayParams->GetEnhancedContrast(),
            displayParams->GetClearTypeLevel(),
            displayParams->GetPixelGeometry(),
            DWRITE_RENDERING_MODE_GDI_CLASSIC,
            enablePixelSnapping
                ? DWRITE_GRID_FIT_MODE_ENABLED
                : DWRITE_GRID_FIT_MODE_DISABLED,
            &renderingParams
        ), "CreateCustomRenderingParams");
    }

    return renderingParams;
}

CComPtr<IDWriteBitmapRenderTarget> GdiTextRenderer::RenderTarget()
{
    if ( !renderTarget )
    {
        qwr::error::CheckHR( GdiInterop()->CreateBitmapRenderTarget
        (
            targetDC, 1,  1,
            &renderTarget
        ), "CreateBitmapRenderTarget" );
    }

    return renderTarget;
}

inline HDC GdiTextRenderer::RenderDC()
{
    return renderDC ? renderDC : renderDC = RenderTarget()->GetMemoryDC();
}

#define USE_LOCALIZED_FONT_FAMILY_NAMES 1

CComPtr<IDWriteTextFormat> GdiTextRenderer::GetTextFormat( HFONT hfont )
{
    CComPtr<IDWriteFontFace> face = nullptr;
    TEXTMETRICW textmetric;

    [&]
    {
        gdi::ObjectSelector autoFont( targetDC, hfont );
        qwr::error::CheckHR( GdiInterop()->CreateFontFaceFromHdc( targetDC, &face ), "CreateFontFaceFromHdc" );
        GetTextMetricsW( targetDC, &textmetric );
    }();

    CComPtr<IDWriteFontFace3> face3;
    qwr::error::CheckHR( face.QueryInterface( &face3 ), "IDWriteFontFace::QueryInterface<IDWriteFontFace3>" );

#if USE_LOCALIZED_FONT_FAMILY_NAMES
    CComPtr<IDWriteLocalizedStrings> familyNames = nullptr;
    HRESULT hr = face3->GetFamilyNames( &familyNames );
    qwr::error::CheckHR( hr, "GetFamilyNames" );

    UINT32 index = 0;
    BOOL found = false;
    WCHAR defaultLocale[LOCALE_NAME_MAX_LENGTH];

    if ( GetUserDefaultLocaleName( defaultLocale, LOCALE_NAME_MAX_LENGTH ) )
        hr = familyNames->FindLocaleName( defaultLocale, &index, &found );

    if ( SUCCEEDED( hr ) && !found && GetSystemDefaultLocaleName( defaultLocale, LOCALE_NAME_MAX_LENGTH ) )
        hr = familyNames->FindLocaleName( defaultLocale, &index, &found );

    if ( SUCCEEDED( hr ) && !found )
        hr = familyNames->FindLocaleName( LOCALE_NAME_INVARIANT, &index, &found );

    if ( !found )
        index = 0;

    UINT32 familyNameLength = 0;
    qwr::error::CheckHR( familyNames->GetStringLength( index, &familyNameLength ), "GetStringLength" );
    WCHAR* familyName = new ( std::nothrow ) WCHAR[familyNameLength + 1];
    qwr::error::CheckHR( familyName  ? S_OK : E_OUTOFMEMORY, "AllocString" );
    qwr::error::CheckHR( familyNames->GetString( index, familyName, familyNameLength + 1 ), "GetString" );

    UINT32 localeNameLength = 0;
    qwr::error::CheckHR( familyNames->GetLocaleNameLength( index, &localeNameLength ), "GetLocaleNameLength" );
    WCHAR* localeName = new ( std::nothrow ) WCHAR[localeNameLength + 1];
    qwr::error::CheckHR( localeName ? S_OK : E_OUTOFMEMORY, "AllocLocaleName" );
    qwr::error::CheckHR( familyNames->GetLocaleName( index, localeName, localeNameLength + 1 ), "GetLocaleName" );
#else
    LOGFONTW logfont;
    GdiInterop()->ConvertFontFaceToLOGFONT( face, &logfont );

    const WCHAR* familyName = logfont.lfFaceName;
    const WCHAR* localeName = LOCALE_NAME_INVARIANT;
#endif

    CComPtr<IDWriteTextFormat> textFormat = nullptr;
    qwr::error::CheckHR( DWriteFactory()->CreateTextFormat
    (
        familyName,
        SystemFontCollection(),
        face3->GetWeight(),
        face3->GetStyle(),
        face3->GetStretch(),
        static_cast<FLOAT>(textmetric.tmHeight - textmetric.tmInternalLeading),
        localeName,
        &textFormat
    ), "CreateTextFormat" );

    return textFormat;
}

CComPtr<IDWriteTextLayout> GdiTextRenderer::GetTextLayout
(
    CComPtr<IDWriteTextFormat> textFormat,
    const std::wstring& text,
    FLOAT w, FLOAT h
)
{
    DWRITE_RENDERING_MODE renderingMode = RenderingParams()->GetRenderingMode();

    CComPtr<IDWriteTextLayout> textLayout = nullptr;
    qwr::error::CheckHR( DWriteFactory()->CreateGdiCompatibleTextLayout
    (
        text.c_str(),
        text.length(),
        textFormat,
        w, h,
        RenderTarget()->GetPixelsPerDip(),
        NULL, //&transform,
        renderingMode == DWRITE_RENDERING_MODE_GDI_NATURAL
            || renderingMode == DWRITE_RENDERING_MODE_NATURAL
            || renderingMode == DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
        &textLayout
    ), "CreateGdiCompatibleTextLayout");

    return textLayout;
}

CComPtr<IDWriteInlineObject> GdiTextRenderer::CreateEllipsisTrimmingSign
(
    CComPtr<IDWriteTextFormat> textFormat
)
{
    CComPtr<IDWriteInlineObject> ellipsisTrimmingSign = nullptr;
    qwr::error::CheckHR( DWriteFactory()->CreateEllipsisTrimmingSign
    (
        textFormat,
        &ellipsisTrimmingSign
    ), "CreateEllipsisTrimmingSign" );

    return ellipsisTrimmingSign;
}

using Gdiplus::ARGB;

void GdiTextRenderer::RenderLayout
(
    CComPtr<IDWriteTextLayout> textLayout,
    UINT32 color,
    FLOAT x, FLOAT y,
    BOOL noclip
)
{
    renderColor = static_cast<COLORREF>( color & ~ALPHA_MASK );
    renderAlpha = static_cast<BYTE>( ( color & ALPHA_MASK ) >> ALPHA_SHIFT );

    DWRITE_TEXT_METRICS textMetrics;
    qwr::error::CheckHR( textLayout->GetMetrics( &textMetrics ), "GetMetrics" );

    POINT origin =
    {
        static_cast<LONG>( x + textMetrics.left ),
        static_cast<LONG>( y + textMetrics.top  )
    };

    SIZE bounds =
    {
        1 + static_cast<LONG>( ( noclip ? textMetrics.widthIncludingTrailingWhitespace : std::min( textMetrics.widthIncludingTrailingWhitespace, textMetrics.layoutWidth ) ) ),
        1 + static_cast<LONG>( ( noclip ? textMetrics.height                           : std::min( textMetrics.height, textMetrics.layoutHeight ) ) )
    };

    SIZE size;
    qwr::error::CheckHR( RenderTarget()->GetSize( &size ), "GetSize" );

    if ( ( size.cx < bounds.cx ) || ( size.cy < bounds.cy ) )
        qwr::error::CheckHR( RenderTarget()->Resize( bounds.cx, bounds.cy ), fmt::format ( "Resize {}x{}->{}x{} @ {},{}", size.cx, size.cy, bounds.cy, bounds.cy, origin.x, origin.y ) );

    // copy background to renderDC
    qwr::error::CheckWinApi (BitBlt
    (
        RenderDC(),      0,        0, bounds.cx, bounds.cy,
        targetDC, origin.x, origin.y,
        SRCCOPY | NOMIRRORBITMAP
    ), "BitBlt->renderDC");

    // draw text over
    qwr::error::CheckHR( textLayout->Draw( nullptr, this, -textMetrics.left, -textMetrics.top ), "Draw" );

    // Blit / Blend back to targetDC
    if (renderAlpha == 0xff)
    {
        qwr::error::CheckWinApi( BitBlt
        (
            targetDC, origin.x, origin.y, bounds.cx, bounds.cy,
            RenderDC(),      0,        0,
            SRCCOPY | NOMIRRORBITMAP
        ), "BitBlt->targetDC" );
    }

    qwr::error::CheckWinApi( AlphaBlend
    (
        targetDC, origin.x, origin.y, bounds.cx, bounds.cy,
        RenderDC(),      0,        0, bounds.cx, bounds.cy,
        { AC_SRC_OVER, 0, renderAlpha, AC_SRC_OVER }
    ), fmt::format( "AlphaBlend {}x{} @ {},{}", bounds.cx, bounds.cy, origin.x, origin.y ) );
}

// HRESULT helper macro-s
#ifndef IFRH // if FAILED return the result
#define IFRH( _expr_ )         { HRESULT _result_ = ( _expr_ ); if ( FAILED( _result_ ) ) { return _result_; } }
#endif

#ifndef IFRC // if FAILED return code
#define IFRC( _expr_, _code_ ) { HRESULT _result_ = ( _expr_ ); if ( FAILED( _result_ ) ) { return _code_; } }
#endif

#ifndef IFRT // if FAILED() return
#define IFRT( _expr_ )         { HRESULT _result_ = ( _expr_ ); if ( FAILED( _result_ ) ) { return; } }
#endif

// IFace Methods

// IUnknown
IFACEMETHODIMP
GdiTextRenderer::QueryInterface
(
    REFIID riid,
    _COM_Outptr_ void** ppvObject
) noexcept
{
    if
    (
        riid == __uuidof( IUnknown             ) ||
        riid == __uuidof( IDWritePixelSnapping ) ||
        riid == __uuidof( IDWriteTextRenderer  ) ||
        riid == __uuidof( IDWriteTextRenderer1 )
    )
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

// IUnknown
IFACEMETHODIMP_( DWORD )
GdiTextRenderer::AddRef()
{
    return ( ++refCount );
}

// IUnknown
IFACEMETHODIMP_( DWORD )
GdiTextRenderer::Release()
{
    DWORD newCount = ( --refCount );

    if ( newCount == 0 )
        delete this;

    return newCount;
}

// IDWritePixelSnapping
IFACEMETHODIMP GdiTextRenderer::IsPixelSnappingDisabled
(
    _In_opt_ void* clientDrawingContext,
    _Out_ BOOL* isDisabled
)
{
    *isDisabled = !enablePixelSnapping;
    return S_OK;
}

// IDWritePixelSnapping
IFACEMETHODIMP GdiTextRenderer::GetCurrentTransform
(
    _In_opt_ void* clientDrawingContext,
    _Out_ DWRITE_MATRIX* transform
)
{
    assert( renderTarget );
    return renderTarget->GetCurrentTransform( transform );
}

// IDWritePixelSnapping
IFACEMETHODIMP GdiTextRenderer::GetPixelsPerDip
(
    _In_opt_ void* clientDrawingContext,
    _Out_ FLOAT* pixelsPerDip
)
{
    assert( renderTarget );
    *pixelsPerDip = renderTarget->GetPixelsPerDip();
    return S_OK;
}

// IDWriteTextRenderer1 method
IFACEMETHODIMP GdiTextRenderer::DrawGlyphRun
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    _In_ DWRITE_GLYPH_RUN const* glyphRun,
    _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    return DrawGlyphRun
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,
        measuringMode,
        glyphRun,
        glyphRunDescription,
        clientDrawingEffect
    );
}

// IDWriteTextRenderer1 method
IFACEMETHODIMP GdiTextRenderer::DrawGlyphRun
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
    DWRITE_MEASURING_MODE measuringMode,
    _In_ DWRITE_GLYPH_RUN const* glyphRun,
    _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    assert( renderTarget );

    COLORREF drawColor = DW_FLIPARGB( renderColor );

    HRESULT hr = S_OK;

    if ( glyphRun->glyphCount <= 0 )
        return hr;

    GdiTextOrientation orientation
    (
        renderTarget,
        baselineOriginX, baselineOriginY,
        glyphOrientationAngle, glyphRun->isSideways
    );

    CComPtr<IDWriteRenderingParams2> recommendedParams = nullptr;
    IFRH( RecommendRenderingParams
    (
        clientDrawingContext,
        measuringMode,
        glyphRun,
        glyphRunDescription,
        &recommendedParams
    ) );

    CComPtr<IDWriteColorGlyphRunEnumerator> colorGlyphRunEnumerator = nullptr;
    hr = GetColorGlyphEnumerator
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        measuringMode,
        glyphRun,
        glyphRunDescription,
        &colorGlyphRunEnumerator
    );


    if ( hr == DWRITE_E_NOCOLOR )
    {
        hr = renderTarget->DrawGlyphRun
        (
            baselineOriginX,
            baselineOriginY,
            measuringMode,
            glyphRun,
            recommendedParams ? recommendedParams : renderingParams,
            drawColor
        );

        return hr;
    }

    IFRH( hr );

    while (true)
    {
        BOOL haveRun;
        IFRH( colorGlyphRunEnumerator->MoveNext( &haveRun ) );
        if ( !haveRun )
            break;

        DWRITE_COLOR_GLYPH_RUN const* colorGlyphRun = nullptr;
        IFRH( colorGlyphRunEnumerator->GetCurrentRun( &colorGlyphRun ) );

        IFRH( renderTarget->DrawGlyphRun
        (
            colorGlyphRun->baselineOriginX,
            colorGlyphRun->baselineOriginY,
            measuringMode,
            &colorGlyphRun->glyphRun,
            recommendedParams ? recommendedParams : renderingParams,
            colorGlyphRun->paletteIndex == 0xffff ? drawColor : DW_COLORREF( colorGlyphRun->runColor )
        ) );
    }

    return S_OK;
}

// private
STDMETHODIMP GdiTextRenderer::GetColorGlyphEnumerator
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    _COM_Outptr_ IDWriteColorGlyphRunEnumerator** colorGlyphRunEnumerator
)
{
    *colorGlyphRunEnumerator = nullptr;

    CComPtr<IDWriteFontFace3> font;
    IFRC( glyphRun->fontFace->QueryInterface( &font ), DWRITE_E_NOCOLOR );

    if ( !font->GetColorPaletteCount() )
        return DWRITE_E_NOCOLOR;

    DWRITE_MATRIX transform;
    IFRH( GetCurrentTransform( clientDrawingContext, &transform ) );

    return DWriteFactory()->TranslateColorGlyphRun
    (
        baselineOriginX,
        baselineOriginY,
        glyphRun,
        glyphRunDescription,
        measuringMode,
        &transform,
        0,
        colorGlyphRunEnumerator
    );
}

STDMETHODIMP GdiTextRenderer::RecommendRenderingParams
(
    _In_opt_ void* clientDrawingContext,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    _COM_Outptr_ IDWriteRenderingParams2** recommendRenderingParams
)
{
    assert( renderingParams );
    assert( renderTarget );

    *recommendRenderingParams = nullptr;

    CComPtr<IDWriteFontFace3> font;
    IFRC( glyphRun->fontFace->QueryInterface( &font ), DWRITE_E_NOCOLOR );

    DWRITE_MATRIX transform;
    IFRH( GetCurrentTransform( clientDrawingContext, &transform ) );

    FLOAT dpi = renderTarget->GetPixelsPerDip() * 96.0f;

    DWRITE_RENDERING_MODE recommendRenderingMode;
    DWRITE_GRID_FIT_MODE recommendGridFitMode;

    IFRH (font->GetRecommendedRenderingMode
    (
        glyphRun->fontEmSize,
        dpi,
        dpi,
        &transform,
        glyphRun->isSideways,
        DWRITE_OUTLINE_THRESHOLD_ANTIALIASED,
        measuringMode,
        renderingParams,
        &recommendRenderingMode,
        &recommendGridFitMode
    ) );

    return DWriteFactory()->CreateCustomRenderingParams
    (
        renderingParams->GetGamma(),
        renderingParams->GetEnhancedContrast(),
        renderingParams->GetGrayscaleEnhancedContrast(),
        renderingParams->GetClearTypeLevel(),
        renderingParams->GetPixelGeometry(),
        recommendRenderingMode,
        recommendGridFitMode,
        recommendRenderingParams
    );
}

// IDWriteTextRenderer method
IFACEMETHODIMP GdiTextRenderer::DrawUnderline
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ DWRITE_UNDERLINE const* underline,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    return DrawUnderline
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,
        underline,
        clientDrawingEffect
    );
}

// IDWriteTextRenderer1 method
IFACEMETHODIMP GdiTextRenderer::DrawUnderline
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
    _In_ DWRITE_UNDERLINE const* underline,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    return DrawLine
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        glyphOrientationAngle,
        underline->width,
        underline->thickness,
        underline->offset,
        clientDrawingEffect
    );
}

// IDWriteTextRenderer method
IFACEMETHODIMP GdiTextRenderer::DrawStrikethrough
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    return DrawStrikethrough
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,
        strikethrough,
        clientDrawingEffect
    );
}

// IDWriteTextRenderer1 method
IFACEMETHODIMP GdiTextRenderer::DrawStrikethrough
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    return DrawLine
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        glyphOrientationAngle,
        strikethrough->width,
        strikethrough->thickness,
        strikethrough->offset,
        clientDrawingEffect
    );
}

// IDWriteTextRenderer method
IFACEMETHODIMP GdiTextRenderer::DrawInlineObject
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ IDWriteInlineObject* inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    return DrawInlineObject
    (
        clientDrawingContext,
        baselineOriginX,
        baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,
        inlineObject,
        isSideways,
        isRightToLeft,
        clientDrawingEffect
    );
}

// IDWriteTextRenderer1 method
IFACEMETHODIMP GdiTextRenderer::DrawInlineObject
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
    _In_ IDWriteInlineObject* inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    assert( renderTarget );

    GdiTextOrientation orientation( renderTarget, baselineOriginX, baselineOriginY, glyphOrientationAngle, isSideways );

    return inlineObject->Draw
    (
        clientDrawingContext,
        this,
        baselineOriginX,
        baselineOriginY,
        isSideways,
        isRightToLeft,
        clientDrawingEffect
    );
}

// private
STDMETHODIMP GdiTextRenderer::DrawLine
(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
    FLOAT length,
    FLOAT thickness,
    FLOAT offset,
    _In_opt_ IUnknown* clientDrawingEffect
)
{
    assert( renderTarget );

    GdiTextOrientation orientation( renderTarget, baselineOriginX, baselineOriginY, glyphOrientationAngle );

    HGDIOBJ selected = SelectObject( RenderDC(), CreatePen( PS_SOLID, std::lround( thickness ), renderColor ) );

    MoveToEx( RenderDC(), std::lround( baselineOriginX ), std::lround( baselineOriginY + offset ), nullptr );
    LineTo( RenderDC(), std::lround( baselineOriginX + length ), std::lround( baselineOriginY + offset ) );

    DeleteObject( SelectObject( RenderDC(), selected ) );

    return S_OK;
}

// GdiTextTransform
void GdiTextRenderer::GdiTextTransform::Multiply( const GdiTextRenderer::GdiTextTransform *matrix)
{
    float _m11 = ( matrix->m11 * this->m11 ) + ( matrix->m12 * this->m21 );
    float _m12 = ( matrix->m11 * this->m12 ) + ( matrix->m12 * this->m22 );

    float _m21 = ( matrix->m21 * this->m11 ) + ( matrix->m22 * this->m21 );
    float _m22 = ( matrix->m21 * this->m12 ) + ( matrix->m22 * this->m22 );

    float _dx = ( matrix->dx * this->m11 ) + ( matrix->dy * this->m21 ) + this->dx;
    float _dy = ( matrix->dx * this->m12 ) + ( matrix->dy * this->m22 ) + this->dy;

    m11 = _m11; m12 = _m12; m21 = _m21; m22 = _m22; dx = _dx; dy = _dy;
}

// GdiTextOrientation
GdiTextRenderer::GdiTextOrientation::GdiTextOrientation
(
    _In_ CComPtr<IDWriteBitmapRenderTarget> renderTarget,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
    BOOL isSideWays
)
{
    if ( glyphOrientationAngle == DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES )
        return;

    CComPtr<IDWriteTextAnalyzer> textAnalyzer = nullptr;
    IFRT( DWriteFactory()->CreateTextAnalyzer( &textAnalyzer ) );

    CComPtr<IDWriteTextAnalyzer2> textAnalyzer2;
    IFRT( textAnalyzer.QueryInterface( &textAnalyzer2 ) );

    IFRT( renderTarget->GetCurrentTransform( &originalTransform ) );

    GdiTextTransform glyphOrientationTransform = {};
    IFRT( textAnalyzer2->GetGlyphOrientationTransform
    (
        glyphOrientationAngle,
        isSideWays,
        baselineOriginX,
        baselineOriginY,
        &glyphOrientationTransform
    ) );

    glyphOrientationTransform.Multiply( &originalTransform );

    IFRT( renderTarget->SetCurrentTransform( &glyphOrientationTransform ) );

    restoreTarget = renderTarget;
}

GdiTextRenderer::GdiTextOrientation::~GdiTextOrientation()
{
    if ( restoreTarget == nullptr )
        return;

    restoreTarget->SetCurrentTransform( &originalTransform );
}

}
