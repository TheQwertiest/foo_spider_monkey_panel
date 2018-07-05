#include <stdafx.h>
#include "gdi_bitmap.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <stackblur.h>
#include <kmeans.h>
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
    JsFinalizeOp<JsGdiBitmap>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiBitmap",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, ApplyAlpha )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, ApplyMask )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, Clone )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, CreateRawBitmap )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, GetColourScheme )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, GetColourSchemeJSON )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, GetGraphics )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, ReleaseGraphics )
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsGdiBitmap, Resize, ResizeWithOpt, 1 )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, RotateFlip )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, SaveAs )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, StackBlur )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "ApplyAlpha", ApplyAlpha, 1, DefaultPropsFlags() ),
    JS_FN( "ApplyMask", ApplyMask, 1, DefaultPropsFlags() ),
    JS_FN( "Clone", Clone, 4, DefaultPropsFlags() ),
    JS_FN( "CreateRawBitmap", CreateRawBitmap, 0, DefaultPropsFlags() ),
    JS_FN( "GetColourScheme", GetColourScheme, 0, DefaultPropsFlags() ),
    JS_FN( "GetColourSchemeJSON", GetColourSchemeJSON, 0, DefaultPropsFlags() ),
    JS_FN( "GetGraphics", GetGraphics, 0, DefaultPropsFlags() ),
    JS_FN( "ReleaseGraphics", ReleaseGraphics, 1, DefaultPropsFlags() ),
    JS_FN( "Resize", Resize, 3, DefaultPropsFlags() ),
    JS_FN( "RotateFlip", RotateFlip, 1, DefaultPropsFlags() ),
    JS_FN( "SaveAs", SaveAs, 2, DefaultPropsFlags() ),
    JS_FN( "StackBlur", StackBlur, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, get_Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiBitmap, get_Width )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Width",  get_Width, DefaultPropsFlags() ),
    JS_PS_END
};


}

namespace mozjs
{


JsGdiBitmap::JsGdiBitmap( JSContext* cx, Gdiplus::Bitmap* pGdiBitmap )
    : pJsCtx_( cx )
    , pGdi_( pGdiBitmap )
{
}


JsGdiBitmap::~JsGdiBitmap()
{
}

JSObject* JsGdiBitmap::Create( JSContext* cx, Gdiplus::Bitmap* pGdiBitmap )
{
    if ( !pGdiBitmap )
    {
        JS_ReportErrorASCII( cx, "Internal error: Gdiplus::Bitmap object is null" );
        return nullptr;
    }

    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiBitmap( cx, pGdiBitmap ) );

    return jsObj;
}

const JSClass& JsGdiBitmap::GetClass()
{
    return jsClass;
}

Gdiplus::Bitmap* JsGdiBitmap::GdiBitmap() const
{
    assert( pGdi_ );
    return pGdi_.get();
}

std::optional<std::uint32_t>
JsGdiBitmap::get_Height()
{
    assert( pGdi_ );
    return pGdi_->GetHeight();
}

std::optional<std::uint32_t>
JsGdiBitmap::get_Width()
{
    assert( pGdi_ );
    return pGdi_->GetWidth();
}

std::optional<JSObject*>
JsGdiBitmap::ApplyAlpha( uint8_t alpha )
{
    assert( pGdi_ );

    t_size width = pGdi_->GetWidth();
    t_size height = pGdi_->GetHeight();

    std::unique_ptr<Gdiplus::Bitmap> out( new Gdiplus::Bitmap( width, height, PixelFormat32bppPARGB ) );
    if ( !helpers::ensure_gdiplus_object( out.get() ) )
    {// TODO: replace with IF_FAILED macro
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create Gdiplus object" );
        return std::nullopt;
    }

    Gdiplus::Graphics g( out.get() );
    Gdiplus::ImageAttributes ia;
    Gdiplus::ColorMatrix cm = { 0.0 };
    Gdiplus::Rect rc;

    cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0;
    cm.m[3][3] = static_cast<float>( alpha ) / 255;
    Gdiplus::Status gdiRet = ia.SetColorMatrix( &cm );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, SetColorMatrix );

    rc.X = rc.Y = 0;
    rc.Width = width;
    rc.Height = height;

    gdiRet = g.DrawImage( pGdi_.get(), rc, 0, 0, width, height, Gdiplus::UnitPixel, &ia );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, DrawImage );

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, out.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    out.release();
    return jsObject;
}

