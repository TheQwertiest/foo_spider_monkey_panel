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

    bool DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour );
    bool DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour );
    
    bool DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour );
    bool DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour );
    bool FillEllipse( float x, float y, float w, float h, uint32_t colour );
    bool FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus );
       
    bool FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour );
    bool FillSolidRect( float x, float y, float w, float h, uint32_t colour );

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
