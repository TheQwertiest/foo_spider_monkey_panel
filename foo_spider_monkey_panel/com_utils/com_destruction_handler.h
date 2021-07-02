#pragma once

namespace smp::com
{

struct ComStorageObject
{
    IDispatch* pDispatch = nullptr;
    IUnknown* pUnknown = nullptr;
    ITypeInfo* pTypeInfo = nullptr;
    _variant_t variant;
};

/// @remark Should be called only from the main thread
ComStorageObject* GetNewStoredObject();

/// @remark Should be called only from the main thread
void MarkStoredObjectAsToBeDeleted( ComStorageObject* pObject );

/// @remark Should be called only from the main thread
void DeleteMarkedObjects();

/// @remark Should be called only from the main thread
void DeleteAllStoredObject();

} // namespace smp::com
