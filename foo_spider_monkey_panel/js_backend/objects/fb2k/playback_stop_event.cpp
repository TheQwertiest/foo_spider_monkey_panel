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

} // namespace

namespace mozjs
{

const JSClass PlaybackStopEvent::JsClass = jsClass;
const JSPropertySpec* PlaybackStopEvent::JsProperties = jsProperties.data();
const JsPrototypeId PlaybackStopEvent::BasePrototypeId = JsPrototypeId::Event;
const JsPrototypeId PlaybackStopEvent::ParentPrototypeId = JsPrototypeId::Event;

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
    return std::unique_ptr<PlaybackStopEvent>( new mozjs::PlaybackStopEvent( cx, event.Reason() ) );
}

size_t PlaybackStopEvent::GetInternalSize()
{
    return 0;
}

uint8_t PlaybackStopEvent::get_Reason()
{
    return stopReason_;
}

} // namespace mozjs
