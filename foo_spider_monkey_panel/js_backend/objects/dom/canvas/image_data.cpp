#include <stdafx.h>

#include "image_data.h"

#include <js_backend/engine/js_to_native_invoker.h>

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

MJS_DEFINE_JS_FN_FROM_NATIVE( ImageData_Constructor, ImageData::Constructor )

MJS_VERIFY_OBJECT( mozjs::ImageData );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ImageData>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<ImageData>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<ImageData>::PrototypeId = JsPrototypeId::New_ImageData;
const JSNative JsObjectTraits<ImageData>::JsConstructor = ::ImageData_Constructor;

ImageData::ImageData( JSContext* cx, uint32_t width, uint32_t height, JS::HandleObject pixels )
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
ImageData::CreateNative( JSContext* cx, uint32_t width, uint32_t height, JS::HandleObject pixels )
{
    return std::unique_ptr<ImageData>( new ImageData( cx, width, height, pixels ) );
}

size_t ImageData::GetInternalSize() const
{
    return 0;
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

JSObject* ImageData::Constructor( JSContext* cx, uint32_t width, uint32_t height, JS::HandleValue dataArray )
{
    qwr::QwrException::ExpectTrue( dataArray.isObject(), "dataArray is not an object" );

    JS::RootedObject dataArrayObject( cx, &dataArray.toObject() );
    return JsObjectBase<ImageData>::CreateJs( cx, width, height, dataArrayObject );
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
