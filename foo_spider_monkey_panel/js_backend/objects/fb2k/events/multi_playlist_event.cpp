#include <stdafx.h>

#include "multi_playlist_event.h"

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
    JsObjectBase<mozjs::MultiPlaylistEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MultiPlaylistEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_playlistIndices, mozjs::MultiPlaylistEvent::get_PlaylistIndices )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "playlistIndices", get_playlistIndices, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( MultiPlaylistEvent_Constructor, MultiPlaylistEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::MultiPlaylistEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<MultiPlaylistEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<MultiPlaylistEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<MultiPlaylistEvent>::PrototypeId = JsPrototypeId::New_MultiPlaylistEvent;
const JSNative JsObjectTraits<MultiPlaylistEvent>::JsConstructor = ::MultiPlaylistEvent_Constructor;

MultiPlaylistEvent::MultiPlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

MultiPlaylistEvent::MultiPlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : MultiPlaylistEvent( cx,
                          type,
                          options.ToDefaultProps() )
{
}

std::unique_ptr<MultiPlaylistEvent>
MultiPlaylistEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<MultiPlaylistEvent>( new mozjs::MultiPlaylistEvent( cx, type, props ) );
}

std::unique_ptr<MultiPlaylistEvent>
MultiPlaylistEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<MultiPlaylistEvent>( new mozjs::MultiPlaylistEvent( cx, type, options ) );
}

size_t MultiPlaylistEvent::GetInternalSize() const
{
    return 0;
}

JSObject* MultiPlaylistEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<MultiPlaylistEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* MultiPlaylistEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

const std::vector<uint32_t>& MultiPlaylistEvent::get_PlaylistIndices() const
{
    return props_.playlistIndices;
}

MultiPlaylistEvent::EventOptions MultiPlaylistEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );
    utils::OptionalPropertyTo( cx, jsOptions, "playlistIndices", parsedOptions.playlistIndices );

    return parsedOptions;
}

MultiPlaylistEvent::EventProperties MultiPlaylistEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .playlistIndices = playlistIndices,
    };
}

} // namespace mozjs
