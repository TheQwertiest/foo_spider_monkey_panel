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
    nullptr
};

JSClass jsClass = {
    "PlaybackStopEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_reason, mozjs::PlaybackStopEvent::get_Reason )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "reason", get_reason, kDefaultPropsFlags ),
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

PlaybackStopEvent::PlaybackStopEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

PlaybackStopEvent::PlaybackStopEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : PlaybackStopEvent( cx,
                         type,
                         options.ToDefaultProps() )
{
}

std::unique_ptr<PlaybackStopEvent>
PlaybackStopEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<PlaybackStopEvent>( new mozjs::PlaybackStopEvent( cx, type, props ) );
}

std::unique_ptr<PlaybackStopEvent>
PlaybackStopEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<PlaybackStopEvent>( new mozjs::PlaybackStopEvent( cx, type, options ) );
}

size_t PlaybackStopEvent::GetInternalSize() const
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

qwr::u8string PlaybackStopEvent::get_Reason() const
{
    return props_.reason;
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

    utils::OptionalPropertyTo( cx, jsOptions, "reason", parsedOptions.reason );

    return parsedOptions;
}

PlaybackStopEvent::EventProperties PlaybackStopEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .reason = reason,
    };
}

} // namespace mozjs
