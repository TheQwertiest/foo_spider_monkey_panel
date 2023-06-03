#include <stdafx.h>

#include "mouse_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/events/mouse_event.h>

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
    JsObjectBase<mozjs::MouseEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MouseEvent",
    kDefaultClassFlags,
    &jsOps
};

// TODO: https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/getModifierState
/*
MJS_DEFINE_JS_FN_FROM_NATIVE( GetModifierState, mozjs::MouseEvent::GetModifierState )
*/
constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        // JS_FN( "getModifierState", GetModifierState, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_AltKey, mozjs::MouseEvent::get_AltKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Button, mozjs::MouseEvent::get_Button )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Buttons, mozjs::MouseEvent::get_Buttons )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_CtrlKey, mozjs::MouseEvent::get_CtrlKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_MetaKey, mozjs::MouseEvent::get_MetaKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_RelatedTarget, mozjs::MouseEvent::get_RelatedTarget )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ScreenX, mozjs::MouseEvent::get_ScreenX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ScreenY, mozjs::MouseEvent::get_ScreenY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ShiftKey, mozjs::MouseEvent::get_ShiftKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_X, mozjs::MouseEvent::get_X )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Y, mozjs::MouseEvent::get_Y )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "altKey", Get_AltKey, kDefaultPropsFlags ),
        JS_PSG( "button", Get_Button, kDefaultPropsFlags ),
        JS_PSG( "buttons", Get_Buttons, kDefaultPropsFlags ),
        JS_PSG( "clientX", Get_X, kDefaultPropsFlags ),
        JS_PSG( "clientY", Get_Y, kDefaultPropsFlags ),
        JS_PSG( "ctrlKey", Get_CtrlKey, kDefaultPropsFlags ),
        JS_PSG( "metaKey", Get_MetaKey, kDefaultPropsFlags ),
        JS_PSG( "relatedTarget", Get_RelatedTarget, kDefaultPropsFlags ),
        JS_PSG( "screenX", Get_ScreenX, kDefaultPropsFlags ),
        JS_PSG( "screenY", Get_ScreenY, kDefaultPropsFlags ),
        JS_PSG( "shiftKey", Get_ShiftKey, kDefaultPropsFlags ),
        JS_PSG( "x", Get_X, kDefaultPropsFlags ),
        JS_PSG( "y", Get_Y, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( MouseEvent_Constructor, MouseEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::MouseEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<MouseEvent>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<MouseEvent>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<MouseEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<MouseEvent>::PrototypeId = JsPrototypeId::New_MouseEvent;
const JSNative JsObjectTraits<MouseEvent>::JsConstructor = ::MouseEvent_Constructor;

MouseEvent::MouseEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : JsEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

MouseEvent::MouseEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : MouseEvent( cx,
                  type,
                  options.ToDefaultProps() )
{
}

std::unique_ptr<MouseEvent>
MouseEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<MouseEvent>( new mozjs::MouseEvent( cx, type, props ) );
}

std::unique_ptr<MouseEvent>
MouseEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<MouseEvent>( new mozjs::MouseEvent( cx, type, options ) );
}

size_t MouseEvent::GetInternalSize()
{
    return 0;
}

JSObject* MouseEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<MouseEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* MouseEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

bool MouseEvent::get_AltKey() const
{
    return props_.altKey;
}

uint32_t MouseEvent::get_Button() const
{
    return props_.button;
}

uint32_t MouseEvent::get_Buttons() const
{
    return props_.buttons;
}

bool MouseEvent::get_CtrlKey() const
{
    return props_.ctrlKey;
}

bool MouseEvent::get_MetaKey() const
{
    return props_.altKey;
}

JSObject* MouseEvent::get_RelatedTarget() const
{
    return nullptr;
}

double MouseEvent::get_ScreenX() const
{
    return props_.screenX;
}

double MouseEvent::get_ScreenY() const
{
    return props_.screenY;
}

bool MouseEvent::get_ShiftKey() const
{
    return props_.shiftKey;
}

double MouseEvent::get_X() const
{
    return props_.x;
}

double MouseEvent::get_Y() const
{
    return props_.y;
}

MouseEvent::EventOptions MouseEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
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
    utils::OptionalPropertyTo( cx, jsOptions, "button", parsedOptions.button );
    utils::OptionalPropertyTo( cx, jsOptions, "buttons", parsedOptions.buttons );
    utils::OptionalPropertyTo( cx, jsOptions, "screenX", parsedOptions.screenX );
    utils::OptionalPropertyTo( cx, jsOptions, "screenY", parsedOptions.screenY );
    utils::OptionalPropertyTo( cx, jsOptions, "clientX", parsedOptions.clientX );
    utils::OptionalPropertyTo( cx, jsOptions, "clientY", parsedOptions.clientY );

    return parsedOptions;
}

MouseEvent::EventProperties MouseEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .altKey = altKey,
        .ctrlKey = ctrlKey,
        .metaKey = metaKey,
        .shiftKey = shiftKey,
        .button = button,
        .buttons = buttons,
        .screenX = screenX,
        .screenY = screenY,
        .x = clientX,
        .y = clientY
    };
}

} // namespace mozjs
