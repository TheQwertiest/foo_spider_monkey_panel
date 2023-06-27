#include <stdafx.h>

#include "playlist_track_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track_list.h>
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
    JsObjectBase<mozjs::PlaylistTrackEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaylistTrackEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_trackIndex, mozjs::PlaylistTrackEvent::get_TrackIndex )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "trackIndex", get_trackIndex, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaybackStopEvent_Constructor, PlaylistTrackEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::PlaylistTrackEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaylistTrackEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PlaylistTrackEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PlaylistTrackEvent>::PrototypeId = JsPrototypeId::New_PlaylistTrackEvent;
const JSNative JsObjectTraits<PlaylistTrackEvent>::JsConstructor = ::PlaybackStopEvent_Constructor;

PlaylistTrackEvent::PlaylistTrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : PlaylistEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

PlaylistTrackEvent::PlaylistTrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : PlaylistTrackEvent( cx,
                          type,
                          options.ToDefaultProps() )
{
}

std::unique_ptr<PlaylistTrackEvent>
PlaylistTrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<PlaylistTrackEvent>( new mozjs::PlaylistTrackEvent( cx, type, props ) );
}

std::unique_ptr<PlaylistTrackEvent>
PlaylistTrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<PlaylistTrackEvent>( new mozjs::PlaylistTrackEvent( cx, type, options ) );
}

size_t PlaylistTrackEvent::GetInternalSize() const
{
    return 0;
}

JSObject* PlaylistTrackEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<PlaylistTrackEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* PlaylistTrackEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

uint32_t PlaylistTrackEvent::get_TrackIndex() const
{
    return props_.trackIndex;
}

PlaylistTrackEvent::EventOptions PlaylistTrackEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );
    utils::OptionalPropertyTo( cx, jsOptions, "trackIndex", parsedOptions.trackIndex );

    return parsedOptions;
}

PlaylistTrackEvent::EventProperties PlaylistTrackEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .trackIndex = trackIndex,
    };
}

} // namespace mozjs
