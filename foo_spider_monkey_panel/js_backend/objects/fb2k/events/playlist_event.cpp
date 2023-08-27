#include <stdafx.h>

#include "playlist_event.h"

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
    JsObjectBase<mozjs::PlaylistEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "PlaylistEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_playlistIndex, mozjs::PlaylistEvent::get_PlaylistIndex )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "playlistIndex", get_playlistIndex, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaylistEvent_Constructor, PlaylistEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::PlaylistEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaylistEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<PlaylistEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<PlaylistEvent>::PrototypeId = JsPrototypeId::New_PlaylistEvent;
const JSNative JsObjectTraits<PlaylistEvent>::JsConstructor = ::PlaylistEvent_Constructor;

PlaylistEvent::PlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

PlaylistEvent::PlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : PlaylistEvent( cx,
                     type,
                     options.ToDefaultProps() )
{
}

std::unique_ptr<PlaylistEvent>
PlaylistEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<PlaylistEvent>( new mozjs::PlaylistEvent( cx, type, props ) );
}

std::unique_ptr<PlaylistEvent>
PlaylistEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<PlaylistEvent>( new mozjs::PlaylistEvent( cx, type, options ) );
}

size_t PlaylistEvent::GetInternalSize() const
{
    return 0;
}

JSObject* PlaylistEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<PlaylistEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* PlaylistEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

uint32_t PlaylistEvent::get_PlaylistIndex() const
{
    return props_.playlistIndex;
}

PlaylistEvent::EventOptions PlaylistEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;
    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );
    utils::OptionalPropertyTo( cx, jsOptions, "playlistIndex", parsedOptions.playlistIndex );

    return parsedOptions;
}

PlaylistEvent::EventProperties PlaylistEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .playlistIndex = playlistIndex,
    };
}

} // namespace mozjs
