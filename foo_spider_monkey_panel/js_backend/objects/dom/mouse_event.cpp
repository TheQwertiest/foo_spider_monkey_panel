#include <stdafx.h>

#include "mouse_event.h"

#include <events/mouse_event.h>
#include <js_backend/engine/js_to_native_invoker.h>

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
/*
MJS_DEFINE_JS_FN_FROM_NATIVE( GetModifierState, mozjs::MouseEvent::GetModifierState )
*/
constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        // JS_FN( "getModifierState", GetModifierState, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

/*
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_AltKey, mozjs::MouseEvent::get_AltKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Button, mozjs::MouseEvent::get_Button )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Buttons, mozjs::MouseEvent::get_Buttons )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ClientX, mozjs::MouseEvent::get_ClientX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ClientY, mozjs::MouseEvent::get_ClientY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_CtrlKey, mozjs::MouseEvent::get_CtrlKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LayerX, mozjs::MouseEvent::get_LayerX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_LayerY, mozjs::MouseEvent::get_LayerY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_MetaKey, mozjs::MouseEvent::get_MetaKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_MovementX, mozjs::MouseEvent::get_MovementX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_MovementY, mozjs::MouseEvent::get_MovementY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_OffsetX, mozjs::MouseEvent::get_OffsetX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_OffsetY, mozjs::MouseEvent::get_OffsetY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_PageX, mozjs::MouseEvent::get_PageX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_PageY, mozjs::MouseEvent::get_PageY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_RelatedTarget, mozjs::MouseEvent::get_RelatedTarget )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ScreenX, mozjs::MouseEvent::get_ScreenX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ScreenY, mozjs::MouseEvent::get_ScreenY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ShiftKey, mozjs::MouseEvent::get_ShiftKey )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_X, mozjs::MouseEvent::get_X )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_Y, mozjs::MouseEvent::get_Y )
*/
constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        // JS_PSG( "altKey", Get_AltKey, kDefaultPropsFlags ),
        // JS_PSG( "button", Get_Button, kDefaultPropsFlags ),
        // JS_PSG( "buttons", Get_Buttons, kDefaultPropsFlags ),
        // JS_PSG( "clientX", Get_ClientX, kDefaultPropsFlags ),
        // JS_PSG( "clientY", Get_ClientY, kDefaultPropsFlags ),
        // JS_PSG( "ctrlKey", Get_CtrlKey, kDefaultPropsFlags ),
        // JS_PSG( "metaKey", Get_MetaKey, kDefaultPropsFlags ),
        // JS_PSG( "movementX", Get_MovementX, kDefaultPropsFlags ),
        // JS_PSG( "movementY", Get_MovementY, kDefaultPropsFlags ),
        // JS_PSG( "offsetX", Get_OffsetX, kDefaultPropsFlags ),
        // JS_PSG( "offsetY", Get_OffsetY, kDefaultPropsFlags ),
        // JS_PSG( "pageX", Get_PageX, kDefaultPropsFlags ),
        // JS_PSG( "pageY", Get_PageY, kDefaultPropsFlags ),
        // JS_PSG( "relatedTarget", Get_RelatedTarget, kDefaultPropsFlags ),
        // JS_PSG( "screenX", Get_ScreenX, kDefaultPropsFlags ),
        // JS_PSG( "screenY", Get_ScreenY, kDefaultPropsFlags ),
        // JS_PSG( "shiftKey", Get_ShiftKey, kDefaultPropsFlags ),
        // JS_PSG( "x", Get_X, kDefaultPropsFlags ),
        // JS_PSG( "y", Get_Y, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( PlaybackStopEvent_Constructor, MouseEvent::Constructor )

} // namespace

namespace mozjs
{

const JSClass MouseEvent::JsClass = jsClass;
const JSFunctionSpec* MouseEvent::JsFunctions = jsFunctions.data();
const JSPropertySpec* MouseEvent::JsProperties = jsProperties.data();
const JsPrototypeId MouseEvent::BasePrototypeId = JsPrototypeId::Event;
const JsPrototypeId MouseEvent::ParentPrototypeId = JsPrototypeId::Event;
const JsPrototypeId MouseEvent::PrototypeId = JsPrototypeId::New_PlaybackStopEvent;
const JSNative MouseEvent::JsConstructor = ::PlaybackStopEvent_Constructor;

MouseEvent::MouseEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : JsEvent( cx, type, options.baseOptions )
    , pJsCtx_( cx )
{
}

std::unique_ptr<MouseEvent>
MouseEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const smp::MouseEventNew& event )
{
    const auto modifiers = event.GetModifiers();
    const auto buttons = event.GetButtons();
    const auto button = event.GetButton();

    EventOptions options{
        .baseOptions{
            .cancelable = true },
        .screenX = 1,
        .screenY = 1,
        .clientX = 1,
        .clientY = 1,
        .ctrlKey = !!qwr::to_underlying( modifiers & smp::MouseEventNew::ModifierKeyFlag::kCtrl ),
        .shiftKey = !!qwr::to_underlying( modifiers & smp::MouseEventNew::ModifierKeyFlag::kShift ),
        .altKey = !!qwr::to_underlying( modifiers & smp::MouseEventNew::ModifierKeyFlag::kAlt ),
        .metaKey = !!qwr::to_underlying( modifiers & smp::MouseEventNew::ModifierKeyFlag::kWin ),
        .button = [&] {
            // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/button
                using enum smp::MouseEventNew::MouseKeyFlag;
            switch ( button )
            {
            case kNoButtons:
            case kPrimary:
                return 0;
            case kSecondary:
                return 1;
            case kAuxiliary:
                return 2;
            case k4:
                return 3;
            case k5:
                return 4;
            default:
                throw qwr::QwrException( "Internal error: unexpected button value: {}", qwr::to_underlying(button) );
            } }(),
        .buttons = [&] {
            // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/buttons
            int32_t buttons_raw = 0;

            using enum smp::MouseEventNew::MouseKeyFlag;
            if ( qwr::to_underlying( buttons & smp::MouseEventNew::MouseKeyFlag::kPrimary ) )
            {
                buttons_raw |= 1;
            }
            if ( qwr::to_underlying( buttons & smp::MouseEventNew::MouseKeyFlag::kSecondary ) )
            {
                buttons_raw |= 2;
            }
            if ( qwr::to_underlying( buttons & smp::MouseEventNew::MouseKeyFlag::kAuxiliary ) )
            {
                buttons_raw |= 4;
            }
            if ( qwr::to_underlying( buttons & smp::MouseEventNew::MouseKeyFlag::k4 ) )
            {
                buttons_raw |= 8;
            }
            if ( qwr::to_underlying( buttons & smp::MouseEventNew::MouseKeyFlag::k5 ) )
            {
                buttons_raw |= 16;
            }
             
                return buttons_raw; }()
    };

    return CreateNative( cx, type, options );
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

uint8_t MouseEvent::get_Reason()
{
    return stopReason_;
}

MouseEvent::EventOptions MouseEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions{};

    // TODO: todo

    return parsedOptions;
}

} // namespace mozjs
