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

uint32_t JsCompartmentInner::GetCurrentHeapBytes()
{
    std::scoped_lock sl( gcDataLock_ );

    if ( lastHeapSize_ > curHeapSize_ )
    {
        lastHeapSize_ = curHeapSize_;
    }

    return curHeapSize_;
}

uint32_t JsCompartmentInner::GetLastHeapBytes()
{
    std::scoped_lock sl( gcDataLock_ );

    if ( lastHeapSize_ > curHeapSize_ )
    {
        lastHeapSize_ = curHeapSize_;
    }

    return lastHeapSize_;
}

uint32_t JsCompartmentInner::GetCurrentAllocCount()
{
    std::scoped_lock sl( gcDataLock_ );

    if ( lastAllocCount_ > curAllocCount_ )
    {
        lastAllocCount_ = curAllocCount_;
    }

    return curAllocCount_;
}

uint32_t JsCompartmentInner::GetLastAllocCount()
{
    std::scoped_lock sl( gcDataLock_ );

    if ( lastAllocCount_ > curAllocCount_ )
    {
        lastAllocCount_ = curAllocCount_;
    }

    return lastAllocCount_;
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

}
