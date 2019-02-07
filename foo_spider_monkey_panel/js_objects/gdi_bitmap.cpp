#include <stdafx.h>
#include "gdi_bitmap.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <utils/gdi_error_helpers.h>
#include <utils/scope_helpers.h>
#include <utils/image_helpers.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <utils/stackblur.h>
#include <utils/kmeans.h>

#include <nlohmann/json.hpp>

#include <map>

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
    JsGdiBitmap::FinalizeJsObject,
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

MJS_DEFINE_JS_FN_FROM_NATIVE( ApplyAlpha, JsGdiBitmap::ApplyAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( ApplyMask, JsGdiBitmap::ApplyMask )
MJS_DEFINE_JS_FN_FROM_NATIVE( Clone, JsGdiBitmap::Clone )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateRawBitmap, JsGdiBitmap::CreateRawBitmap )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetColourScheme, JsGdiBitmap::GetColourScheme )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetColourSchemeJSON, JsGdiBitmap::GetColourSchemeJSON )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetGraphics, JsGdiBitmap::GetGraphics )
MJS_DEFINE_JS_FN_FROM_NATIVE( ReleaseGraphics, JsGdiBitmap::ReleaseGraphics )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Resize, JsGdiBitmap::Resize, JsGdiBitmap::ResizeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( RotateFlip, JsGdiBitmap::RotateFlip )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SaveAs, JsGdiBitmap::SaveAs, JsGdiBitmap::SaveAsWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( StackBlur, JsGdiBitmap::StackBlur )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "ApplyAlpha", ApplyAlpha, 1, DefaultPropsFlags() ),
    JS_FN( "ApplyMask", ApplyMask, 1, DefaultPropsFlags() ),
    JS_FN( "Clone", Clone, 4, DefaultPropsFlags() ),
    JS_FN( "CreateRawBitmap", CreateRawBitmap, 0, DefaultPropsFlags() ),
    JS_FN( "GetColourScheme", GetColourScheme, 1, DefaultPropsFlags() ),
    JS_FN( "GetColourSchemeJSON", GetColourSchemeJSON, 1, DefaultPropsFlags() ),
    JS_FN( "GetGraphics", GetGraphics, 0, DefaultPropsFlags() ),
    JS_FN( "ReleaseGraphics", ReleaseGraphics, 1, DefaultPropsFlags() ),
    JS_FN( "Resize", Resize, 2, DefaultPropsFlags() ),
    JS_FN( "RotateFlip", RotateFlip, 1, DefaultPropsFlags() ),
    JS_FN( "SaveAs", SaveAs, 1, DefaultPropsFlags() ),
    JS_FN( "StackBlur", StackBlur, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsGdiBitmap::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsGdiBitmap::get_Width )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Width", get_Width, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace
{

bool Constructor_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    SmpException::ExpectTrue( argc, "Argument is missing" );

    args.rval().setObjectOrNull( JsGdiBitmap::Constructor( cx, convert::to_native::ToValue<JsGdiBitmap*>( cx, args[0] ) ) );
    return true;
}

MJS_DEFINE_JS_FN( Constructor, Constructor_Impl )

} // namespace

namespace mozjs
{

const JSClass JsGdiBitmap::JsClass = jsClass;
const JSFunctionSpec* JsGdiBitmap::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiBitmap::JsProperties = jsProperties;
const JsPrototypeId JsGdiBitmap::PrototypeId = JsPrototypeId::GdiBitmap;
const JSNative JsGdiBitmap::JsConstructor = ::Constructor;

JsGdiBitmap::JsGdiBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap )
    : pJsCtx_( cx )
    , pGdi_( std::move( gdiBitmap ) )
{
}

JsGdiBitmap::~JsGdiBitmap()
{
}

std::unique_ptr<JsGdiBitmap>
JsGdiBitmap::CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap )
{
    SmpException::ExpectTrue( !!gdiBitmap, "Internal error: Gdiplus::Bitmap object is null" );

    return std::unique_ptr<JsGdiBitmap>( new JsGdiBitmap( cx, std::move( gdiBitmap ) ) );
}

