#include <stdafx.h>
#include "js_compartment_inner.h"

#include <js_engine/js_engine.h>


namespace mozjs
{

void JsCompartmentInner::OnGcStart()
{
    hasGcStarted_ = true;
}

void JsCompartmentInner::OnGcDone()
{
    std::scoped_lock sl( heapSizeLock_ );

    lastHeapSize_ = curHeapSize_;
    hasGcStarted_ = false;
}

bool JsCompartmentInner::HasGcStarted() const
{
    return hasGcStarted_;
}

uint32_t JsCompartmentInner::GetCurrentHeapBytes()
{
    std::scoped_lock sl( heapSizeLock_ );

    if ( lastHeapSize_ > curHeapSize_ )
    {
        lastHeapSize_ = curHeapSize_;
    }

    return curHeapSize_;
}

uint32_t JsCompartmentInner::GetLastHeapBytes()
{
    std::scoped_lock sl( heapSizeLock_ );
    return lastHeapSize_;
}

void JsCompartmentInner::OnHeapAllocate( uint32_t size )
{
    std::scoped_lock sl( heapSizeLock_ );
    curHeapSize_ += size;
}

void JsCompartmentInner::OnHeapDeallocate( uint32_t size )
{
    std::scoped_lock sl( heapSizeLock_ );
    if ( size > curHeapSize_ )
    {
        assert( 0 );
        curHeapSize_ = 0;
    }
    else
    {
        curHeapSize_ -= size;
    }
    
}

}
