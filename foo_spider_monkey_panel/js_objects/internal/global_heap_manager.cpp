#include <stdafx.h>
#include "global_heap_manager.h"

#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
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

std::unique_ptr<GlobalHeapManager> GlobalHeapManager::Create( JSContext * cx )
{
    std::unique_ptr<GlobalHeapManager> tmp( new GlobalHeapManager( cx ) );
    if ( !JS_AddExtraGCRootsTracer( cx, GlobalHeapManager::TraceHeapValue, tmp.get() ) )
    {
        return nullptr;
    }

    return tmp;
}

void GlobalHeapManager::RemoveTracer()
{
    JS_RemoveExtraGCRootsTracer( pJsCtx_, GlobalHeapManager::TraceHeapValue, this );
    
    std::scoped_lock sl( heapUsersLock_ );

    for ( auto& [id, heapUser] : heapUsers_ )
    {
        heapUser->DisableHeapCleanup();
    }

    heapUsers_.clear();
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

    while( heapElements_.count( currentHeapId_ ))
    {
        ++currentHeapId_;
    }

    heapElements_.emplace(currentHeapId_, std::make_unique<HeapElement>( valueToStore ));
    return currentHeapId_++;
}

JS::Heap<JS::Value>& GlobalHeapManager::Get( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapElements_.count( id ) );
    return heapElements_[id]->value;
}

void GlobalHeapManager::Remove( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );
    
    assert( heapElements_.count(id) );    
    heapElements_[id]->inUse = false;    
}

void GlobalHeapManager::TraceHeapValue( JSTracer *trc, void *data )
{  
    assert( data );
    auto globalObject = static_cast<GlobalHeapManager*>( data );

    std::scoped_lock sl( globalObject->heapElementsLock_ );
    
    auto& heapMap = globalObject->heapElements_;
    for ( auto it = heapMap.cbegin(); it != heapMap.cend();)
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
