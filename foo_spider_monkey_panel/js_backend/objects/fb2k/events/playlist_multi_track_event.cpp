#include <stdafx.h>

#include "playlist_multi_track_event.h"

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
    JsObjectBase<mozjs::PlaylistMultiTrackEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaylistMultiTrackEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_trackIndices, mozjs::PlaylistMultiTrackEvent::get_TrackIndices )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "trackIndices", get_trackIndices, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaylistMultiTrackEvent_Constructor, PlaylistMultiTrackEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::PlaylistMultiTrackEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaylistMultiTrackEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PlaylistMultiTrackEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PlaylistMultiTrackEvent>::PrototypeId = JsPrototypeId::New_PlaylistMultiTrackEvent;
const JSNative JsObjectTraits<PlaylistMultiTrackEvent>::JsConstructor = ::PlaylistMultiTrackEvent_Constructor;

PlaylistMultiTrackEvent::PlaylistMultiTrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : PlaylistEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

PlaylistMultiTrackEvent::PlaylistMultiTrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : PlaylistMultiTrackEvent( cx,
                               type,
                               options.ToDefaultProps() )
{
}

std::unique_ptr<PlaylistMultiTrackEvent>
PlaylistMultiTrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<PlaylistMultiTrackEvent>( new mozjs::PlaylistMultiTrackEvent( cx, type, props ) );
}

std::unique_ptr<PlaylistMultiTrackEvent>
PlaylistMultiTrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<PlaylistMultiTrackEvent>( new mozjs::PlaylistMultiTrackEvent( cx, type, options ) );
}

size_t PlaylistMultiTrackEvent::GetInternalSize() const
{
    return 0;
}

JSObject* PlaylistMultiTrackEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<PlaylistMultiTrackEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* PlaylistMultiTrackEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

const std::vector<uint32_t>& PlaylistMultiTrackEvent::get_TrackIndices() const
{
    return props_.trackIndices;
}

PlaylistMultiTrackEvent::EventOptions PlaylistMultiTrackEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );
    utils::OptionalPropertyTo( cx, jsOptions, "trackIndices", parsedOptions.trackIndices );

    return parsedOptions;
}

PlaylistMultiTrackEvent::EventProperties PlaylistMultiTrackEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .trackIndices = trackIndices,
    };
}

} // namespace mozjs
