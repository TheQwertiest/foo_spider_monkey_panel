#pragma once

#include <GdiPlus.h>

namespace sm_core
{

class GdiGraphics : public Gdiplus::Graphics
{
protected:
    GdiGraphics();
    static void GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height );

public:
    STDMETHODIMP CalcTextHeight( BSTR str, IGdiFont* font, UINT* p );
    STDMETHODIMP CalcTextWidth( BSTR str, IGdiFont* font, UINT* p );
    STDMETHODIMP DrawEllipse( float x, float y, float w, float h, float line_width, VARIANT colour );
    STDMETHODIMP DrawImage( IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha );
    STDMETHODIMP DrawLine( float x1, float y1, float x2, float y2, float line_width, VARIANT colour );
    STDMETHODIMP DrawPolygon( VARIANT colour, float line_width, VARIANT points );
    STDMETHODIMP DrawRect( float x, float y, float w, float h, float line_width, VARIANT colour );
    STDMETHODIMP DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT colour );
    STDMETHODIMP DrawString( BSTR str, IGdiFont* font, VARIANT colour, float x, float y, float w, float h, int flags );
    STDMETHODIMP EstimateLineWrap( BSTR str, IGdiFont* font, int max_width, VARIANT* p );
    STDMETHODIMP FillEllipse( float x, float y, float w, float h, VARIANT colour );
    STDMETHODIMP FillGradRect( float x, float y, float w, float h, float angle, VARIANT colour1, VARIANT colour2, float focus );
    STDMETHODIMP FillPolygon( VARIANT colour, int fillmode, VARIANT points );
    STDMETHODIMP FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, VARIANT colour );
    STDMETHODIMP FillSolidRect( float x, float y, float w, float h, VARIANT colour );
    STDMETHODIMP GdiAlphaBlend( IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha );
    STDMETHODIMP GdiDrawBitmap( IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH );
    STDMETHODIMP GdiDrawText( BSTR str, IGdiFont* font, VARIANT colour, int x, int y, int w, int h, int format, VARIANT* p );
    STDMETHODIMP MeasureString( BSTR str, IGdiFont* font, float x, float y, float w, float h, int flags, IMeasureStringInfo** pp );
    STDMETHODIMP SetInterpolationMode( int mode );
    STDMETHODIMP SetSmoothingMode( int mode );
    STDMETHODIMP SetTextRenderingHint( UINT mode );
    STDMETHODIMP put__ptr( void* p );
};

}