std::optional<bool> 
JsGdiBitmap::ApplyMask( JsGdiBitmap* mask )
{
    assert( pGdi_ );

    if ( !mask )
    {
        JS_ReportErrorASCII( pJsCtx_, "mask argument is null" );
        return std::nullopt;
    }

    Gdiplus::Bitmap* pBitmapMask = mask->GdiBitmap();
    assert( pBitmapMask );

    if ( pBitmapMask->GetHeight() != pGdi_->GetHeight() 
         || pBitmapMask->GetWidth() != pGdi_->GetWidth() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Mismatched dimensions" );
        return std::nullopt;
    }

    Gdiplus::Rect rect( 0, 0, pGdi_->GetWidth(), pGdi_->GetHeight() );
    Gdiplus::BitmapData maskBmpData = { 0 };
    Gdiplus::BitmapData dstBmpData = { 0 };

    Gdiplus::Status gdiRet = pBitmapMask->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &maskBmpData );
    if ( Gdiplus::Ok != gdiRet )
    {
        return false;
    }

    gdiRet = pGdi_->LockBits( &rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &dstBmpData );
    if ( Gdiplus::Ok != gdiRet )
    {
        pBitmapMask->UnlockBits( &maskBmpData );
        return false;
    }

    const int width = rect.Width;
    const int height = rect.Height;
    const int size = width * height;
    //const int size_threshold = 512;
    uint32_t* pMask = reinterpret_cast<uint32_t *>( maskBmpData.Scan0 );
    uint32_t* pDst = reinterpret_cast<uint32_t *>( dstBmpData.Scan0 );
    const uint32_t* pMaskEnd = pMask + rect.Width * rect.Height;

    while ( pMask < pMaskEnd )
    {
        // Method 1:
        // alpha = (~*p_mask & 0xff) * (*p_dst >> 24) + 0x80;
        //*p_dst = ((((alpha >> 8) + alpha) & 0xff00) << 16) | (*p_dst & 0xffffff);
        // Method 2
        uint32_t alpha = ( ( ( ~*pMask & 0xff ) * ( *pDst >> 24 ) ) << 16 ) & 0xff000000;
        *pDst = alpha | ( *pDst & 0xffffff );

        ++pMask;
        ++pDst;
    }

    pGdi_->UnlockBits( &dstBmpData );
    pBitmapMask->UnlockBits( &maskBmpData );

    return true;
}

