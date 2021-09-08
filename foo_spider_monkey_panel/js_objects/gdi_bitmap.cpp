#include <stdafx.h>

#include "gdi_bitmap.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_error_helpers.h>
#include <utils/image_helpers.h>
#include <utils/kmeans.h>
#include <utils/stackblur.h>

#include <qwr/final_action.h>

#include <cmath>
#include <map>
#include <span>

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
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( ApplyAlpha, JsGdiBitmap::ApplyAlpha )
MJS_DEFINE_JS_FN_FROM_NATIVE( ApplyMask, JsGdiBitmap::ApplyMask )
MJS_DEFINE_JS_FN_FROM_NATIVE( Clone, JsGdiBitmap::Clone )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateRawBitmap, JsGdiBitmap::CreateRawBitmap )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetColourScheme, JsGdiBitmap::GetColourScheme )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetColourSchemeJSON, JsGdiBitmap::GetColourSchemeJSON )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetGraphics, JsGdiBitmap::GetGraphics )
MJS_DEFINE_JS_FN_FROM_NATIVE( InvertColours, JsGdiBitmap::InvertColours )
MJS_DEFINE_JS_FN_FROM_NATIVE( ReleaseGraphics, JsGdiBitmap::ReleaseGraphics )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Resize, JsGdiBitmap::Resize, JsGdiBitmap::ResizeWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( RotateFlip, JsGdiBitmap::RotateFlip )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SaveAs, JsGdiBitmap::SaveAs, JsGdiBitmap::SaveAsWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( StackBlur, JsGdiBitmap::StackBlur )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "ApplyAlpha", ApplyAlpha, 1, kDefaultPropsFlags ),
        JS_FN( "ApplyMask", ApplyMask, 1, kDefaultPropsFlags ),
        JS_FN( "Clone", Clone, 4, kDefaultPropsFlags ),
        JS_FN( "CreateRawBitmap", CreateRawBitmap, 0, kDefaultPropsFlags ),
        JS_FN( "GetColourScheme", GetColourScheme, 1, kDefaultPropsFlags ),
        JS_FN( "GetColourSchemeJSON", GetColourSchemeJSON, 1, kDefaultPropsFlags ),
        JS_FN( "GetGraphics", GetGraphics, 0, kDefaultPropsFlags ),
        JS_FN( "InvertColours", InvertColours, 0, kDefaultPropsFlags ),
        JS_FN( "ReleaseGraphics", ReleaseGraphics, 1, kDefaultPropsFlags ),
        JS_FN( "Resize", Resize, 2, kDefaultPropsFlags ),
        JS_FN( "RotateFlip", RotateFlip, 1, kDefaultPropsFlags ),
        JS_FN( "SaveAs", SaveAs, 1, kDefaultPropsFlags ),
        JS_FN( "StackBlur", StackBlur, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsGdiBitmap::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsGdiBitmap::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Height", get_Height, kDefaultPropsFlags ),
        JS_PSG( "Width", get_Width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( GdiBitmap_Constructor, JsGdiBitmap::Constructor )

} // namespace

namespace
{

std::unique_ptr<Gdiplus::Bitmap> CreateDownsizedImage( Gdiplus::Bitmap& srcImg, uint32_t maxPixelCount )
{
    const auto [imgWidth, imgHeight] = [&srcImg, maxPixelCount] {
        if ( srcImg.GetWidth() * srcImg.GetHeight() > maxPixelCount )
        {
            const double ratio = static_cast<double>( srcImg.GetWidth() ) / srcImg.GetHeight();
            const auto imgHeight = static_cast<uint32_t>( std::round( std::sqrt( maxPixelCount / ratio ) ) );
            const auto imgWidth = static_cast<uint32_t>( std::round( imgHeight * ratio ) );

            return std::make_tuple( imgWidth, imgHeight );
        }
        else
        {
            return std::make_tuple( srcImg.GetWidth(), srcImg.GetHeight() );
        }
    }();

    auto pBitmap = std::make_unique<Gdiplus::Bitmap>( imgWidth, imgHeight, PixelFormat32bppPARGB );
    qwr::error::CheckGdiPlusObject( pBitmap );

    Gdiplus::Graphics gr( pBitmap.get() );

    Gdiplus::Status gdiRet = gr.SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBilinear );
    qwr::error::CheckGdi( gdiRet, "SetInterpolationMode" );

    gdiRet = gr.DrawImage( &srcImg, 0, 0, imgWidth, imgHeight ); // scale image down
    qwr::error::CheckGdi( gdiRet, "DrawImage" );

    return pBitmap;
}

} // namespace

