#include <stdafx.h>
#include "global_heap_manager.h"

#pragma warning( push )
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#   include <js/TracingAPI.h>
#pragma warning( pop )

namespace mozjs
{

GlobalHeapManager::GlobalHeapManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

GlobalHeapManager::~GlobalHeapManager()
{
    RemoveTracer();
}

std::unique_ptr<GlobalHeapManager> GlobalHeapManager::Create( JSContext* cx )
{
    std::unique_ptr<GlobalHeapManager> tmp( new GlobalHeapManager( cx ) );
    if ( !JS_AddExtraGCRootsTracer( cx, GlobalHeapManager::TraceHeapValue, tmp.get() ) )
    {
        throw smp::JsException();
    }

    return tmp;
}

void GlobalHeapManager::RegisterUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( !heapUsers_.count( heapUser ) );
    heapUsers_.emplace( heapUser, heapUser );
}

void GlobalHeapManager::UnregisterUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( heapUsers_.count( heapUser ) );
    heapUsers_.erase( heapUser );
}

uint32_t GlobalHeapManager::Store( JS::HandleValue valueToStore )
{
    std::scoped_lock sl( heapElementsLock_ );

    if ( !unusedHeapElements_.empty() )
    {
        unusedHeapElements_.clear();
    }

    while ( heapElements_.count( currentHeapId_ ) )
    {
        ++currentHeapId_;
    }

    heapElements_.emplace( currentHeapId_, std::make_unique<HeapElement>( valueToStore ) );
    return currentHeapId_++;
}

JS::Heap<JS::Value>& GlobalHeapManager::Get( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    if ( !unusedHeapElements_.empty() )
    {
        unusedHeapElements_.clear();
    }

    assert( heapElements_.count( id ) );
    return *heapElements_[id];
}

void GlobalHeapManager::Remove( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapElements_.count( id ) );
    unusedHeapElements_.emplace_back( std::move( heapElements_[id] ) );
    heapElements_.erase( id );
}

void GlobalHeapManager::RemoveTracer()
{    
    { // clean up everything manually
        std::scoped_lock sl( heapUsersLock_ );

        for ( auto& [id, heapUser] : heapUsers_ )
        {
            heapUser->PrepareForGlobalGc();
        }
        unusedHeapElements_.clear();
        heapElements_.clear();
    }

    JS_RemoveExtraGCRootsTracer( pJsCtx_, GlobalHeapManager::TraceHeapValue, this );
}

void GlobalHeapManager::TraceHeapValue( JSTracer* trc, void* data )
{
    assert( data );
    auto globalObject = static_cast<GlobalHeapManager*>( data );

    std::scoped_lock sl( globalObject->heapElementsLock_ );

    for ( auto& [id, heapElement] : globalObject->heapElements_ )
    {
        JS::TraceEdge( trc, heapElement.get(), "CustomHeap_Global" );
    }
    for ( auto& heapElement : globalObject->unusedHeapElements_ )
    {
        JS::TraceEdge( trc, heapElement.get(), "CustomHeap_Global" );
    }
}

} // namespace mozjs
