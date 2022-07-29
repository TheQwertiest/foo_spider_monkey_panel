#include <stdafx.h>

#include "js_event.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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
    JsEvent::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Event",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_timeStamp, JsEvent::get_TimeStamp );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_type, JsEvent::get_Type );

constexpr auto jsProperties = std::to_array<JSPropertySpec>( {
    JS_PSG( "type", get_type, kDefaultPropsFlags ),
    JS_PSG( "timeStamp", get_timeStamp, kDefaultPropsFlags ),
    JS_PS_END,
} );

MJS_DEFINE_JS_FN_FROM_NATIVE( JsEvent_Constructor, JsEvent::Constructor );

} // namespace

namespace mozjs
{

const JSClass JsEvent::JsClass = jsClass;
const JSFunctionSpec* JsEvent::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsEvent::JsProperties = jsProperties.data();
const JsPrototypeId JsEvent::PrototypeId = JsPrototypeId::Event;
const JSNative JsEvent::JsConstructor = ::JsEvent_Constructor;

JsEvent::JsEvent( JSContext* cx, const qwr::u8string& type, uint64_t timeStamp )
    : pJsCtx_( cx )
    , type_( type )
    , timeStamp_( timeStamp ? timeStamp : std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() )
{
}

JSObject* JsEvent::Constructor( JSContext* cx, const qwr::u8string& type )
{
    return JsEvent::CreateJs( cx, type, 0 );
}

std::unique_ptr<mozjs::JsEvent>
JsEvent::CreateNative( JSContext* cx, const qwr::u8string& type, uint64_t timeStamp )
{
    return std::unique_ptr<JsEvent>( new JsEvent( cx, type, timeStamp ) );
}

size_t JsEvent::GetInternalSize()
{
    return type_.size();
}

uint64_t JsEvent::get_TimeStamp() const
{
    return timeStamp_;
}

const qwr::u8string& JsEvent::get_Type() const
{
    return type_;
}

} // namespace mozjs