size_t JsGdiBitmap::GetInternalSize( const std::unique_ptr<Gdiplus::Bitmap>& gdiBitmap )
{
    return sizeof( Gdiplus::Bitmap ) + gdiBitmap->GetWidth() * gdiBitmap->GetHeight() * Gdiplus::GetPixelFormatSize( gdiBitmap->GetPixelFormat() ) / 8;
}

Gdiplus::Bitmap* JsGdiBitmap::GdiBitmap() const
{
    return pGdi_.get();
}

JSObject* JsGdiBitmap::Constructor( JSContext* cx, JsGdiBitmap* other )
{
    SmpException::ExpectTrue( other, "Invalid argument type" );

    auto pGdi = other->GdiBitmap();

    std::unique_ptr<Gdiplus::Bitmap> img( pGdi->Clone( 0, 0, pGdi->GetWidth(), pGdi->GetHeight(), PixelFormat32bppPARGB ) );
    smp::error::CheckGdiPlusObject( img );

    return JsGdiBitmap::CreateJs( cx, std::move( img ) );
}

std::uint32_t JsGdiBitmap::get_Height()
{
    return pGdi_->GetHeight();
}

std::uint32_t JsGdiBitmap::get_Width()
{
    return pGdi_->GetWidth();
}

JSObject* JsGdiBitmap::ApplyAlpha( uint8_t alpha )
{
    const UINT width = pGdi_->GetWidth();
    const UINT height = pGdi_->GetHeight();

    std::unique_ptr<Gdiplus::Bitmap> out( new Gdiplus::Bitmap( width, height, PixelFormat32bppPARGB ) );
    smp::error::CheckGdiPlusObject( out );

    Gdiplus::ColorMatrix cm = { 0.0 };
    cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0;
    cm.m[3][3] = static_cast<float>( alpha ) / 255;

    Gdiplus::ImageAttributes ia;
    Gdiplus::Status gdiRet = ia.SetColorMatrix( &cm );
    smp::error::CheckGdi( gdiRet, "SetColorMatrix" );

    Gdiplus::Graphics g( out.get() );
    gdiRet = g.DrawImage( pGdi_.get(),
                          Gdiplus::Rect{ 0, 0, static_cast<int>( width ), static_cast<int>( height ) },
                          0,
                          0,
                          width,
                          height,
                          Gdiplus::UnitPixel,
                          &ia );
    smp::error::CheckGdi( gdiRet, "DrawImage" );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( out ) );
}

void JsGdiBitmap::ApplyMask( JsGdiBitmap* mask )
{
    SmpException::ExpectTrue( mask, "mask argument is null" );

    Gdiplus::Bitmap* pBitmapMask = mask->GdiBitmap();
    assert( pBitmapMask );

    SmpException::ExpectTrue( pBitmapMask->GetHeight() == pGdi_->GetHeight()
                                  && pBitmapMask->GetWidth() == pGdi_->GetWidth(),
                              "Mismatched dimensions" );

    const Gdiplus::Rect rect{ 0, 0, static_cast<int>( pGdi_->GetWidth() ), static_cast<int>( pGdi_->GetHeight() ) };

    Gdiplus::BitmapData maskBmpData = { 0 };
    Gdiplus::Status gdiRet = pBitmapMask->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &maskBmpData );
    smp::error::CheckGdi( gdiRet, "mask::LockBits" );

    utils::final_action autoMaskBits( [pBitmapMask, &maskBmpData] {
        pBitmapMask->UnlockBits( &maskBmpData );
    } );

    Gdiplus::BitmapData dstBmpData = { 0 };
    gdiRet = pGdi_->LockBits( &rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &dstBmpData );
    smp::error::CheckGdi( gdiRet, "dst::LockBits" );

    utils::final_action autoDstBits( [& pGdi = pGdi_, &dstBmpData] {
        pGdi->UnlockBits( &dstBmpData );
    } );

    const auto maskRange = ranges::make_iterator_range( reinterpret_cast<uint32_t*>( maskBmpData.Scan0 ),
                                                        reinterpret_cast<uint32_t*>( maskBmpData.Scan0 ) + rect.Width * rect.Height );
    for ( auto pMask = maskRange.begin(), pDst = reinterpret_cast<uint32_t*>( dstBmpData.Scan0 ); pMask != maskRange.end(); ++pMask, ++pDst )
    {
        /// Method 1:
        // alpha = (~*p_mask & 0xff) * (*p_dst >> 24) + 0x80;
        // *p_dst = ((((alpha >> 8) + alpha) & 0xff00) << 16) | (*p_dst & 0xffffff);

        /// Method 2
        uint32_t alpha = ( ( ( ~*pMask & 0xff ) * ( *pDst >> 24 ) ) << 16 ) & 0xff000000;
        *pDst = alpha | ( *pDst & 0xffffff );
    }
}

