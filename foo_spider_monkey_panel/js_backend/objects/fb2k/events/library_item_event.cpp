#include <stdafx.h>

#include "library_item_event.h"

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
    JsObjectBase<mozjs::LibraryTrackEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "LibraryTrackEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_tracks, mozjs::LibraryTrackEvent::get_Tracks )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "tracks", get_tracks, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( LibraryTrackEvent_Constructor, LibraryTrackEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::LibraryTrackEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<LibraryTrackEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<LibraryTrackEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<LibraryTrackEvent>::PrototypeId = JsPrototypeId::New_LibraryTrackEvent;
const JSNative JsObjectTraits<LibraryTrackEvent>::JsConstructor = ::LibraryTrackEvent_Constructor;

LibraryTrackEvent::LibraryTrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

LibraryTrackEvent::LibraryTrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : LibraryTrackEvent( cx,
                         type,
                         options.ToDefaultProps() )
{
}

std::unique_ptr<LibraryTrackEvent>
LibraryTrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<LibraryTrackEvent>( new mozjs::LibraryTrackEvent( cx, type, props ) );
}

std::unique_ptr<LibraryTrackEvent>
LibraryTrackEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<LibraryTrackEvent>( new mozjs::LibraryTrackEvent( cx, type, options ) );
}

size_t LibraryTrackEvent::GetInternalSize() const
{
    return 0;
}

JSObject* LibraryTrackEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<LibraryTrackEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* LibraryTrackEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

JSObject* LibraryTrackEvent::get_Tracks() const
{
    return TrackList::CreateJs( pJsCtx_, props_.handles );
}

LibraryTrackEvent::EventOptions LibraryTrackEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
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
    utils::OptionalPropertyTo( cx, jsOptions, "tracks", pTrackList );
    if ( pTrackList )
    {
        parsedOptions.handles = pTrackList->GetHandleList();
    }

    return parsedOptions;
}

LibraryTrackEvent::EventProperties LibraryTrackEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .handles = handles,
    };
}

} // namespace mozjs
