#include <stdafx.h>

#include "keyboard_event.h"

#include <events/keyboard_event.h>
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
    JsObjectBase<mozjs::KeyboardEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "KeyboardEvent",
    kDefaultClassFlags,
    &jsOps
};

// TODO: https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/getModifierState
/*
MJS_DEFINE_JS_FN_FROM_NATIVE( GetModifierState, mozjs::KeyboardEvent::GetModifierState )
*/
constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        // JS_FN( "getModifierState", GetModifierState, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Code, mozjs::KeyboardEvent::get_Code )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Key, mozjs::KeyboardEvent::get_Key )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "code", Get_Code, kDefaultPropsFlags ),
        JS_PSG( "key", Get_Key, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( KeyboardEvent_Constructor, KeyboardEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::KeyboardEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<KeyboardEvent>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<KeyboardEvent>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<KeyboardEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<KeyboardEvent>::PrototypeId = JsPrototypeId::New_KeyboardEvent;
const JSNative JsObjectTraits<KeyboardEvent>::JsConstructor = ::KeyboardEvent_Constructor;

KeyboardEvent::KeyboardEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

KeyboardEvent::KeyboardEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : KeyboardEvent( cx,
                     type,
                     options.ToDefaultProps() )
{
}

std::unique_ptr<KeyboardEvent>
KeyboardEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<KeyboardEvent>( new mozjs::KeyboardEvent( cx, type, props ) );
}

std::unique_ptr<KeyboardEvent>
KeyboardEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<KeyboardEvent>( new mozjs::KeyboardEvent( cx, type, options ) );
}

size_t KeyboardEvent::GetInternalSize()
{
    return 0;
}

JSObject* KeyboardEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<KeyboardEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* KeyboardEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

const qwr::u8string& KeyboardEvent::get_Code() const
{
    return props_.code;
}

const qwr::u8string& KeyboardEvent::get_Key() const
{
    return props_.key;
}

KeyboardEvent::EventOptions KeyboardEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;

    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );

    utils::OptionalPropertyTo( cx, jsOptions, "key", parsedOptions.key );
    utils::OptionalPropertyTo( cx, jsOptions, "code", parsedOptions.code );

    return parsedOptions;
}

KeyboardEvent::EventProperties KeyboardEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .key = key,
        .code = code,
    };
}

} // namespace mozjs