namespace mozjs
{

const JSClass JsGdiBitmap::JsClass = jsClass;
const JSFunctionSpec* JsGdiBitmap::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsGdiBitmap::JsProperties = jsProperties.data();
const JsPrototypeId JsGdiBitmap::PrototypeId = JsPrototypeId::GdiBitmap;
const JSNative JsGdiBitmap::JsConstructor = ::GdiBitmap_Constructor;

JsGdiBitmap::JsGdiBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap )
    : pJsCtx_( cx )
    , pGdi_( std::move( gdiBitmap ) )
{
}

std::unique_ptr<JsGdiBitmap>
JsGdiBitmap::CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap )
{
    qwr::QwrException::ExpectTrue( !!gdiBitmap, "Internal error: Gdiplus::Bitmap object is null" );

    return std::unique_ptr<JsGdiBitmap>( new JsGdiBitmap( cx, std::move( gdiBitmap ) ) );
}

size_t JsGdiBitmap::GetInternalSize( const std::unique_ptr<Gdiplus::Bitmap>& gdiBitmap )
{
    if ( !gdiBitmap )
    { // we don't care about return value, since it will fail in CreateNative later
        return 0;
    }
    return sizeof( Gdiplus::Bitmap ) + gdiBitmap->GetWidth() * gdiBitmap->GetHeight() * Gdiplus::GetPixelFormatSize( gdiBitmap->GetPixelFormat() ) / 8;
}

Gdiplus::Bitmap* JsGdiBitmap::GdiBitmap() const
{
    return pGdi_.get();
}

JSObject* JsGdiBitmap::Constructor( JSContext* cx, JsGdiBitmap* other )
{
    qwr::QwrException::ExpectTrue( other, "Invalid argument type" );

    auto pGdi = other->GdiBitmap();

    std::unique_ptr<Gdiplus::Bitmap> img( pGdi->Clone( 0, 0, pGdi->GetWidth(), pGdi->GetHeight(), PixelFormat32bppPARGB ) );
    qwr::error::CheckGdiPlusObject( img, pGdi );

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
    qwr::error::CheckGdiPlusObject( out );

    Gdiplus::ColorMatrix cm{};
    cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0;
    cm.m[3][3] = static_cast<float>( alpha ) / 255;

    Gdiplus::ImageAttributes ia;
    Gdiplus::Status gdiRet = ia.SetColorMatrix( &cm );
    qwr::error::CheckGdi( gdiRet, "SetColorMatrix" );

    Gdiplus::Graphics g( out.get() );
    gdiRet = g.DrawImage( pGdi_.get(),
                          Gdiplus::Rect{ 0, 0, static_cast<int>( width ), static_cast<int>( height ) },
                          0,
                          0,
                          width,
                          height,
                          Gdiplus::UnitPixel,
                          &ia );
    qwr::error::CheckGdi( gdiRet, "DrawImage" );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( out ) );
}

