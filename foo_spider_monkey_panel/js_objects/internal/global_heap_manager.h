#pragma once

#include <shared_mutex>
#include <optional>
#include <set>


class ActiveX;

namespace mozjs
{

class IHeapUser
{
public:
    IHeapUser() = default;
    virtual ~IHeapUser() = default;
    virtual void PrepareForGlobalGc() = 0;
};

/// @details Contains a tracer, which is removed only in destructor
class GlobalHeapManager
{
public:
    /// @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
    ~GlobalHeapManager() = default;
    GlobalHeapManager(const GlobalHeapManager& ) = delete;
    GlobalHeapManager& operator=( const GlobalHeapManager& ) = delete;

    static std::unique_ptr<GlobalHeapManager> Create( JSContext * cx );

public:
    void RegisterUser( IHeapUser* heapUser );
    void UnregisterUser( IHeapUser* heapUser );

    uint32_t Store( JS::HandleValue valueToStore );
    uint32_t Store( JS::HandleObject valueToStore );
    uint32_t Store( JS::HandleFunction valueToStore );
    JS::Heap<JS::Value>& Get( uint32_t id );
    void Remove( uint32_t id );

    void Trace( JSTracer* trc );
    void PrepareForGc();

private:
    GlobalHeapManager( JSContext * cx );

private: 
    JSContext * pJsCtx_ = nullptr;

    uint32_t currentHeapId_ = 0;

    using HeapElement = JS::Heap<JS::Value>;

    std::mutex heapElementsLock_;
    std::unordered_map<uint32_t, std::unique_ptr<HeapElement>> heapElements_;
    std::list<std::unique_ptr<HeapElement>> unusedHeapElements_;

    std::mutex heapUsersLock_;
    std::unordered_map<IHeapUser*, IHeapUser*> heapUsers_;
};

}
