#include <stdafx.h>

#include "js_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/js_object_helper.h>
#include <js_backend/utils/js_property_helper.h>

#include <chrono>

using namespace smp;

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
    JsEvent::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Event",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( composedPath, JsEvent::ComposedPath );
MJS_DEFINE_JS_FN_FROM_NATIVE( preventDefault, JsEvent::PreventDefault );
MJS_DEFINE_JS_FN_FROM_NATIVE( stopImmediatePropagation, JsEvent::StopImmediatePropagation );
MJS_DEFINE_JS_FN_FROM_NATIVE( stopPropagation, JsEvent::DoNothing );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>( {
    JS_FN( "composedPath", composedPath, 0, kDefaultPropsFlags ),
    JS_FN( "preventDefault", preventDefault, 0, kDefaultPropsFlags ),
    JS_FN( "stopImmediatePropagation", stopImmediatePropagation, 0, kDefaultPropsFlags ),
    JS_FN( "stopPropagation", stopPropagation, 0, kDefaultPropsFlags ),
    JS_FS_END,
} );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_bubbles, JsEvent::get_False );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_cancelable, JsEvent::get_Cancelable );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_composed, JsEvent::get_False );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_currentTarget, JsEvent::get_Target );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_defaultPrevented, JsEvent::get_DefaultPrevented );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_eventPhase, JsEvent::get_EventPhase );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_isTrusted, JsEvent::get_False );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_target, JsEvent::get_Target );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_timeStamp, JsEvent::get_TimeStamp );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_type, JsEvent::get_Type );

constexpr auto jsProperties = std::to_array<JSPropertySpec>( {
    JS_PSG( "bubbles", get_bubbles, kDefaultPropsFlags ),
    JS_PSG( "cancelable", get_cancelable, kDefaultPropsFlags ),
    JS_PSG( "composed", get_composed, kDefaultPropsFlags ),
    JS_PSG( "currentTarget", get_currentTarget, kDefaultPropsFlags ),
    JS_PSG( "defaultPrevented", get_defaultPrevented, kDefaultPropsFlags ),
    JS_PSG( "eventPhase", get_eventPhase, kDefaultPropsFlags ),
    JS_PSG( "isTrusted", get_isTrusted, kDefaultPropsFlags ),
    JS_PSG( "target", get_target, kDefaultPropsFlags ),
    JS_PSG( "timeStamp", get_timeStamp, kDefaultPropsFlags ),
    JS_PSG( "type", get_type, kDefaultPropsFlags ),
    JS_PS_END,
} );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( JsEvent_Constructor, JsEvent::Constructor, JsEvent::ConstructorWithOpt, 1 )

} // namespace

namespace mozjs
{

const JSClass JsEvent::JsClass = jsClass;
const JSFunctionSpec* JsEvent::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsEvent::JsProperties = jsProperties.data();
const JsPrototypeId JsEvent::PrototypeId = JsPrototypeId::Event;
const JSNative JsEvent::JsConstructor = ::JsEvent_Constructor;

JsEvent::JsEvent( JSContext* cx, const qwr::u8string& type, bool isCancelable )
    : pJsCtx_( cx )
    , currentTarget_( cx )
    , isCancelable_( isCancelable )
    , type_( type )
    , timeStamp_( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() )
{
}

JsEvent::~JsEvent()
{
    // should be cleared by EventTarget after each dispatch
    assert( !currentTarget_ );
}

bool JsEvent::IsPropagationStopped() const
{
    return isPropagationStopped_;
}

bool JsEvent::HasTarget() const
{
    return !!currentTarget_;
}

void JsEvent::SetCurrentTarget( JS::HandleObject pTarget )
{
    currentTarget_ = pTarget;
}

void JsEvent::ResetPropagationStatus()
{
    isPropagationStopped_ = false;
}

JSObject* JsEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    bool isCancelable = false;
    if ( !options.isNullOrUndefined() )
    {
        qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
        JS::RootedObject jsOptions( cx, &options.toObject() );

        if ( const auto propOpt = utils::GetOptionalProperty<bool>( cx, jsOptions, "cancelable" ) )
        {
            isCancelable = *propOpt;
        }
    }

    return JsEvent::CreateJs( cx, type, isCancelable );
}

JSObject* JsEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
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

void JsEvent::DoNothing() const
{
}

JS::Value JsEvent::ComposedPath() const
{
    const auto hasTarget = !!currentTarget_;

    JS::RootedObject jsArray( pJsCtx_, JS::NewArrayObject( pJsCtx_, hasTarget ? 1 : 0 ) );
    smp::JsException::ExpectTrue( jsArray );

    if ( !hasTarget )
    {
        return JS::ObjectValue( *jsArray );
    }

    if ( !JS_SetElement( pJsCtx_, jsArray, 0, currentTarget_ ) )
    {
        throw smp::JsException();
    }

    return JS::ObjectValue( *jsArray );
}

void JsEvent::StopImmediatePropagation()
{
    isPropagationStopped_ = true;
}

void JsEvent::PreventDefault()
{
    if ( isCancelable_ )
    {
        isDefaultPrevented_ = true;
    }
}

bool JsEvent::get_False() const
{
    return false;
}

bool JsEvent::get_Cancelable() const
{
    return isCancelable_;
}

bool JsEvent::get_DefaultPrevented() const
{
    return isDefaultPrevented_;
}

uint8_t JsEvent::get_EventPhase() const
{
    return ( currentTarget_ ? 2 : 0 );
}

JSObject* JsEvent::get_Target() const
{
    return currentTarget_;
}

uint64_t JsEvent::get_TimeStamp() const
{
    return timeStamp_;
}

const qwr::u8string& JsEvent::get_Type() const
{
    return type_;
}

size_t JsEvent::GetInternalSize()
{
    return type_.size();
}

std::unique_ptr<mozjs::JsEvent>
JsEvent::CreateNative( JSContext* cx, const qwr::u8string& type, bool isCancelable )
{
    return std::unique_ptr<JsEvent>( new JsEvent( cx, type, isCancelable ) );
}

} // namespace mozjs
