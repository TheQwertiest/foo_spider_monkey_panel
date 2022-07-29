#include <stdafx.h>

#include "js_event_target.h"

#include <js_engine/js_to_native_invoker.h>
#include <utils/logging.h>

#include <chrono>

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
    JsEventTarget::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "EventTarget",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( addEventListener, JsEventTarget::AddEventListener );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeEventListener, JsEventTarget::RemoveEventListener );
MJS_DEFINE_JS_FN_FROM_NATIVE( dispatchEvent, JsEventTarget::DispatchEvent );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>( {
    JS_FN( "addEventListener", addEventListener, 2, kDefaultPropsFlags ),
    JS_FN( "removeEventListener", removeEventListener, 2, kDefaultPropsFlags ),
    JS_FN( "dispatchEvent", dispatchEvent, 1, kDefaultPropsFlags ),
    JS_FS_END,
} );

constexpr auto jsProperties = std::to_array<JSPropertySpec>( {
    JS_PS_END,
} );

MJS_DEFINE_JS_FN_FROM_NATIVE( JsEventTarget_Constructor, JsEventTarget::Constructor );

} // namespace

namespace mozjs
{

const JSClass JsEventTarget::JsClass = jsClass;
const JSFunctionSpec* JsEventTarget::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsEventTarget::JsProperties = jsProperties.data();
const JsPrototypeId JsEventTarget::PrototypeId = JsPrototypeId::EventTarget;
const JSNative JsEventTarget::JsConstructor = ::JsEventTarget_Constructor;

JsEventTarget::JsEventTarget( JSContext* cx )
    : pJsCtx_( cx )
{
}

JSObject* JsEventTarget::Constructor( JSContext* cx )
{
    return JsEventTarget::CreateJs( cx );
}

std::unique_ptr<mozjs::JsEventTarget>
JsEventTarget::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsEventTarget>( new JsEventTarget( cx ) );
}

size_t JsEventTarget::GetInternalSize()
{
    return 0;
}

void JsEventTarget::Trace( JSTracer* trc )
{
}

void JsEventTarget::PrepareForGc()
{
}

void JsEventTarget::AddEventListener( const qwr::u8string& type, JS::HandleValue listener )
{
    test_ = type;
}

void JsEventTarget::RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener )
{
    test_.clear();
}

void JsEventTarget::DispatchEvent( JS::HandleValue event )
{
    smp::utils::LogError( test_ );
}

} // namespace mozjs
