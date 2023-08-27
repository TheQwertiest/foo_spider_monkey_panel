#include <stdafx.h>

#include "keyboard_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/events/keyboard_event.h>

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

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_AltKey, mozjs::KeyboardEvent::get_AltKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Code, mozjs::KeyboardEvent::get_Code )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_CtrlKey, mozjs::KeyboardEvent::get_CtrlKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_IsComposing, mozjs::KeyboardEvent::get_IsComposing )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Key, mozjs::KeyboardEvent::get_Key )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_MetaKey, mozjs::KeyboardEvent::get_MetaKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Location, mozjs::KeyboardEvent::get_Location )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Repeat, mozjs::KeyboardEvent::get_Repeat )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ShiftKey, mozjs::KeyboardEvent::get_ShiftKey )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "altKey", Get_AltKey, kDefaultPropsFlags ),
        JS_PSG( "code", Get_Code, kDefaultPropsFlags ),
        JS_PSG( "ctrlKey", Get_CtrlKey, kDefaultPropsFlags ),
        JS_PSG( "isComposing", Get_IsComposing, kDefaultPropsFlags ),
        JS_PSG( "key", Get_Key, kDefaultPropsFlags ),
        JS_PSG( "metaKey", Get_MetaKey, kDefaultPropsFlags ),
        JS_PSG( "location", Get_Location, kDefaultPropsFlags ),
        JS_PSG( "repeat", Get_Repeat, kDefaultPropsFlags ),
        JS_PSG( "shiftKey", Get_ShiftKey, kDefaultPropsFlags ),
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

bool KeyboardEvent::get_AltKey() const
{
    return props_.altKey;
}

const qwr::u8string& KeyboardEvent::get_Code() const
{
    return props_.code;
}

bool KeyboardEvent::get_CtrlKey() const
{
    return props_.ctrlKey;
}

bool KeyboardEvent::get_IsComposing() const
{
    return false;
}

const std::wstring& KeyboardEvent::get_Key() const
{
    return props_.key;
}

bool KeyboardEvent::get_MetaKey() const
{
    return props_.metaKey;
}

uint32_t KeyboardEvent::get_Location() const
{
    return props_.location;
}

bool KeyboardEvent::get_Repeat() const
{
    return props_.repeat;
}

bool KeyboardEvent::get_ShiftKey() const
{
    return props_.shiftKey;
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

    utils::OptionalPropertyTo( cx, jsOptions, "altKey", parsedOptions.altKey );
    utils::OptionalPropertyTo( cx, jsOptions, "ctrlKey", parsedOptions.ctrlKey );
    utils::OptionalPropertyTo( cx, jsOptions, "shiftKey", parsedOptions.shiftKey );
    utils::OptionalPropertyTo( cx, jsOptions, "metaKey", parsedOptions.metaKey );
    utils::OptionalPropertyTo( cx, jsOptions, "code", parsedOptions.code );
    utils::OptionalPropertyTo( cx, jsOptions, "key", parsedOptions.key );
    utils::OptionalPropertyTo( cx, jsOptions, "location", parsedOptions.location );
    utils::OptionalPropertyTo( cx, jsOptions, "repeat", parsedOptions.repeat );

    return parsedOptions;
}

KeyboardEvent::EventProperties KeyboardEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .altKey = altKey,
        .ctrlKey = ctrlKey,
        .metaKey = metaKey,
        .shiftKey = shiftKey,
        .code = code,
        .key = key,
        .location = location,
        .repeat = repeat,
    };
}

} // namespace mozjs
