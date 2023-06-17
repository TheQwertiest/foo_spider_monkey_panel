#pragma once

#include <optional>
#include <set>
#include <shared_mutex>

#define MJS_HEAP_TRACER_BEGIN            \
    void Trace( JSTracer* trc ) override \
    {
#define MJS_HEAP_TRACE_VALUE( value, description ) \
    JS::TraceEdge( trc, &value, "CustomHeap: " description );
#define MJS_HEAP_TRACER_END }

namespace mozjs
{

class IHeapUser
{
public:
    IHeapUser() = default;
    virtual ~IHeapUser() = default;
    virtual void PrepareForGlobalGc() = 0;
};

struct IHeapTraceableData
{
    virtual ~IHeapTraceableData() = default;
    virtual void Trace( JSTracer* trc ) = 0;
};

/// @details Contains a tracer, which is removed only in destructor
class GlobalHeapManager
{
public:
    /// @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
    ~GlobalHeapManager() = default;
    GlobalHeapManager( const GlobalHeapManager& ) = delete;
    GlobalHeapManager& operator=( const GlobalHeapManager& ) = delete;

    static [[nodiscard]] std::unique_ptr<GlobalHeapManager> Create( JSContext* cx );

public:
    void RegisterUser( IHeapUser* heapUser );
    void UnregisterUser( IHeapUser* heapUser );

    [[nodiscard]] uint32_t Store( JS::HandleValue valueToStore );
    [[nodiscard]] uint32_t Store( JS::HandleObject valueToStore );
    [[nodiscard]] uint32_t Store( JS::HandleFunction valueToStore );
    [[nodiscard]] JS::Heap<JS::Value>& Get( uint32_t id );
    [[nodiscard]] JSObject* GetObject( uint32_t id );
    void Remove( uint32_t id );

    void StoreData( smp::not_null_unique<IHeapTraceableData> pData );
    void RemoveData( void* pData );

    void Trace( JSTracer* trc );
    void PrepareForGc();

private:
    GlobalHeapManager( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;

    uint32_t currentHeapId_ = 0;

    using HeapElement = JS::Heap<JS::Value>;

    std::mutex heapElementsLock_;
    std::unordered_map<uint32_t, std::unique_ptr<HeapElement>> heapElements_;
    std::vector<std::unique_ptr<HeapElement>> removedHeapElements_;

    std::unordered_map<void*, std::unique_ptr<IHeapTraceableData>> heapData_;
    std::vector<std::unique_ptr<IHeapTraceableData>> removedHeapData_;

    std::mutex heapUsersLock_;
    std::unordered_map<IHeapUser*, IHeapUser*> heapUsers_;
};

} // namespace mozjs
