#pragma once

// TLDR: This is needed to avoid message loop re-entrancy hell and consequent dead-locks.
// Full story:
// COM object might lock and return control to message loop when being destroyed.
// JS GC acquires lock during it's operations.
// If COM object locks during JS object's `finalize`, then it will cause a dead-lock
// once GC is triggered again in re-entered message loop.

namespace smp::com
{

struct StorageObject
{
    IDispatch* pDispatch = nullptr;
    IUnknown* pUnknown = nullptr;
    ITypeInfo* pTypeInfo = nullptr;
    _variant_t variant;
};

/// @remark Should be called only from the main thread
[[nodiscard]] StorageObject* GetNewStoredObject();

/// @remark Should be called only from the main thread
void MarkStoredObjectAsToBeDeleted( StorageObject* pObject );

/// @remark Should be called only from the main thread
void DeleteMarkedObjects();

/// @remark Should be called only from the main thread
void DeleteAllStoredObject();

} // namespace smp::com