void JsGdiBitmap::ApplyMask( JsGdiBitmap* mask )
{
    qwr::QwrException::ExpectTrue( mask, "mask argument is null" );

    Gdiplus::Bitmap* pBitmapMask = mask->GdiBitmap();
    assert( pBitmapMask );

    qwr::QwrException::ExpectTrue( pBitmapMask->GetHeight() == pGdi_->GetHeight()
                                       && pBitmapMask->GetWidth() == pGdi_->GetWidth(),
                                   "Mismatched dimensions" );

    const Gdiplus::Rect rect{ 0, 0, static_cast<int>( pGdi_->GetWidth() ), static_cast<int>( pGdi_->GetHeight() ) };

    Gdiplus::BitmapData maskBmpData = { 0 };
    Gdiplus::Status gdiRet = pBitmapMask->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &maskBmpData );
    qwr::error::CheckGdi( gdiRet, "mask::LockBits" );

    qwr::final_action autoMaskBits( [pBitmapMask, &maskBmpData] {
        pBitmapMask->UnlockBits( &maskBmpData );
    } );

    Gdiplus::BitmapData dstBmpData = { 0 };
    gdiRet = pGdi_->LockBits( &rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &dstBmpData );
    qwr::error::CheckGdi( gdiRet, "dst::LockBits" );

    qwr::final_action autoDstBits( [&pGdi = pGdi_, &dstBmpData] {
        pGdi->UnlockBits( &dstBmpData );
    } );

    assert( maskBmpData.Scan0 );
    const auto maskRange = ranges::make_subrange( reinterpret_cast<uint32_t*>( maskBmpData.Scan0 ),
                                                  reinterpret_cast<uint32_t*>( maskBmpData.Scan0 ) + rect.Width * rect.Height );
    for ( auto pMaskIt = maskRange.begin(), pDst = reinterpret_cast<uint32_t*>( dstBmpData.Scan0 ); pMaskIt != maskRange.end(); ++pMaskIt, ++pDst )
    {
        /// Method 1:
        // alpha = (~*p_mask & 0xff) * (*p_dst >> 24) + 0x80;
        // *p_dst = ((((alpha >> 8) + alpha) & 0xff00) << 16) | (*p_dst & 0xffffff);

        /// Method 2
        uint32_t alpha = ( ( ( ~*pMaskIt & 0xff ) * ( *pDst >> 24 ) ) << 16 ) & 0xff000000;
        *pDst = alpha | ( *pDst & 0xffffff );
    }
}

JSObject* JsGdiBitmap::Clone( float x, float y, float w, float h )
{
    std::unique_ptr<Gdiplus::Bitmap> img( pGdi_->Clone( x, y, w, h, PixelFormat32bppPARGB ) );
    qwr::error::CheckGdiPlusObject( img, pGdi_.get() );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( img ) );
}

JSObject* JsGdiBitmap::CreateRawBitmap()
{
    return JsGdiRawBitmap::CreateJs( pJsCtx_, pGdi_.get() );
}

JS::Value JsGdiBitmap::GetColourScheme( uint32_t count )
{
    constexpr uint32_t kMaxPixelCount = 220 * 220;
    auto pBitmap = CreateDownsizedImage( *pGdi_, kMaxPixelCount );
    assert( pBitmap );

    const Gdiplus::Rect rect{ 0, 0, static_cast<int>( pBitmap->GetWidth() ), static_cast<int>( pBitmap->GetHeight() ) };
    Gdiplus::BitmapData bmpdata{};

    Gdiplus::Status gdiRet = pBitmap->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata );
    qwr::error::CheckGdi( gdiRet, "LockBits" );

    std::map<uint32_t, uint32_t> color_counters;
    const auto colourRange = ranges::make_subrange( reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ),
                                                    reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ) + bmpdata.Width * bmpdata.Height );
    for ( auto colour: colourRange )
    {
        // format: 0xaarrggbb
        uint32_t r = ( colour >> 16 ) & 0xff;
        uint32_t g = ( colour >> 8 ) & 0xff;
        uint32_t b = colour & 0xff;

        // Round colors
        r = ( r > 0xef ) ? 0xff : ( r + 0x10 ) & 0xe0;
        g = ( g > 0xef ) ? 0xff : ( g + 0x10 ) & 0xe0;
        b = ( b > 0xef ) ? 0xff : ( b + 0x10 ) & 0xe0;

        ++color_counters[Gdiplus::Color::MakeARGB( 0xff,
                                                   static_cast<BYTE>( r ),
                                                   static_cast<BYTE>( g ),
                                                   static_cast<BYTE>( b ) )];
    }

    pBitmap->UnlockBits( &bmpdata );

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
    return jsValue;
}

