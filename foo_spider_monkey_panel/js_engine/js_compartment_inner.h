#pragma once

#include <mutex>

namespace mozjs
{

class JsCompartmentInner final
{
public:
    JsCompartmentInner() = default;
    ~JsCompartmentInner() = default;
    JsCompartmentInner( const JsCompartmentInner& ) = delete;
    JsCompartmentInner& operator=( const JsCompartmentInner& ) = delete;

public:
    void MarkForDeletion();
    bool IsMarkedForDeletion() const;

    void OnGcStart();
    void OnGcDone();
    bool IsMarkedForGc() const;

    uint32_t GetCurrentHeapBytes() const;
    uint32_t GetLastHeapBytes() const;

    uint32_t GetCurrentAllocCount() const;
    uint32_t GetLastAllocCount() const;

    void OnHeapAllocate( uint32_t size );
    void OnHeapDeallocate( uint32_t size );

private:
    bool isMarkedForDeletion_ = false;
    bool isMarkedForGc_ = false;
    mutable std::mutex gcDataLock_;
    uint32_t curHeapSize_ = 0;
    uint32_t lastHeapSize_ = 0;
    uint32_t curAllocCount_ = 0;
    uint32_t lastAllocCount_ = 0;
};

} // namespace mozjs
