#include <stdafx.h>

#include "placeholder.h"

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
    PlaceHolder::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaceHolder",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::PlaceHolder );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaceHolder>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<PlaceHolder>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<PlaceHolder>::JsProperties = jsProperties.data();

PlaceHolder::PlaceHolder( JSContext* cx )
    : pJsCtx_( cx )
{
}

PlaceHolder::~PlaceHolder()
{
}

std::unique_ptr<PlaceHolder>
PlaceHolder::CreateNative( JSContext* cx )
{
    return std::unique_ptr<PlaceHolder>( new PlaceHolder( cx ) );
}

size_t PlaceHolder::GetInternalSize() const
{
    return 0;
}

JSObject* PlaceHolder::Constructor( JSContext* cx )
{
    return JsObjectBase<PlaceHolder>::CreateJs( cx );
}

} // namespace mozjs
