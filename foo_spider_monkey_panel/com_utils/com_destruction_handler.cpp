#include <stdafx.h>

#include "com_destruction_handler.h"

namespace
{

// using `map` instead of `set`, because making lookup transparent for smart-ptrs requires much more code
std::unordered_map<smp::com::StorageObject*, std::unique_ptr<smp::com::StorageObject>> g_objectStorage;
std::vector<std::unique_ptr<smp::com::StorageObject>> g_objectsToDelete;

} // namespace

namespace
{

void CleanObject( smp::com::StorageObject& object )
{
    if ( object.pDispatch )
    {
        object.pDispatch->Release();
    }
    if ( object.pUnknown )
    {
        object.pUnknown->Release();
    }
    if ( object.pTypeInfo )
    {
        object.pTypeInfo->Release();
    }
    try
    {
        object.variant.Clear();
    }
    catch ( const _com_error& )
    {
    }

    CoFreeUnusedLibraries();

    object = smp::com::StorageObject{};
}

} // namespace

namespace smp::com
{

StorageObject* GetNewStoredObject()
{
    assert( core_api::is_main_thread() );

    auto pObject = std::make_unique<StorageObject>();
    auto* pObjectToReturn = pObject.get();
    g_objectStorage.try_emplace( pObjectToReturn, std::move( pObject ) );

    return pObjectToReturn;
}

void MarkStoredObjectAsToBeDeleted( StorageObject* pObject )
{
    assert( core_api::is_main_thread() );
    assert( pObject );
    assert( g_objectStorage.contains( pObject ) );

    g_objectsToDelete.emplace_back( std::move( g_objectStorage.at( pObject ) ) );
    g_objectStorage.erase( pObject );
}

void DeleteMarkedObjects()
{
    assert( core_api::is_main_thread() );

    // cause re-entrancy...
    auto localCopy = std::move( g_objectsToDelete );
    g_objectsToDelete.clear();

    for ( auto& pObject: localCopy )
    {
        CleanObject( *pObject );
    }
}

void DeleteAllStoredObject()
{
    assert( core_api::is_main_thread() );

    for ( auto& [dummy, pObject]: g_objectStorage )
    {
        CleanObject( *pObject );
    }
    g_objectStorage.clear();

    DeleteMarkedObjects();
}

} // namespace smp::com