qwr::u8string JsGdiBitmap::GetColourSchemeJSON( uint32_t count )
{
    using json = nlohmann::json;
    namespace kmeans = smp::utils::kmeans;

    // rescaled image will have max of ~48k pixels
    constexpr uint32_t kMaxPixelCount = 220 * 220;
    auto pBitmap = CreateDownsizedImage( *pGdi_, kMaxPixelCount );
    assert( pBitmap );

    const Gdiplus::Rect rect{ 0, 0, static_cast<int>( pBitmap->GetWidth() ), static_cast<int>( pBitmap->GetHeight() ) };
    Gdiplus::BitmapData bmpdata{};

    Gdiplus::Status gdiRet = pBitmap->LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata );
    qwr::error::CheckGdi( gdiRet, "LockBits" );

    std::map<uint32_t, uint32_t> colour_counters;
    const auto colourRange = ranges::make_subrange( reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ),
                                                    reinterpret_cast<const uint32_t*>( bmpdata.Scan0 ) + bmpdata.Width * bmpdata.Height );
    for ( auto colour: colourRange )
    { // reduce color set to pass to k-means by rounding colour components to multiples of 8
        uint32_t r = ( colour >> 16 ) & 0xff;
        uint32_t g = ( colour >> 8 ) & 0xff;
        uint32_t b = ( colour & 0xff );

        // We're reducing total colors from 2^24 to 2^15 by rounding each color component value to multiples of 8.
        // First we need to check if the byte will overflow, and if so pin to 0xff, otherwise add 4 and round down.
        r = ( r > 0xfb ) ? 0xff : ( r + 4 ) & 0xf8;
        g = ( g > 0xfb ) ? 0xff : ( g + 4 ) & 0xf8;
        b = ( b > 0xfb ) ? 0xff : ( b + 4 ) & 0xf8;

        ++colour_counters[r << 16 | g << 8 | b];
    }
    pBitmap->UnlockBits( &bmpdata );

    const auto points =
        ranges::views::transform( colour_counters, []( const auto& colourCounter ) {
            const auto [colour, pixelCount] = colourCounter;

            const uint8_t r = ( colour >> 16 ) & 0xff;
            const uint8_t g = ( colour >> 8 ) & 0xff;
            const uint8_t b = ( colour & 0xff );

            return kmeans::PointData{ std::vector<uint8_t>{ r, g, b }, pixelCount };
        } )
        | ranges::to_vector;

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
    if ( clusters.size() > count )
    {
        clusters.resize( count );
    }

    json j = json::array();
    for ( const auto& cluster: clusters )
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

    return j.dump( 2 );
}

JSObject* JsGdiBitmap::GetGraphics()
{
    std::unique_ptr<Gdiplus::Graphics> g( new Gdiplus::Graphics( pGdi_.get() ) );
    qwr::error::CheckGdiPlusObject( g );

    JS::RootedObject jsObject( pJsCtx_, JsGdiGraphics::CreateJs( pJsCtx_ ) );

    auto* pNativeObject = GetInnerInstancePrivate<JsGdiGraphics>( pJsCtx_, jsObject );
    qwr::QwrException::ExpectTrue( pNativeObject, "Internal error: failed to get JsGdiGraphics object" );

    pNativeObject->SetGraphicsObject( g.release() );

    return jsObject;
}