std::optional<JSObject*>
JsGdiBitmap::Clone( float x, float y, float w, float h )
{
    assert( pGdi_ );

    std::unique_ptr<Gdiplus::Bitmap> img( pGdi_->Clone( x, y, w, h, PixelFormat32bppPARGB ) );
    if ( !helpers::ensure_gdiplus_object( img.get() ) )
    {
        JS_ReportErrorASCII( pJsCtx_, "Clone failed" );
        return std::nullopt;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, img.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    img.release();
    return jsObject;
}

std::optional<JSObject*> JsGdiBitmap::CreateRawBitmap()
{
    assert( pGdi_ );

    JS::RootedObject jsObject( pJsCtx_, JsGdiRawBitmap::Create( pJsCtx_, pGdi_.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*> 
JsGdiBitmap::GetColourScheme( uint32_t count )
{
    assert( pGdi_ );

    Gdiplus::BitmapData bmpdata;
    Gdiplus::Rect rect( 0, 0, (LONG)pGdi_->GetWidth(), (LONG)pGdi_->GetHeight() );

    Gdiplus::Status gdiRet = pGdi_->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, LockBits );

    std::map<unsigned, int> color_counters;
    const unsigned colors_length = bmpdata.Width * bmpdata.Height;
    const t_uint32* colors = (const t_uint32 *)bmpdata.Scan0;

    for ( unsigned i = 0; i < colors_length; i++ )
    {
        // format: 0xaarrggbb
        unsigned color = colors[i];
        unsigned r = ( color >> 16 ) & 0xff;
        unsigned g = ( color >> 8 ) & 0xff;
        unsigned b = ( color ) & 0xff;

        // Round colors
        r = ( r + 16 ) & 0xffffffe0;
        g = ( g + 16 ) & 0xffffffe0;
        b = ( b + 16 ) & 0xffffffe0;

        if ( r > 0xff ) r = 0xff;
        if ( g > 0xff ) g = 0xff;
        if ( b > 0xff ) b = 0xff;

        ++color_counters[Gdiplus::Color::MakeARGB( 0xff, r, g, b )];
    }

    pGdi_->UnlockBits( &bmpdata );

    // Sorting
    typedef std::pair<unsigned, int> sort_vec_pair_t;
    std::vector<sort_vec_pair_t> sort_vec( color_counters.begin(), color_counters.end() );
    count = std::min( count, sort_vec.size() );
    std::partial_sort(
        sort_vec.begin(),
        sort_vec.begin() + count,
        sort_vec.end(),
        []( const sort_vec_pair_t& a, const sort_vec_pair_t& b )
        {
            return a.second > b.second;
        } );

    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, count ) );
    if ( !jsArray )
    {
        JS_ReportOutOfMemory( pJsCtx_ );
        return std::nullopt;
    }

    JS::RootedValue jsValue( pJsCtx_ );
    for ( t_size i = 0; i < count; ++i )
    {
        jsValue.setNumber( sort_vec[i].first );

        if ( !JS_SetElement( pJsCtx_, jsArray, i, jsValue ) )
        {
            JS_ReportErrorASCII( pJsCtx_, "Internal error: JS_SetElement failed" );
            return std::nullopt;
        }
    }

    return jsArray;
}

std::optional<std::string> 
JsGdiBitmap::GetColourSchemeJSON( uint32_t count )
{
    assert( pGdi_ );

    Gdiplus::BitmapData bmpdata;

    // rescaled image will have max of ~48k pixels
    uint32_t w = std::min( pGdi_->GetWidth(), static_cast<UINT>( 220 ) );
    uint32_t h = std::min( pGdi_->GetHeight(), static_cast<UINT>( 220 ) );

    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB );
    if ( !helpers::ensure_gdiplus_object( bitmap ) )
    {// TODO: replace with IF_FAILED macro
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create Gdiplus object" );
        return std::nullopt;
    }

    Gdiplus::Graphics g( bitmap );
    Gdiplus::Rect rect( 0, 0, (LONG)w, (LONG)h );
    Gdiplus::Status gdiRet = g.SetInterpolationMode( ( Gdiplus::InterpolationMode )6 ); // InterpolationModeHighQualityBilinear
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, SetInterpolationMode );

    gdiRet = g.DrawImage( pGdi_.get(), 0, 0, w, h ); // scale image down
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, DrawImage );

    gdiRet = bitmap->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, LockBits );

    std::map<unsigned, int> colour_counters;
    const unsigned colours_length = bmpdata.Width * bmpdata.Height;
    const t_uint32* colours = (const t_uint32 *)bmpdata.Scan0;

    // reduce color set to pass to k-means by rounding colour components to multiples of 8
    for ( unsigned i = 0; i < colours_length; i++ )
    {
        unsigned int r = ( colours[i] >> 16 ) & 0xff;
        unsigned int g = ( colours[i] >> 8 ) & 0xff;
        unsigned int b = ( colours[i] & 0xff );

        // round colours
        r = ( r + 4 ) & 0xfffffff8;
        g = ( g + 4 ) & 0xfffffff8;
        b = ( b + 4 ) & 0xfffffff8;

        if ( r > 255 ) r = 0xff;
        if ( g > 255 ) g = 0xff;
        if ( b > 255 ) b = 0xff;

        ++colour_counters[r << 16 | g << 8 | b];
    }
    bitmap->UnlockBits( &bmpdata );

    std::map<unsigned, int>::iterator it;
    std::vector<Point> points;
    int idx = 0;

    for ( it = colour_counters.begin(); it != colour_counters.end(); it++, idx++ )
    {
        unsigned int r = ( it->first >> 16 ) & 0xff;
        unsigned int g = ( it->first >> 8 ) & 0xff;
        unsigned int b = ( it->first & 0xff );

        std::vector<unsigned int> values = { r, g, b };
        Point p( idx, values, it->second );
        points.push_back( p );
    }

    KMeans kmeans( count, colour_counters.size(), 12 ); // 12 iterations max
    std::vector<Cluster> clusters = kmeans.run( points );

    // sort by largest clusters
    std::sort(
        clusters.begin(),
        clusters.end(),
        []( Cluster& a, Cluster& b )
        {
            return a.getTotalPoints() > b.getTotalPoints();
        } );

    json j;
    t_size outCount = std::min( count, colour_counters.size() );
    for ( t_size i = 0; i < outCount; ++i )
    {
        int colour = 0xff000000 
            | (int)clusters[i].getCentralValue( 0 ) << 16 
            | (int)clusters[i].getCentralValue( 1 ) << 8 
            | (int)clusters[i].getCentralValue( 2 );
        double frequency = clusters[i].getTotalPoints() / (double)colours_length;

        j.push_back(
            {
                { "col", colour },
                { "freq", frequency }
            } );
    }
    return j.dump();    
}

