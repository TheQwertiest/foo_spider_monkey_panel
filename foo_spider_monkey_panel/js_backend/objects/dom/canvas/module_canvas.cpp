#include <stdafx.h>

#include "module_canvas.h"

#include <js_backend/objects/dom/canvas/canvas.h>

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
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ModuleCanvas",
    kDefaultClassFlags,
    &jsOps
};

MJS_VERIFY_OBJECT( mozjs::ModuleCanvas );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ModuleCanvas>::JsClass = jsClass;
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
}

} // namespace mozjs
