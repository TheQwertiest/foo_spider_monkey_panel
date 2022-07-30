#include <stdafx.h>

#include "js_event_target.h"

#include <js_engine/js_to_native_invoker.h>
#include <utils/logging.h>

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
    JsEventTarget::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "EventTarget",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( addEventListener, JsEventTarget::AddEventListener );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeEventListener, JsEventTarget::RemoveEventListener );
MJS_DEFINE_JS_FN_FROM_NATIVE( dispatchEvent, JsEventTarget::DispatchEvent );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>( {
    JS_FN( "addEventListener", addEventListener, 2, kDefaultPropsFlags ),
    JS_FN( "removeEventListener", removeEventListener, 2, kDefaultPropsFlags ),
    JS_FN( "dispatchEvent", dispatchEvent, 1, kDefaultPropsFlags ),
    JS_FS_END,
} );

constexpr auto jsProperties = std::to_array<JSPropertySpec>( {
    JS_PS_END,
} );

MJS_DEFINE_JS_FN_FROM_NATIVE( JsEventTarget_Constructor, JsEventTarget::Constructor );

} // namespace

namespace mozjs
{

const JSClass JsEventTarget::JsClass = jsClass;
const JSFunctionSpec* JsEventTarget::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsEventTarget::JsProperties = jsProperties.data();
const JsPrototypeId JsEventTarget::PrototypeId = JsPrototypeId::EventTarget;
const JSNative JsEventTarget::JsConstructor = ::JsEventTarget_Constructor;

JsEventTarget::JsEventTarget( JSContext* cx )
    : pJsCtx_( cx )
{
}

JSObject* JsEventTarget::Constructor( JSContext* cx )
{
    return JsEventTarget::CreateJs( cx );
}

std::unique_ptr<mozjs::JsEventTarget>
JsEventTarget::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsEventTarget>( new JsEventTarget( cx ) );
}

size_t JsEventTarget::GetInternalSize()
{
    return 0;
}

void JsEventTarget::Trace( JSTracer* trc )
{
    for ( const auto& [type, listeners]: typeToListeners_ )
    {
        for ( const auto& pListener: listeners )
        {
            JS::TraceEdge( trc, &pListener->jsObject, "CustomHeap: EventTarget listeners" );
        }
    }
}

void JsEventTarget::PrepareForGc()
{
    typeToListeners_.clear();
}

void JsEventTarget::AddEventListener( const qwr::u8string& type, JS::HandleValue listener )
{
    qwr::QwrException::ExpectTrue( listener.isObject(),
                                   "`listener` argument is not a JS object" );

    JS::RootedObject listenerObject( pJsCtx_, &listener.toObject() );

    auto& listeners = typeToListeners_[type];
    for ( const auto& listenerElement: listeners )
    {
        if ( listenerElement->jsObject.get() == listenerObject.get() )
        {
            return;
        }
    }
    listeners.emplace_back( std::make_unique<HeapElement>( listenerObject ) );
}

void JsEventTarget::RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener )
{
    qwr::QwrException::ExpectTrue( listener.isObject(),
                                   "`listener` argument is not a JS object" );

    JS::RootedObject listenerObject( pJsCtx_, &listener.toObject() );

    if ( !typeToListeners_.contains( type ) )
    {
        return;
    }

    ranges::actions::remove_if( typeToListeners_.at( type ), [&listenerObject]( const auto& listenerElement ) { return listenerElement->jsObject.get() == listenerObject.get(); } );
}

void JsEventTarget::DispatchEvent( JS::HandleValue event )
{
    const auto type = convert::to_native::ToValue<std::string>( pJsCtx_, event );
    if ( !typeToListeners_.contains( type ) )
    {
        return;
    }

    JS::RootedObject jsGlobal( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
    assert( jsGlobal );

    for ( const auto& listenerElement: typeToListeners_.at( type ) )
    {
        JS::RootedObject jsObject( pJsCtx_, listenerElement->jsObject );
        InvokeListener( jsGlobal, jsObject, event );
    }
}

void JsEventTarget::InvokeListener( JS::HandleObject currentGlobal, JS::HandleObject listener, JS::HandleValue event )
{
    const auto isListenerCallable = JS_ObjectIsFunction( listener );
    JS::RootedValue jsCallback( pJsCtx_ );
    if ( isListenerCallable )
    {
        jsCallback = JS::ObjectValue( *listener );
    }
    else
    {
        bool hasProperty = false;
        JS::RootedValue jsEventHandler( pJsCtx_ );
        // TODO: atomize property name: https://searchfox.org/mozilla-central/source/__GENERATED__/dom/bindings/EventListenerBinding.cpp#52
        if ( !JS_GetProperty( pJsCtx_, listener, "handleEvent", &jsEventHandler ) )
        {
            throw JsException();
        }

        if ( !jsEventHandler.isObject() || !JS_ObjectIsFunction( &jsEventHandler.toObject() ) )
        {
            throw qwr::QwrException( "Event listener is not callable" );
        }

        jsCallback = jsEventHandler;
    }

    JS::RootedValue thisValue( pJsCtx_, JS::ObjectValue( isListenerCallable ? *currentGlobal : *listener ) );
    JS::RootedValueArray<1> args( pJsCtx_ );
    args[0].set( event );
    JS::RootedValue rval( pJsCtx_ );
    if ( !JS::Call( pJsCtx_, thisValue, jsCallback, args, &rval ) )
    {
        throw JsException();
    }
}

} // namespace mozjs
