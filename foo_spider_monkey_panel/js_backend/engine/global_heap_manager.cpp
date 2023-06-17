#include <stdafx.h>

#include "global_heap_manager.h"

#include <js/TracingAPI.h>

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

    if ( !removedHeapElements_.empty() )
    {
        removedHeapElements_.clear();
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

    if ( !removedHeapElements_.empty() )
    {
        removedHeapElements_.clear();
    }

    assert( heapElements_.contains( id ) );
    return *heapElements_[id];
}

JSObject* GlobalHeapManager::GetObject( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    if ( !removedHeapElements_.empty() )
    {
        removedHeapElements_.clear();
    }

    assert( heapElements_.contains( id ) );
    assert( heapElements_[id]->get().isObject() );
    return &heapElements_[id]->get().toObject();
}

void GlobalHeapManager::Remove( uint32_t id )
{ // Can be called in worker thread, outside of JS ctx, so we can't GC or destroy JS objects here
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapElements_.contains( id ) );
    removedHeapElements_.emplace_back( std::move( heapElements_[id] ) );
    heapElements_.erase( id );
}

void GlobalHeapManager::StoreData( smp::not_null_unique<IHeapTraceableData> pData )
{
    std::scoped_lock sl( heapElementsLock_ );

    if ( !removedHeapData_.empty() )
    {
        removedHeapData_.clear();
    }

    heapData_.try_emplace( pData.get().get(), std::move( pData.get() ) );
}

void GlobalHeapManager::RemoveData( void* pData )
{ // Can be called in worker thread, outside of JS ctx, so we can't GC or destroy JS objects here
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapData_.contains( pData ) );
    removedHeapData_.emplace_back( std::move( heapData_[pData] ) );
    heapData_.erase( pData );
}

void GlobalHeapManager::Trace( JSTracer* trc )
{
    std::scoped_lock sl( heapElementsLock_ );

    for ( auto& pElem: heapElements_ | ranges::views::values )
    {
        JS::TraceEdge( trc, pElem.get(), "CustomHeap: Global(in-use elements)" );
    }
    for ( auto& pElem: removedHeapElements_ )
    {
        JS::TraceEdge( trc, pElem.get(), "CustomHeap: Global(unused elements)" );
    }
    for ( auto& pElem: heapData_ | ranges::views::values )
    {
        pElem->Trace( trc );
    }
    for ( auto& pElem: removedHeapData_ )
    {
        pElem->Trace( trc );
    }
}

void GlobalHeapManager::PrepareForGc()
{ // clean up everything manually
    std::scoped_lock sl( heapUsersLock_ );

    for ( auto& [id, heapUser]: heapUsers_ )
    {
        heapUser->PrepareForGlobalGc();
    }
    removedHeapElements_.clear();
    heapElements_.clear();
    removedHeapData_.clear();
    heapData_.clear();
}

} // namespace mozjs
