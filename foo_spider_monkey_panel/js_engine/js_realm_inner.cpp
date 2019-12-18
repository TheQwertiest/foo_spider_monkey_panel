#include <stdafx.h>

#include "js_realm_inner.h"

#include <js_engine/js_engine.h>

namespace mozjs
{

void JsRealmInner::OnGcStart()
{
    isMarkedForGc_ = true;
}

void JsRealmInner::OnGcDone()
{
    std::scoped_lock sl( gcDataLock_ );

    lastHeapSize_ = curHeapSize_;
    lastAllocCount_ = curAllocCount_;
    isMarkedForGc_ = false;
}

bool JsRealmInner::IsMarkedForGc() const
{
    return isMarkedForGc_;
}

uint64_t JsRealmInner::GetCurrentHeapBytes() const
{
    std::scoped_lock sl( gcDataLock_ );
    return curHeapSize_;
}

uint64_t JsRealmInner::GetLastHeapBytes() const
{
    std::scoped_lock sl( gcDataLock_ );
    return std::min( lastHeapSize_, curHeapSize_ );
}

uint32_t JsRealmInner::GetCurrentAllocCount() const
{
    std::scoped_lock sl( gcDataLock_ );
    return curAllocCount_;
}

uint32_t JsRealmInner::GetLastAllocCount() const
{
    std::scoped_lock sl( gcDataLock_ );
    return std::min( lastAllocCount_, curAllocCount_ );
}

void JsRealmInner::OnHeapAllocate( uint32_t size )
{
    std::scoped_lock sl( gcDataLock_ );
    curHeapSize_ += size;
    ++curAllocCount_;
}

void JsRealmInner::OnHeapDeallocate( uint32_t size )
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

void JsRealmInner::MarkForDeletion()
{
    isMarkedForDeletion_ = true;
}

bool JsRealmInner::IsMarkedForDeletion() const
{
    return isMarkedForDeletion_;
}

} // namespace mozjs
