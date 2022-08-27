#include <stdafx.h>

#include "playback_stop_event.h"

#include <events/playback_stop_event.h>
#include <js_backend/engine/js_to_native_invoker.h>

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
    JsObjectBase<mozjs::PlaybackStopEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaybackStopEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Reason, mozjs::PlaybackStopEvent::get_Reason )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "reason", Get_Reason, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaybackStopEvent_Constructor, PlaybackStopEvent::Constructor )

} // namespace

namespace mozjs
{

const JSClass PlaybackStopEvent::JsClass = jsClass;
const JSPropertySpec* PlaybackStopEvent::JsProperties = jsProperties.data();
const JsPrototypeId PlaybackStopEvent::BasePrototypeId = JsPrototypeId::Event;
const JsPrototypeId PlaybackStopEvent::ParentPrototypeId = JsPrototypeId::Event;
const JsPrototypeId PlaybackStopEvent::PrototypeId = JsPrototypeId::New_PlaybackStopEvent;
const JSNative PlaybackStopEvent::JsConstructor = ::PlaybackStopEvent_Constructor;

PlaybackStopEvent::PlaybackStopEvent( JSContext* cx, play_control::t_stop_reason stopReason )
    : JsEvent( cx, "stop", false )
    , pJsCtx_( cx )
    , stopReason_( stopReason )
{
}

std::unique_ptr<PlaybackStopEvent>
PlaybackStopEvent::CreateNative( JSContext* cx, const smp::PlaybackStopEvent& event )
{
    assert( event.GetId() == smp::EventId::kNew_FbPlaybackStop );
    return CreateNative( cx, event.Reason() );
}

std::unique_ptr<PlaybackStopEvent>
PlaybackStopEvent::CreateNative( JSContext* cx, play_control::t_stop_reason stopReason )
{
    return std::unique_ptr<PlaybackStopEvent>( new mozjs::PlaybackStopEvent( cx, stopReason ) );
}

size_t PlaybackStopEvent::GetInternalSize()
{
    return 0;
}

JSObject* PlaybackStopEvent::Constructor( JSContext* cx, int32_t reason )
{
    qwr::QwrException::ExpectTrue( reason >= playback_control::stop_reason_user && reason <= playback_control::stop_reason_shutting_down,
                                   "Unknown stop reason: {}",
                                   reason );

    return JsObjectBase<PlaybackStopEvent>::CreateJs( cx, static_cast<play_control::t_stop_reason>( reason ) );
}

uint8_t PlaybackStopEvent::get_Reason()
{
    return stopReason_;
}

} // namespace mozjs
