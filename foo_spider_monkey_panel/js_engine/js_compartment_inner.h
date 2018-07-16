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
    void OnGcStart();
    void OnGcDone();
    bool HasGcStarted() const;
    uint32_t GetCurrentHeapBytes();    
    uint32_t GetLastHeapBytes();
    void OnHeapAllocate( uint32_t size );
    void OnHeapDeallocate( uint32_t size );

private:
    bool hasGcStarted_ = false;
    std::mutex heapSizeLock_;
    uint32_t curHeapSize_ = 0;
    uint32_t lastHeapSize_ = 0;
};

}
