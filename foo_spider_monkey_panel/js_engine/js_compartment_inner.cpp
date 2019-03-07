#include <stdafx.h>
#include "js_compartment_inner.h"

#include <js_engine/js_engine.h>

namespace mozjs
{

void JsCompartmentInner::OnGcStart()
{
    isMarkedForGc_ = true;
}

void JsCompartmentInner::OnGcDone()
{
    std::scoped_lock sl( gcDataLock_ );

    lastHeapSize_ = curHeapSize_;
    lastAllocCount_ = curAllocCount_;
    isMarkedForGc_ = false;
}

bool JsCompartmentInner::IsMarkedForGc() const
{
    return isMarkedForGc_;
}

uint64_t JsCompartmentInner::GetCurrentHeapBytes() const
{
    std::scoped_lock sl( gcDataLock_ );
    return curHeapSize_;
}

uint64_t JsCompartmentInner::GetLastHeapBytes() const
{
    std::scoped_lock sl( gcDataLock_ );
    return std::min( lastHeapSize_, curHeapSize_ );
}

uint32_t JsCompartmentInner::GetCurrentAllocCount() const
{
    std::scoped_lock sl( gcDataLock_ );
    return curAllocCount_;
}

uint32_t JsCompartmentInner::GetLastAllocCount() const
{
    std::scoped_lock sl( gcDataLock_ );
    return std::min( lastAllocCount_, curAllocCount_ );
}

void JsCompartmentInner::OnHeapAllocate( uint32_t size )
{
    std::scoped_lock sl( gcDataLock_ );
    curHeapSize_ += size;
    ++curAllocCount_;
}

void JsCompartmentInner::OnHeapDeallocate( uint32_t size )
{
    std::scoped_lock sl( gcDataLock_ );
    if ( size > curHeapSize_ )
    {
        assert( 0 );
        curHeapSize_ = 0;
    }
    else
    {
        curHeapSize_ -= size;
    }

    if ( !curAllocCount_ )
    {
        assert( 0 );
    }
    else
    {
        --curAllocCount_;
    }
}

void JsCompartmentInner::MarkForDeletion()
{
    isMarkedForDeletion_ = true;
}

bool JsCompartmentInner::IsMarkedForDeletion() const
{
    return isMarkedForDeletion_;
}

} // namespace mozjs