std::optional<JSObject*>
JsGdiBitmap::GetGraphics()
{
    assert( pGdi_ );

    std::unique_ptr<Gdiplus::Graphics> g( new Gdiplus::Graphics( pGdi_.get() ) );
    if ( !helpers::ensure_gdiplus_object( g.get() ) )
    {// TODO: replace with IF_FAILED macro
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create Gdiplus object" );
        return std::nullopt;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiGraphics::Create( pJsCtx_ ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    JsGdiGraphics* pNativeObject = GetNativeFromJsObject<JsGdiGraphics>( pJsCtx_, jsObject );
    if ( !pNativeObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to get JsGdiGraphics object" );
        return std::nullopt;
    }

    pNativeObject->SetGraphicsObject( g.get() );

    g.release();
    return jsObject;
}

std::optional<std::nullptr_t>
JsGdiBitmap::ReleaseGraphics( JsGdiGraphics* graphics )
{
    assert( pGdi_ );

    if ( !graphics )
    {// Not an error
        return nullptr;
    }

    auto pGdiGraphics = graphics->GetGraphicsObject();
    graphics->SetGraphicsObject( nullptr );
    if ( pGdiGraphics )
    {
        delete pGdiGraphics;
    }

    return nullptr;
}

std::optional<JSObject*>
JsGdiBitmap::Resize( uint32_t w, uint32_t h, uint32_t interpolationMode )
{
    assert( pGdi_ );

    std::unique_ptr<Gdiplus::Bitmap> bitmap( new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB ) );
    if ( !helpers::ensure_gdiplus_object( bitmap.get() ) )
    {// TODO: replace with IF_FAILED macro
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create Gdiplus object" );
        return std::nullopt;
    }

    Gdiplus::Graphics g( bitmap.get() );
    Gdiplus::Status gdiRet = g.SetInterpolationMode( ( Gdiplus::InterpolationMode )interpolationMode );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, SetInterpolationMode );

    gdiRet = g.DrawImage( pGdi_.get(), 0, 0, w, h );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, DrawImage );

    JS::RootedObject jsRetObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, bitmap.get() ) );
    if ( !jsRetObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    bitmap.release();
    return jsRetObject;
}


std::optional<JSObject*>
JsGdiBitmap::ResizeWithOpt( size_t optArgCount, uint32_t w, uint32_t h, uint32_t interpolationMode )
{
    assert( pGdi_ );

    if ( optArgCount > 1 )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return Resize( w, h );
    }

    return Resize( w, h, interpolationMode );
}

std::optional<std::nullptr_t>
JsGdiBitmap::RotateFlip( uint32_t mode )
{
    assert( pGdi_ );

    Gdiplus::Status gdiRet = pGdi_->RotateFlip( ( Gdiplus::RotateFlipType )mode );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, RotateFlip );

    return nullptr;
}

std::optional<bool> 
JsGdiBitmap::SaveAs( const std::wstring& path, const std::wstring& format )
{
    assert( pGdi_ );

    CLSID clsid_encoder;
    int ret = helpers::get_encoder_clsid( format.c_str(), &clsid_encoder );
    if ( ret < 0 )
    {
        return false;
    }

    Gdiplus::Status gdiRet = pGdi_->Save( path.c_str(), &clsid_encoder );
    return ( Gdiplus::Ok == gdiRet );
}

std::optional<std::nullptr_t>
JsGdiBitmap::StackBlur( uint32_t radius )
{
    assert( pGdi_ );

    stack_blur_filter( *pGdi_, radius );
    return nullptr;
}

}
