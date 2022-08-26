#include <stdafx.h>

#include "event_target.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <utils/logging.h>

#include <qwr/final_action.h>

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
    JsEventTarget::Trace
};

JSClass jsClass = {
    "EventTarget",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( addEventListener, JsEventTarget::AddEventListener );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeEventListener, JsEventTarget::RemoveEventListener );

bool dispatchEvent( JSContext* cx, unsigned argc, JS::Value* vp )
{
    // handling manually, because we need `this`
    const auto wrappedFunc = []( JSContext* cx, unsigned argc, JS::Value* vp ) {
        JS::CallArgs jsArgs = JS::CallArgsFromVp( argc, vp );
        qwr::QwrException::ExpectTrue( jsArgs.length() == 1, "Invalid number of arguments" );

        auto pNative = JsEventTarget::ExtractNative( cx, jsArgs.thisv() );
        qwr::QwrException::ExpectTrue( pNative, "`this` is not an object of valid type" );

        JS::RootedObject jsSelf( cx, &jsArgs.thisv().toObject() );
        pNative->DispatchEvent( jsSelf, jsArgs[0] );
    };

    return mozjs::error::Execute_JsSafe( cx, "dispatchEvent", wrappedFunc, argc, vp );
}

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

int64_t JsEventTarget::dispatchNestedCounter_ = 0;

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

void JsEventTarget::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<JsEventTarget>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    for ( const auto& [type, listeners]: pNative->typeToListeners_ )
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

bool JsEventTarget::HasEventListener( const qwr::u8string& type )
{
    return typeToListeners_.count( type );
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
            listenerElement->isRemoved = false;
            return;
        }
    }
    listeners.emplace_back( std::make_shared<HeapElement>( listenerObject ) );

    if ( !dispatchNestedCounter_ )
    {
        ranges::actions::remove_if( typeToListeners_.at( type ), [&listenerObject]( const auto& listenerElement ) { return listenerElement->isRemoved; } );
    }
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

    for ( const auto& listenerElement: typeToListeners_.at( type ) )
    {
        if ( listenerElement->jsObject.get() == listenerObject.get() )
        {
            listenerElement->isRemoved = true;
            break;
        }
    }
}

void JsEventTarget::DispatchEvent( JS::HandleObject self, JS::HandleValue event )
{
    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, event );
    qwr::QwrException::ExpectTrue( !pNativeEvent->HasTarget(),
                                   "An attempt was made to use an object that is not, or is no longer, usable" );

    const auto& type = pNativeEvent->get_Type();

    if ( !typeToListeners_.contains( type ) )
    {
        return;
    }

    JS::RootedObject jsGlobal( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
    assert( jsGlobal );

    pNativeEvent->SetCurrentTarget( self );
    ++dispatchNestedCounter_;
    const qwr::final_action autoTarget( [&] {
        pNativeEvent->SetCurrentTarget( nullptr );
        pNativeEvent->ResetPropagationStatus();
        --dispatchNestedCounter_;
    } );

    // make a copy, since listeners might be added or removed during event dispatch
    const auto listenersElementCopy = typeToListeners_.at( type );

    // root to avoid untraceable js values
    JS::RootedObjectVector jsListenersCopy( pJsCtx_ );
    for ( const auto& listener: listenersElementCopy )
    {
        if ( !jsListenersCopy.append( listener->jsObject.get() ) )
        {
            throw JsException();
        }
    }

    for ( const auto& [jsListener, element]: ranges::views::zip( jsListenersCopy, listenersElementCopy ) )
    {
        if ( element->isRemoved )
        {
            continue;
        }

        JS::RootedObject jsObject( pJsCtx_, jsListener );
        InvokeListener( jsGlobal, jsObject, event );

        if ( pNativeEvent->IsPropagationStopped() )
        {
            break;
        }
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
