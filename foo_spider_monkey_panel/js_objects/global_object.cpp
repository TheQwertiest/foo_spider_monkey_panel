#include <stdafx.h>
#include "global_object.h"

#include <js_engine/js_container.h>
#include <js_objects/active_x.h>
#include <js_objects/console.h>
#include <js_objects/fb_playlist_manager.h>
#include <js_objects/gdi_utils.h>
#include <js_objects/utils.h>
#include <js_objects/fb_utils.h>
#include <js_objects/window.h>
#include <js_utils/js_object_helper.h>

#include <js_panel_window.h>

#include <js/TracingAPI.h>

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
    JsFinalizeOp<JsGlobalObject>,
    nullptr,
    nullptr,
    nullptr,
    nullptr // set in runtime to JS_GlobalObjectTraceHook
};

// TODO: remove HWND and HAS_PRIVATE after creating Window class

JSClass jsClass = {
     "Global",
     JSCLASS_GLOBAL_FLAGS | DefaultClassFlags(),
     &jsOps
};

}

namespace mozjs
{


JsGlobalObject::JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentContainer_( parentContainer )
    , parentPanel_( parentPanel )
{    
}


JsGlobalObject::~JsGlobalObject()
{
    RemoveHeapTracer();
}

JSObject* JsGlobalObject::Create( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
{
    if ( !jsOps.trace )
    {// JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.      
        jsOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::CompartmentOptions options;
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &jsClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    {
        JSAutoCompartment ac( cx, jsObj );

        if ( !JS_InitStandardClasses( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !DefineConsole( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !CreateAndInstallObject( cx, jsObj, "gdi", JsGdiUtils::Create ) 
             || !CreateAndInstallObject( cx, jsObj, "plman", JsFbPlaylistManager::Create )
             || !CreateAndInstallObject( cx, jsObj, "utils", JsUtils::Create ) 
             || !CreateAndInstallObject( cx, jsObj, "fb", JsFbUtils::Create ) 
             || !CreateAndInstallObject( cx, jsObj, "window", JsWindow::Create, parentPanel ) )
        {
            return nullptr;
        }

        // TODO: proto must be saved and used in object creation
        JS::RootedObject activeXproto( cx, CreateActiveXProto( cx, jsObj ) );
        if ( !activeXproto )
        {
            return nullptr;
        }

        auto pNative = new JsGlobalObject( cx, parentContainer, parentPanel );

        JS_SetPrivate( jsObj, pNative );

        if ( !JS_AddExtraGCRootsTracer( cx, JsGlobalObject::TraceHeapValue, pNative ) )
        {
            return nullptr;
        }

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

const JSClass& JsGlobalObject::GetClass()
{
    return jsClass;
}

void JsGlobalObject::Fail( std::string_view errorText )
{
    parentContainer_.Fail();
    parentPanel_.JsEngineFail( errorText );
}

void JsGlobalObject::RegisterHeapUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( !heapUsers_.count( heapUser ) );
    heapUsers_.emplace( heapUser, heapUser );
}

void JsGlobalObject::UnregisterHeapUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( heapUsers_.count( heapUser ) );
    heapUsers_.erase( heapUser );
}

uint32_t JsGlobalObject::StoreToHeap( JS::HandleValue valueToStore )
{
    std::scoped_lock sl( heapElementsLock_ );

    while( heapElements_.count( currentHeapId_ ))
    {
        ++currentHeapId_;
    }

    heapElements_[currentHeapId_] = std::make_shared<HeapElement>( valueToStore );
    return currentHeapId_++;
}

JS::Heap<JS::Value>& JsGlobalObject::GetFromHeap( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapElements_.count( id ) );
    return heapElements_[id]->value;
}

void JsGlobalObject::RemoveFromHeap( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );
    
    assert( heapElements_.count(id) );    
    heapElements_[id]->inUse = false;    
}

void JsGlobalObject::RemoveHeapTracer()
{
    JS_RemoveExtraGCRootsTracer( pJsCtx_, JsGlobalObject::TraceHeapValue, this );
    
    std::scoped_lock sl( heapUsersLock_ );

    for ( auto heapUser : heapUsers_ )
    {
        heapUser.second->DisableHeapCleanup();
    }

    heapUsers_.clear();
}

void JsGlobalObject::TraceHeapValue( JSTracer *trc, void *data )
{  
    assert( data );
    auto globalObject = static_cast<JsGlobalObject*>( data );

    std::scoped_lock sl( globalObject->heapElementsLock_ );
    
    auto& heapMap = globalObject->heapElements_;
    for ( auto it = heapMap.begin(); it != heapMap.end();)
    {
        if ( !it->second->inUse )
        {
            it = heapMap.erase( it );
        }
        else
        {
            JS::TraceEdge( trc, &(it->second->value), "CustomHeap_Global" );
            it++;
        }
    }
}

}
