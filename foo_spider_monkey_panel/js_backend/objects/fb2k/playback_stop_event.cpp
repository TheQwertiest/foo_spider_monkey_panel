#include <stdafx.h>

#include "playback_stop_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_property_helper.h>

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

MJS_VERIFY_OBJECT( mozjs::PlaybackStopEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaybackStopEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PlaybackStopEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PlaybackStopEvent>::PrototypeId = JsPrototypeId::New_PlaybackStopEvent;
const JSNative JsObjectTraits<PlaybackStopEvent>::JsConstructor = ::PlaybackStopEvent_Constructor;

PlaybackStopEvent::PlaybackStopEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : JsEvent( cx, type, options.baseOptions )
    , pJsCtx_( cx )
    , stopReason_( options.reason )
{
}

std::unique_ptr<PlaybackStopEvent>
PlaybackStopEvent::CreateNative( JSContext* cx, const qwr::u8string& type, play_control::t_stop_reason reason )
{
    // TODO: replace with options.ToDefaultProps()?
    EventOptions options{
        .baseOptions{ .cancelable = false },
        .reason = reason
    };

    return CreateNative( cx, type, options );
}

std::unique_ptr<PlaybackStopEvent>
PlaybackStopEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<PlaybackStopEvent>( new mozjs::PlaybackStopEvent( cx, type, options ) );
}

size_t PlaybackStopEvent::GetInternalSize()
{
    return 0;
}

JSObject* PlaybackStopEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<PlaybackStopEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* PlaybackStopEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, type, options );
    case 1:
        return Constructor( cx, type );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint8_t PlaybackStopEvent::get_Reason() const
{
    return stopReason_;
}

PlaybackStopEvent::EventOptions PlaybackStopEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );

    if ( const auto propOpt =
             utils::GetOptionalProperty<int32_t>(
                 cx, jsOptions, "reason" ) )
    {
        const auto reasonRaw = *propOpt;
        qwr::QwrException::ExpectTrue( reasonRaw >= playback_control::stop_reason_user && reasonRaw <= playback_control::stop_reason_shutting_down,
                                       "Unknown stop reason: {}",
                                       reasonRaw );

        parsedOptions.reason = static_cast<play_control::t_stop_reason>( reasonRaw );
    }

    return parsedOptions;
}

} // namespace mozjs
