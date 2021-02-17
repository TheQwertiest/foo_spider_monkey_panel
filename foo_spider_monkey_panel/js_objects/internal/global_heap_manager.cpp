#include <stdafx.h>

#include "global_heap_manager.h"

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/TracingAPI.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

namespace mozjs
{

GlobalHeapManager::GlobalHeapManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<GlobalHeapManager> GlobalHeapManager::Create( JSContext* cx )
{
    return std::unique_ptr<GlobalHeapManager>( new GlobalHeapManager( cx ) );
}

void GlobalHeapManager::RegisterUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( !heapUsers_.contains( heapUser ) );
    heapUsers_.emplace( heapUser, heapUser );
}

void GlobalHeapManager::UnregisterUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( heapUsers_.contains( heapUser ) );
    heapUsers_.erase( heapUser );
}

uint32_t GlobalHeapManager::Store( JS::HandleValue valueToStore )
{
    std::scoped_lock sl( heapElementsLock_ );

    if ( !unusedHeapElements_.empty() )
    {
        unusedHeapElements_.clear();
    }

    while ( heapElements_.contains( currentHeapId_ ) )
    {
        ++currentHeapId_;
    }

    heapElements_.emplace( currentHeapId_, std::make_unique<HeapElement>( valueToStore ) );
    return currentHeapId_++;
}

uint32_t GlobalHeapManager::Store( JS::HandleObject valueToStore )
{
    JS::RootedValue jsValue( pJsCtx_, JS::ObjectValue( *valueToStore ) );
    return Store( jsValue );
}

uint32_t GlobalHeapManager::Store( JS::HandleFunction valueToStore )
{
    JS::RootedObject jsObject( pJsCtx_, JS_GetFunctionObject( valueToStore ) );
    return Store( jsObject );
}

JS::Heap<JS::Value>& GlobalHeapManager::Get( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    if ( !unusedHeapElements_.empty() )
    {
        unusedHeapElements_.clear();
    }

    assert( heapElements_.contains( id ) );
    return *heapElements_[id];
}

void GlobalHeapManager::Remove( uint32_t id )
{ // Can be called in worker thread, outside of JS ctx, so we can't GC or destroy JS objects here
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapElements_.contains( id ) );
    unusedHeapElements_.emplace_back( std::move( heapElements_[id] ) );
    heapElements_.erase( id );
}

void GlobalHeapManager::Trace( JSTracer* trc )
{
    std::scoped_lock sl( heapElementsLock_ );

    for ( auto& [id, heapElement]: heapElements_ )
    {
        JS::TraceEdge( trc, heapElement.get(), "CustomHeap_Global" );
    }
    for ( auto& heapElement: unusedHeapElements_ )
    {
        JS::TraceEdge( trc, heapElement.get(), "CustomHeap_Global" );
    }
}

void GlobalHeapManager::PrepareForGc()
{ // clean up everything manually
    std::scoped_lock sl( heapUsersLock_ );

    for ( auto& [id, heapUser]: heapUsers_ )
    {
        heapUser->PrepareForGlobalGc();
    }
    unusedHeapElements_.clear();
    heapElements_.clear();
}

} // namespace mozjs