JSObject* JsGdiBitmap::InvertColours()
{
    const UINT width = pGdi_->GetWidth();
    const UINT height = pGdi_->GetHeight();

    std::unique_ptr<Gdiplus::Bitmap> out( new Gdiplus::Bitmap( width, height, PixelFormat32bppPARGB ) );
    qwr::error::CheckGdiPlusObject( out );

    Gdiplus::ColorMatrix cm{};
    cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = -1.f;
    cm.m[3][3] = cm.m[4][0] = cm.m[4][1] = cm.m[4][2] = cm.m[4][4] = 1.f;

    Gdiplus::ImageAttributes ia;
    Gdiplus::Status gdiRet = ia.SetColorMatrix( &cm );
    qwr::error::CheckGdi( gdiRet, "SetColorMatrix" );

    Gdiplus::Graphics g( out.get() );
    gdiRet = g.DrawImage( pGdi_.get(),
                          Gdiplus::Rect{ 0, 0, static_cast<int>( width ), static_cast<int>( height ) },
                          0,
                          0,
                          width,
                          height,
                          Gdiplus::UnitPixel,
                          &ia );
    qwr::error::CheckGdi( gdiRet, "DrawImage" );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( out ) );
}

void JsGdiBitmap::ReleaseGraphics( JsGdiGraphics* graphics )
{
    if ( !graphics )
    { // Not an error
        return;
    }

    auto pGdiGraphics = graphics->GetGraphicsObject();
    graphics->SetGraphicsObject( nullptr );
    delete pGdiGraphics;
}

JSObject* JsGdiBitmap::Resize( uint32_t w, uint32_t h, uint32_t interpolationMode )
{
    std::unique_ptr<Gdiplus::Bitmap> bitmap( new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB ) );
    qwr::error::CheckGdiPlusObject( bitmap );

    Gdiplus::Graphics g( bitmap.get() );
    Gdiplus::Status gdiRet = g.SetInterpolationMode( static_cast<Gdiplus::InterpolationMode>( interpolationMode ) );
    qwr::error::CheckGdi( gdiRet, "SetInterpolationMode" );

    gdiRet = g.DrawImage( pGdi_.get(), 0, 0, w, h );
    qwr::error::CheckGdi( gdiRet, "DrawImage" );

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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiBitmap::RotateFlip( uint32_t mode )
{
    Gdiplus::Status gdiRet = pGdi_->RotateFlip( static_cast<Gdiplus::RotateFlipType>( mode ) );
    qwr::error::CheckGdi( gdiRet, "RotateFlip" );
}

bool JsGdiBitmap::SaveAs( const std::wstring& path, const std::wstring& format )
{
    const auto clsIdRet = [&format]() -> std::optional<CLSID> {
        UINT num = 0;
        UINT size = 0;
        Gdiplus::Status status = Gdiplus::GetImageEncodersSize( &num, &size );
        if ( status != Gdiplus::Ok || !size )
        {
            return std::nullopt;
        }

        std::vector<uint8_t> imageCodeInfoBuf( size );
        auto* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>( imageCodeInfoBuf.data() );

        status = Gdiplus::GetImageEncoders( num, size, pImageCodecInfo );
        if ( status != Gdiplus::Ok )
        {
            return std::nullopt;
        }

        std::span<Gdiplus::ImageCodecInfo> codecSpan{ pImageCodecInfo, num };
        const auto it = ranges::find_if( codecSpan, [&format]( const auto& codec ) { return ( format == codec.MimeType ); } );
        if ( it == codecSpan.end() )
        {
            return std::nullopt;
        }

        return it->Clsid;
    }();

    if ( !clsIdRet )
    {
        return false;
    }

    Gdiplus::Status gdiRet = pGdi_->Save( path.c_str(), &( *clsIdRet ) );
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
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiBitmap::StackBlur( uint32_t radius )
{
    smp::utils::stack_blur_filter( *pGdi_, radius );
}

} // namespace mozjs
