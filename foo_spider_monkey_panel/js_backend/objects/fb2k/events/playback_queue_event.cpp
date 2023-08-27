#include <stdafx.h>

#include "playback_queue_event.h"

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
    JsObjectBase<mozjs::PlaybackQueueEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaybackQueueEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_origin, mozjs::PlaybackQueueEvent::get_Origin )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "origin", get_origin, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaybackQueueEvent_Constructor, PlaybackQueueEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::PlaybackQueueEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaybackQueueEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PlaybackQueueEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PlaybackQueueEvent>::PrototypeId = JsPrototypeId::New_PlaybackQueueEvent;
const JSNative JsObjectTraits<PlaybackQueueEvent>::JsConstructor = ::PlaybackQueueEvent_Constructor;

PlaybackQueueEvent::PlaybackQueueEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

PlaybackQueueEvent::PlaybackQueueEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : PlaybackQueueEvent( cx,
                          type,
                          options.ToDefaultProps() )
{
}

std::unique_ptr<PlaybackQueueEvent>
PlaybackQueueEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<PlaybackQueueEvent>( new mozjs::PlaybackQueueEvent( cx, type, props ) );
}

std::unique_ptr<PlaybackQueueEvent>
PlaybackQueueEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<PlaybackQueueEvent>( new mozjs::PlaybackQueueEvent( cx, type, options ) );
}

size_t PlaybackQueueEvent::GetInternalSize() const
{
    return 0;
}

JSObject* PlaybackQueueEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<PlaybackQueueEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* PlaybackQueueEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

qwr::u8string PlaybackQueueEvent::get_Origin() const
{
    return props_.origin;
}

PlaybackQueueEvent::EventOptions PlaybackQueueEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );

    utils::OptionalPropertyTo( cx, jsOptions, "origin", parsedOptions.origin );

    return parsedOptions;
}

PlaybackQueueEvent::EventProperties PlaybackQueueEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .origin = origin,
    };
}

} // namespace mozjs
