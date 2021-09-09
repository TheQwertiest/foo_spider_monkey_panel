#pragma once

#include <mutex>

namespace mozjs
{

class JsRealmInner final
{
public:
    JsRealmInner() = default;
    ~JsRealmInner() = default;
    JsRealmInner( const JsRealmInner& ) = delete;
    JsRealmInner& operator=( const JsRealmInner& ) = delete;

public:
    void MarkForDeletion();
    [[nodiscard]] bool IsMarkedForDeletion() const;

    void OnGcStart();
    void OnGcDone();
    [[nodiscard]] bool IsMarkedForGc() const;

    [[nodiscard]] uint64_t GetCurrentHeapBytes() const;
    [[nodiscard]] uint64_t GetLastHeapBytes() const;

    [[nodiscard]] uint32_t GetCurrentAllocCount() const;
    [[nodiscard]] uint32_t GetLastAllocCount() const;

    void OnHeapAllocate( uint32_t size );
    void OnHeapDeallocate( uint32_t size );

private:
    bool isMarkedForDeletion_ = false;
    bool isMarkedForGc_ = false;
    mutable std::mutex gcDataLock_;
    uint64_t curHeapSize_ = 0;
    uint64_t lastHeapSize_ = 0;
    uint32_t curAllocCount_ = 0;
    uint32_t lastAllocCount_ = 0;
};

} // namespace mozjs
