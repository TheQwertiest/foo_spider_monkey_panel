#include <stdafx.h>

#include "image.h"

#include <dom/css_colours.h>
#include <dom/double_helpers.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_rendering_context_2d.h>
#include <utils/gdi_error_helpers.h>

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
    Image::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "CanvasGradient",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( Image_Constructor, Image::Constructor )

MJS_VERIFY_OBJECT( mozjs::Image );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Image>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<Image>::JsProperties = jsProperties.data();
const JSFunctionSpec* JsObjectTraits<Image>::JsFunctions = jsFunctions.data();
const JsPrototypeId JsObjectTraits<Image>::PrototypeId = JsPrototypeId::New_Image;
const JSNative JsObjectTraits<Image>::JsConstructor = ::Image_Constructor;

Image::Image( JSContext* cx )
    : pJsCtx_( cx )
{
}

Image::~Image()
{
}

std::unique_ptr<Image>
Image::CreateNative( JSContext* cx )
{
    return std::unique_ptr<Image>( new Image( cx ) );
}

size_t Image::GetInternalSize() const
{
    return 0;
}

JSObject* Image::Constructor( JSContext* cx )
{
    return JsObjectBase<Image>::CreateJs( cx );
}

} // namespace mozjs