JSObject* JsGdiBitmap::Clone( float x, float y, float w, float h )
{
    std::unique_ptr<Gdiplus::Bitmap> img( pGdi_->Clone( x, y, w, h, PixelFormat32bppPARGB ) );
    smp::error::CheckGdiPlusObject( img, pGdi_.get() );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( img ) );
}

JSObject* JsGdiBitmap::CreateRawBitmap()
{
    return JsGdiRawBitmap::CreateJs( pJsCtx_, pGdi_.get() );
}

JSObject* JsGdiBitmap::GetColourScheme( uint32_t count )
{
    const Gdiplus::Rect rect{ 0, 0, static_cast<int>( pGdi_->GetWidth() ), static_cast<int>( pGdi_->GetHeight() ) };
    Gdiplus::BitmapData bmpdata;

    Gdiplus::Status gdiRet = pGdi_->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata );
    smp::error::CheckGdi( gdiRet, "LockBits" );

    std::map<uint32_t, uint32_t> color_counters;
    const auto colourRange = ranges::make_iterator_range( reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ),
                                                          reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ) + bmpdata.Width * bmpdata.Height );
    for ( auto colour : colourRange )
    {
        // format: 0xaarrggbb
        uint32_t r = ( colour >> 16 ) & 0xff;
        uint32_t g = ( colour >> 8 ) & 0xff;
        uint32_t b = colour & 0xff;

        // Round colors
        r = ( r + 16 ) & 0xffffffe0;
        g = ( g + 16 ) & 0xffffffe0;
        b = ( b + 16 ) & 0xffffffe0;

        if ( r > 0xff )
            r = 0xff;
        if ( g > 0xff )
            g = 0xff;
        if ( b > 0xff )
            b = 0xff;

        ++color_counters[Gdiplus::Color::MakeARGB( 0xff,
                                                   static_cast<BYTE>( r ),
                                                   static_cast<BYTE>( g ),
                                                   static_cast<BYTE>( b ) )];
    }

    pGdi_->UnlockBits( &bmpdata );

    std::vector<std::pair<uint32_t, uint32_t>> sort_vec( color_counters.cbegin(), color_counters.cend() );
    ranges::sort( sort_vec,
                  []( const auto& a, const auto& b ) {
                      return a.second > b.second;
                  } );
    sort_vec.resize( std::min( count, color_counters.size() ) );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue(
        pJsCtx_,
        sort_vec,
        []( const auto& vec, auto index ) {
            return vec[index].first;
        },
        &jsValue );

    return &jsValue.toObject();
}

