#include <stdafx.h>

#include "image_data.h"

#include <js_backend/engine/js_to_native_invoker.h>

#include <js/experimental/TypedData.h>

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
    ImageData::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ImageData",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_data, ImageData::get_Data );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, ImageData::get_Height );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, ImageData::get_Width );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "data", get_data, kDefaultPropsFlags ),
        JS_PSG( "height", get_height, kDefaultPropsFlags ),
        JS_PSG( "width", get_width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ImageData_Constructor, ImageData::Constructor_Fake, ImageData::ConstructorWithOpt, 1 )

MJS_VERIFY_OBJECT( mozjs::ImageData );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ImageData>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<ImageData>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<ImageData>::PrototypeId = JsPrototypeId::New_ImageData;
const JSNative JsObjectTraits<ImageData>::JsConstructor = ::ImageData_Constructor;

ImageData::ImageData( JSContext* cx, JS::HandleObject pixels, uint32_t width, uint32_t height )
    : pJsCtx_( cx )
    , width_( width )
    , height_( height )
    , pixels_( pixels )
{
}

ImageData::~ImageData()
{
}

std::unique_ptr<ImageData>
ImageData::CreateNative( JSContext* cx, JS::HandleObject pixels, uint32_t width, uint32_t height )
{
    return std::unique_ptr<ImageData>( new ImageData( cx, pixels, width, height ) );
}

size_t ImageData::GetInternalSize() const
{
    return 0;
}

std::vector<uint8_t> ImageData::GetDataCopy() const
{
    // TODO: replace with actual clamped array everywhere after updating mozjs
    size_t arraySize = 0;
    bool isShared = false;
    uint8_t* data = nullptr;
    js::GetUint8ClampedArrayLengthAndData( pixels_, &arraySize, &isShared, &data );
    return { data, data + arraySize };
}

void ImageData::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<ImageData>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->pixels_, "Heap: ImageData: pixels" );
}

uint32_t ImageData::GetHeight() const
{
    return height_;
}

uint32_t ImageData::GetWidth() const
{
    return width_;
}

JSObject* ImageData::Constructor_Fake( JSContext* /*cx*/, JS::HandleValue /*arg1*/, uint32_t /*arg2*/, uint32_t /*arg3*/ )
{
    assert( false );
    return nullptr;
}

JSObject* ImageData::Constructor_1( JSContext* cx, uint32_t sw, uint32_t sh )
{
    JS::RootedObject jsArray( cx, JS_NewUint8ClampedArray( cx, 4 * sw * sh ) );
    smp::JsException::ExpectTrue( jsArray );

    return Constructor_2( cx, jsArray, sw, sh );
}

JSObject* ImageData::Constructor_2( JSContext* cx, JS::HandleObject data, uint32_t sw, uint32_t sh )
{
    return JsObjectBase<ImageData>::CreateJs( cx, data, sw, sh );
}

JSObject* ImageData::ConstructorWithOpt( JSContext* cx, size_t optArgCount, JS::HandleValue arg1, uint32_t arg2, uint32_t arg3 )
{
    if ( arg1.isObject() )
    {
        JS::RootedObject data( cx, &arg1.toObject() );
        qwr::QwrException::ExpectTrue( JS_IsUint8ClampedArray( data ), "Argument 1 is not Uint8ClampedArray" );

        size_t arraySize = 0;
        bool bDummy = false;
        uint8_t* iDummy = nullptr;
        js::GetUint8ClampedArrayLengthAndData( data, &arraySize, &bDummy, &iDummy );

        qwr::QwrException::ExpectTrue( arg2, "The source width is zero or not a number" );
        const auto width = arg2;

        qwr::QwrException::ExpectTrue( !( arraySize % 4 ), "The input data length is not a multiple of 4" );
        qwr::QwrException::ExpectTrue( !( arraySize % ( width * 4 ) ), "The input data length is not a multiple of (4 * width)" );

        switch ( optArgCount )
        {
        case 0:
        {
            qwr::QwrException::ExpectTrue( arg3, "The source height is zero or not a number" );
            const auto height = arg3;

            qwr::QwrException::ExpectTrue( !( arraySize % ( height * width * 4 ) ), "The input data length is not a multiple of (4 * width * height)" );
            return Constructor_2( cx, data, width, height );
        }
        case 1:
        {
            const auto height = arraySize / ( width * 4 );
            return Constructor_2( cx, data, width, height );
        }
        default:
            throw qwr::QwrException( "{} is not a valid argument count for any overload", 3 - optArgCount );
        }
    }
    else
    {
        const auto width = convert::to_native::ToValue<uint32_t>( cx, arg1 );
        qwr::QwrException::ExpectTrue( width, "The source width is zero or not a number" );
        qwr::QwrException::ExpectTrue( arg2, "The source height is zero or not a number" );

        switch ( optArgCount )
        {
        case 1:
            return Constructor_1( cx, width, arg2 );
        default:
            throw qwr::QwrException( "{} is not a valid argument count for any overload", 3 - optArgCount );
        }
    }
}

JSObject* ImageData::get_Data() const
{
    return pixels_;
}

uint32_t ImageData::get_Height() const
{
    return GetHeight();
}

uint32_t ImageData::get_Width() const
{
    return GetWidth();
}

} // namespace mozjs
