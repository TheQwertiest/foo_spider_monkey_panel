#include <stdafx.h>

#include "track_event.h"

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
    JsObjectBase<mozjs::TrackEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "TrackEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_affectedTracks, mozjs::TrackEvent::get_AffectedTracks )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "affectedTracks", get_affectedTracks, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaybackStopEvent_Constructor, TrackEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::TrackEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<TrackEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<TrackEvent>::PrototypeId = JsPrototypeId::New_TrackEvent;
const JSNative JsObjectTraits<TrackEvent>::JsConstructor = ::PlaybackStopEvent_Constructor;

TrackEvent::TrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

TrackEvent::TrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : TrackEvent( cx,
                  type,
                  options.ToDefaultProps() )
{
}

std::unique_ptr<TrackEvent>
TrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<TrackEvent>( new mozjs::TrackEvent( cx, type, props ) );
}

std::unique_ptr<TrackEvent>
TrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<TrackEvent>( new mozjs::TrackEvent( cx, type, options ) );
}

size_t TrackEvent::GetInternalSize() const
{
    return 0;
}

JSObject* TrackEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<TrackEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* TrackEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

JSObject* TrackEvent::get_AffectedTracks() const
{
    return TrackList::CreateJs( pJsCtx_, props_.affectedTracks );
}

TrackEvent::EventOptions TrackEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );
    TrackList* pTrackList = nullptr;
    utils::OptionalPropertyTo( cx, jsOptions, "affectedTracks", pTrackList );
    if ( pTrackList )
    {
        parsedOptions.affectedTracks = pTrackList->GetHandleList();
    }

    return parsedOptions;
}

TrackEvent::EventProperties TrackEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .affectedTracks = affectedTracks,
    };
}

} // namespace mozjs
