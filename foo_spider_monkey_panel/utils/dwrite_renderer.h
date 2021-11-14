#pragma once

#include <dwrite_3.h>

namespace smp::dwrite
{
#define DW_FLIPARGB( color ) ( color & 0xff00ff00 | ( color & 0x00ff0000 ) >> 16 | ( color & 0x000000ff ) << 16 )
#define DW_COLOR_CH( chann ) ( (BYTE)std::clamp( chann * 255.0f, 0.0f, 255.0f ) )
#define DW_COLORREF( color ) ( RGB( DW_COLOR_CH( color.r ), DW_COLOR_CH( color.g ), DW_COLOR_CH( color.b ) ) )

class GdiTextRenderer
    : public IDWriteTextRenderer1
{

public:
    GdiTextRenderer( HDC hdc, bool pixelSnapping = true );
    static CComPtr<IDWriteFactory3> DWriteFactory();
    static CComPtr<IDWriteGdiInterop> GdiInterop();
    static CComPtr<IDWriteFontCollection> SystemFontCollection();

private:
    CComPtr<IDWriteRenderingParams2> RenderingParams();
    CComPtr<IDWriteBitmapRenderTarget> RenderTarget();
    HDC RenderDC();

public: // GdiTextRenderer
    CComPtr<IDWriteTextFormat> GetTextFormat
    (
        HFONT hfont
    );

    CComPtr<IDWriteInlineObject> CreateEllipsisTrimmingSign
    (
        CComPtr<IDWriteTextFormat> textFormat
    );

    CComPtr<IDWriteTextLayout> GetTextLayout
    (
        CComPtr<IDWriteTextFormat> textFormat,
        const std::wstring& text,
        FLOAT w, FLOAT h
    );

    void RenderLayout
    (
        CComPtr<IDWriteTextLayout> textLayout,
        UINT32 color,
        FLOAT x, FLOAT y,
        BOOL noclip
    );

private: // IUnknown
    DWORD refCount = 0;

private: // GdiTextRerender
    CComPtr<IDWriteRenderingParams2> renderingParams = nullptr;
    CComPtr<IDWriteBitmapRenderTarget> renderTarget = nullptr;

    HDC targetDC = nullptr;
    HDC renderDC = nullptr;
    SIZE size = {};

    COLORREF renderColor = 0xff000000;
    BYTE renderAlpha = 0xff;

    bool enablePixelSnapping = true;

public: // IUnknown
    IFACEMETHOD_( DWORD, AddRef ) ();
    IFACEMETHOD_( DWORD, Release ) ();
    IFACEMETHOD( QueryInterface )
    (
        REFIID riid,
        _COM_Outptr_ void** ppvObject
    );

public:// IDWritePixelSnapping
    IFACEMETHOD( IsPixelSnappingDisabled )
    (
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
    );

    IFACEMETHOD( GetCurrentTransform )
    (
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
    );

    IFACEMETHOD( GetPixelsPerDip )
    (
        _In_opt_ void* clientDrawingContext,
        _Out_ FLOAT* pixelsPerDip
    );

public: // IDWriteTextRenderer
    IFACEMETHOD( DrawGlyphRun )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_opt_ IUnknown* clientDrawingEffect
    );

    IFACEMETHOD( DrawUnderline )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_opt_ IUnknown* clientDrawingEffect
    );

    IFACEMETHOD( DrawStrikethrough )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
    );

    IFACEMETHOD( DrawInlineObject )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
    );

public: // IDWriteTextRenderer1
    IFACEMETHOD( DrawGlyphRun )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_opt_ IUnknown* clientDrawingEffect
    );

    IFACEMETHOD( DrawUnderline )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_opt_ IUnknown* clientDrawingEffect
    );

    IFACEMETHOD( DrawStrikethrough )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
    );

    IFACEMETHOD( DrawInlineObject )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
        _In_ IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
    );

private:
    STDMETHOD( GetColorGlyphEnumerator )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GLYPH_RUN const* glyphRun,
        DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _COM_Outptr_ IDWriteColorGlyphRunEnumerator** colorGlyphRunEnumerator
    );

    STDMETHOD( RecommendRenderingParams )
    (
        _In_opt_ void* clientDrawingContext,
        DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GLYPH_RUN const* glyphRun,
        DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _COM_Outptr_ IDWriteRenderingParams2** recommendedRenderParams
    );

    STDMETHOD( DrawLine )
    (
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
        FLOAT length,
        FLOAT thickness,
        FLOAT offset,
        _In_opt_ IUnknown* clientDrawingEffectS
    );

private:
    class GdiTextTransform : public DWRITE_MATRIX
    {
    public:
        void Multiply( const GdiTextTransform* other );
    };

private:
    class GdiTextOrientation
    {
    public:
        GdiTextOrientation(
            _In_ CComPtr<IDWriteBitmapRenderTarget> renderTarget,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle = DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,
            BOOL isSideWays = false );

        ~GdiTextOrientation();

    private:
        CComPtr<IDWriteBitmapRenderTarget> restoreTarget = nullptr;
        GdiTextTransform originalTransform = {};
    };
};

}
