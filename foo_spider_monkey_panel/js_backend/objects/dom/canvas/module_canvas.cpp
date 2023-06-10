#include <stdafx.h>

#include "module_canvas.h"

#include <convert/js_to_native.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas.h>
#include <js_backend/objects/dom/canvas/image_bitmap.h>
#include <js_backend/objects/dom/canvas/image_data.h>

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
    JsObjectBase<ModuleCanvas>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ModuleCanvas",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( createImageBitmap, ModuleCanvas::CreateImageBitmap2, ModuleCanvas::CreateImageBitmapWithOpt, 5 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "createImageBitmap", createImageBitmap, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::ModuleCanvas );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ModuleCanvas>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<ModuleCanvas>::JsFunctions = jsFunctions.data();
const PostJsCreateFn JsObjectTraits<ModuleCanvas>::PostCreate = ModuleCanvas::PostCreate;

ModuleCanvas::ModuleCanvas( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<ModuleCanvas>
ModuleCanvas::CreateNative( JSContext* cx )
{
    return std::unique_ptr<ModuleCanvas>( new ModuleCanvas( cx ) );
}

size_t ModuleCanvas::GetInternalSize()
{
    return 0;
}

void ModuleCanvas::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::Canvas>>( cx, self, JsPrototypeId::New_Canvas );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::ImageData>>( cx, self, JsPrototypeId::New_ImageData );
}

JSObject* ModuleCanvas::CreateImageBitmap1( JS::HandleValue image, JS::HandleValue options )
{
    return ImageBitmap::CreateImageBitmap1( pJsCtx_, image, options );
}

JSObject* ModuleCanvas::CreateImageBitmap2( JS::HandleValue image,
                                            JS::HandleValue sxValue, int32_t sy,
                                            int32_t sw, int32_t sh, JS::HandleValue options )
{
    const auto sx = convert::to_native::ToValue<int32_t>( pJsCtx_, sxValue );
    return ImageBitmap::CreateImageBitmap2( pJsCtx_, image, sx, sy, sw, sh, options );
}

JSObject* ModuleCanvas::CreateImageBitmapWithOpt( size_t optArgCount, JS::HandleValue image,
                                                  JS::HandleValue arg1, int32_t arg2,
                                                  int32_t arg3, int32_t arg4,
                                                  JS::HandleValue arg5 )
{
    switch ( optArgCount )
    {
    case 0:
        return CreateImageBitmap2( image, arg1, arg2, arg3, arg4, arg5 );
    case 1:
        return CreateImageBitmap2( image, arg1, arg2, arg3, arg4 );
    case 4:
        return CreateImageBitmap1( image, arg1 );
    case 5:
        return CreateImageBitmap1( image );
    default:
        throw qwr::QwrException( "{} is not a valid argument count for any overload", 6 - optArgCount );
    }
}

} // namespace mozjs