pfc::string8_fast JsGdiBitmap::GetColourSchemeJSON( uint32_t count )
{
    using json = nlohmann::json;
    namespace kmeans = smp::utils::kmeans;

    // rescaled image will have max of ~48k pixels
    const auto [imgWidth, imgHeight] = [& pGdi = pGdi_] {
        constexpr uint32_t kMaxDimensionSize = 220;
        return smp::image::GetResizedImageSize( std::make_tuple( pGdi->GetWidth(), pGdi->GetHeight() ), std::make_tuple( kMaxDimensionSize, kMaxDimensionSize ) );
    }();

    auto bitmap = std::make_unique<Gdiplus::Bitmap>( imgWidth, imgHeight, PixelFormat32bppPARGB );
    smp::error::CheckGdiPlusObject( bitmap );

    Gdiplus::Graphics gr( bitmap.get() );

    Gdiplus::Status gdiRet = gr.SetInterpolationMode( (Gdiplus::InterpolationMode)6 ); // InterpolationModeHighQualityBilinear
    smp::error::CheckGdi( gdiRet, "SetInterpolationMode" );

    gdiRet = gr.DrawImage( pGdi_.get(), 0, 0, imgWidth, imgHeight ); // scale image down
    smp::error::CheckGdi( gdiRet, "DrawImage" );

    const Gdiplus::Rect rect( 0, 0, (LONG)imgWidth, (LONG)imgHeight );
    Gdiplus::BitmapData bmpdata;

    gdiRet = bitmap->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata );
    smp::error::CheckGdi( gdiRet, "LockBits" );

    std::map<uint32_t, uint32_t> colour_counters;
    const auto colourRange = ranges::make_iterator_range( reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ),
                                                          reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ) + bmpdata.Width * bmpdata.Height );
    for ( auto colour : colourRange )
    { // reduce color set to pass to k-means by rounding colour components to multiples of 8
        uint32_t r = ( colour >> 16 ) & 0xff;
        uint32_t g = ( colour >> 8 ) & 0xff;
        uint32_t b = ( colour & 0xff );

        // round colours
        r = ( r + 4 ) & 0xfffffff8;
        g = ( g + 4 ) & 0xfffffff8;
        b = ( b + 4 ) & 0xfffffff8;

        if ( r > 0xff )
            r = 0xff;
        if ( g > 0xff )
            g = 0xff;
        if ( b > 0xff )
            b = 0xff;

        ++colour_counters[r << 16 | g << 8 | b];
    }
    bitmap->UnlockBits( &bmpdata );

    const std::vector<kmeans::PointData> points =
        ranges::view::transform( colour_counters, []( const auto& colourCounter ) {
            const auto [colour, pixelCount] = colourCounter;

            const uint8_t r = ( colour >> 16 ) & 0xff;
            const uint8_t g = ( colour >> 8 ) & 0xff;
            const uint8_t b = ( colour & 0xff );

            return kmeans::PointData{ std::vector<uint8_t>{ r, g, b }, pixelCount };
        } );

    constexpr uint32_t kKmeansIterationCount = 12;
    std::vector<kmeans::ClusterData> clusters = kmeans::run( points, count, kKmeansIterationCount );

    const auto getTotalPixelCount = []( const kmeans::ClusterData& cluster ) -> uint32_t {
        return ranges::accumulate( cluster.points, 0, []( auto sum, const auto pData ) {
            return sum + pData->pixel_count;
        } );
    };

    // sort by largest clusters
    ranges::sort( clusters, [&getTotalPixelCount]( const auto& a, const auto& b ) {
        return getTotalPixelCount( a ) > getTotalPixelCount( b );
    } );
    if ( count < clusters.size() )
    {
        clusters.erase( clusters.cbegin() + count, clusters.cend() );
    }

    json j = json::array();
    for ( const auto& cluster : clusters )
    {
        const auto& centralValues = cluster.central_values;

        const uint32_t colour = 0xff000000
                                | static_cast<uint32_t>( centralValues[0] ) << 16
                                | static_cast<uint32_t>( centralValues[1] ) << 8
                                | static_cast<uint32_t>( centralValues[2] );
        const double frequency = static_cast<double>( getTotalPixelCount( cluster ) ) / colourRange.size();

        j.push_back(
            { { "col", colour },
              { "freq", frequency } } );
    }

    return j.dump().c_str();
}

