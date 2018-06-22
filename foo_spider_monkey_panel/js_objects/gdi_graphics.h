#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

class JsGdiGraphics
{
public:
    ~JsGdiGraphics();
    
    static JSObject* Create( JSContext* cx, JS::HandleObject global );

    void SetGraphicsObject( Gdiplus::Graphics* graphics );

public: // TODO: Move to private
    bool DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour );

    //CalcTextHeight( BSTR str, IGdiFont* font, UINT* p );
    //CalcTextWidth( BSTR str, IGdiFont* font, UINT* p );
    //DrawEllipse( float x, float y, float w, float h, float line_width, VARIANT colour );
    //DrawImage( IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha );
    //DrawLine( float x1, float y1, float x2, float y2, float line_width, VARIANT colour );
    //DrawPolygon( VARIANT colour, float line_width, VARIANT points );    
    //DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT colour );
    //DrawString( BSTR str, IGdiFont* font, VARIANT colour, float x, float y, float w, float h, int flags );
    //EstimateLineWrap( BSTR str, IGdiFont* font, int max_width, VARIANT* p );
    //FillEllipse( float x, float y, float w, float h, VARIANT colour );
    //FillGradRect( float x, float y, float w, float h, float angle, VARIANT colour1, VARIANT colour2, float focus );
    //FillPolygon( VARIANT colour, int fillmode, VARIANT points );
    //FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, VARIANT colour );
    //FillSolidRect( float x, float y, float w, float h, VARIANT colour );
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
    JsGdiGraphics( const JsGdiGraphics& );

    static void GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height );

private:
    JSContext * pJsCtx_;
    std::unique_ptr<Gdiplus::Graphics> graphics_;
};

}
