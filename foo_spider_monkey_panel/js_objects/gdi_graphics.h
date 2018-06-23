#pragma once

#include <js_engine/js_error_codes.h>


class JSObject;
struct JSContext;

namespace mozjs
{

class JsGdiGraphics
{
public:
    ~JsGdiGraphics();
    
    static JSObject* Create( JSContext* cx );

    void SetGraphicsObject( Gdiplus::Graphics* graphics );

public: // TODO: Move to private

    Mjs_Status DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour );
    Mjs_Status DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour );
    
    Mjs_Status DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour );
    Mjs_Status DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour );
    Mjs_Status FillEllipse( float x, float y, float w, float h, uint32_t colour );
    Mjs_Status FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus );
       
    Mjs_Status FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour );
    Mjs_Status FillSolidRect( float x, float y, float w, float h, uint32_t colour );

    //bool DrawPolygon( uint32_t colour, float line_width, VARIANT points );
    //bool FillPolygon( uint32_t colour, int fillmode, VARIANT points );

    //CalcTextHeight( BSTR str, IGdiFont* font, UINT* p );
    //CalcTextWidth( BSTR str, IGdiFont* font, UINT* p );
    
    //DrawImage( IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha );
    
    
    //DrawString( BSTR str, IGdiFont* font, VARIANT colour, float x, float y, float w, float h, int flags );
    //EstimateLineWrap( BSTR str, IGdiFont* font, int max_width, VARIANT* p );
    
    //GdiAlphaBlend( IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha );
    //GdiDrawBitmap( IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH );
    //GdiDrawText( BSTR str, IGdiFont* font, VARIANT colour, int x, int y, int w, int h, int format, VARIANT* p );
    //MeasureString( BSTR str, IGdiFont* font, float x, float y, float w, float h, int flags, IMeasureStringInfo** pp );
    //SetInterpolationMode( int mode );
    //SetSmoothingMode( int mode );
    //SetTextRenderingHint( UINT mode );
    //put__ptr( void* p );

private:
    JsGdiGraphics( JSContext* cx );
    JsGdiGraphics( const JsGdiGraphics& ) = delete;

    static int GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height );

private:
    JSContext * pJsCtx_;
    Gdiplus::Graphics* graphics_;
};

}