JSObject* JsGdiBitmap::GetGraphics()
{
    std::unique_ptr<Gdiplus::Graphics> g( new Gdiplus::Graphics( pGdi_.get() ) );
    smp::error::CheckGdiPlusObject( g );

    JS::RootedObject jsObject( pJsCtx_, JsGdiGraphics::CreateJs( pJsCtx_ ) );

    JsGdiGraphics* pNativeObject = GetInnerInstancePrivate<JsGdiGraphics>( pJsCtx_, jsObject );
    SmpException::ExpectTrue( pNativeObject, "Internal error: failed to get JsGdiGraphics object" );

    pNativeObject->SetGraphicsObject( g.release() );

    return jsObject;
}

void JsGdiBitmap::ReleaseGraphics( JsGdiGraphics* graphics )
{
    if ( !graphics )
    { // Not an error
        return;
    }

    auto pGdiGraphics = graphics->GetGraphicsObject();
    graphics->SetGraphicsObject( nullptr );
    if ( pGdiGraphics )
    {
        delete pGdiGraphics;
    }
}

JSObject* JsGdiBitmap::Resize( uint32_t w, uint32_t h, uint32_t interpolationMode )
{
    std::unique_ptr<Gdiplus::Bitmap> bitmap( new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB ) );
    smp::error::CheckGdiPlusObject( bitmap );

    Gdiplus::Graphics g( bitmap.get() );
    Gdiplus::Status gdiRet = g.SetInterpolationMode( (Gdiplus::InterpolationMode)interpolationMode );
    smp::error::CheckGdi( gdiRet, "SetInterpolationMode" );

    gdiRet = g.DrawImage( pGdi_.get(), 0, 0, w, h );
    smp::error::CheckGdi( gdiRet, "DrawImage" );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( bitmap ) );
}

JSObject* JsGdiBitmap::ResizeWithOpt( size_t optArgCount, uint32_t w, uint32_t h, uint32_t interpolationMode )
{
    switch ( optArgCount )
    {
    case 0:
        return Resize( w, h, interpolationMode );
    case 1:
        return Resize( w, h );
    default:
        throw SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiBitmap::RotateFlip( uint32_t mode )
{
    Gdiplus::Status gdiRet = pGdi_->RotateFlip( (Gdiplus::RotateFlipType)mode );
    smp::error::CheckGdi( gdiRet, "RotateFlip" );
}

bool JsGdiBitmap::SaveAs( const std::wstring& path, const std::wstring& format )
{
    CLSID clsid_encoder;
    const int imageEncoderId = []( const std::wstring& format, CLSID& clsId ) -> int { // get image encoder
        UINT num = 0;
        UINT size = 0;
        Gdiplus::Status status = Gdiplus::GetImageEncodersSize( &num, &size );
        if ( status != Gdiplus::Ok || !size )
        {
            return -1;
        }

        std::vector<uint8_t> imageCodeInfoBuf( size );
        Gdiplus::ImageCodecInfo* pImageCodecInfo =
            reinterpret_cast<Gdiplus::ImageCodecInfo*>( imageCodeInfoBuf.data() );

        status = Gdiplus::GetImageEncoders( num, size, pImageCodecInfo );
        if ( status != Gdiplus::Ok )
        {
            return -1;
        }

        for ( UINT i = 0; i < num; ++i )
        {
            if ( format != pImageCodecInfo[i].MimeType )
            {
                clsId = pImageCodecInfo[i].Clsid;
                return i;
            }
        }

        return -1;
    }( format, clsid_encoder );

    if ( imageEncoderId < 0 )
    {
        return false;
    }

    Gdiplus::Status gdiRet = pGdi_->Save( path.c_str(), &clsid_encoder );
    return ( Gdiplus::Ok == gdiRet );
}

bool JsGdiBitmap::SaveAsWithOpt( size_t optArgCount, const std::wstring& path, const std::wstring& format /* ='image/png' */ )
{
    switch ( optArgCount )
    {
    case 0:
        return SaveAs( path, format );
    case 1:
        return SaveAs( path );
    default:
        throw SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsGdiBitmap::StackBlur( uint32_t radius )
{
    smp::utils::stack_blur_filter( *pGdi_, radius );
}

} // namespace mozjs
