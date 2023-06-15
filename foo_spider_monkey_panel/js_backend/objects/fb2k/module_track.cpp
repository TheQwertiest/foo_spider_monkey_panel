#include <stdafx.h>

#include "module_track.h"

#include <convert/js_to_native.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track.h>

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
    JsObjectBase<ModuleTrack>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ModuleTrack",
    kDefaultClassFlags,
    &jsOps
};

MJS_VERIFY_OBJECT( mozjs::ModuleTrack );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ModuleTrack>::JsClass = jsClass;
const PostJsCreateFn JsObjectTraits<ModuleTrack>::PostCreate = ModuleTrack::PostCreate;

ModuleTrack::ModuleTrack( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<ModuleTrack>
ModuleTrack::CreateNative( JSContext* cx )
{
    return std::unique_ptr<ModuleTrack>( new ModuleTrack( cx ) );
}

size_t ModuleTrack::GetInternalSize()
{
    return 0;
}

void ModuleTrack::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::Track>>( cx, self, JsPrototypeId::New_Track );
}

} // namespace mozjs
