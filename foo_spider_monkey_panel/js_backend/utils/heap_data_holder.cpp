#include <stdafx.h>

#include "heap_data_holder.h"

#include <js_backend/objects/core/global_object.h>

#include <array>

namespace mozjs
{

HeapDataHolderImpl::HeapDataHolderImpl( JSContext* cx, smp::not_null_unique<IHeapTraceableData> pData )
    : pJsCtx_( cx )
    , pData_( pData.get().get() )
{
    assert( cx );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    pNativeGlobal_ = JsGlobalObject::ExtractNative( cx, jsGlobal );
    assert( pNativeGlobal_ );

    auto& heapManager = pNativeGlobal_->GetHeapManager();
    heapManager.RegisterUser( this );
    heapManager.StoreData( std::move( pData ) );
}

HeapDataHolderImpl::HeapDataHolderImpl( HeapDataHolderImpl&& other )
    : pJsCtx_( other.pJsCtx_ )
    , pData_( other.pData_ )
    , pNativeGlobal_( other.pNativeGlobal_ )
{
    std::scoped_lock sl( other.cleanupLock_ );
    isJsAvailable_ = other.isJsAvailable_;
    if ( isJsAvailable_ )
    {
        auto& heapManager = pNativeGlobal_->GetHeapManager();
        heapManager.RegisterUser( this );
    }
    assert( other.hasData_ );
    other.hasData_ = false;
}

HeapDataHolderImpl::~HeapDataHolderImpl()
{
    Finalize();
};

IHeapTraceableData& HeapDataHolderImpl::Get()
{
    assert( core_api::is_main_thread() );
    assert( isJsAvailable_ && hasData_ );
    return *pData_;
}

bool HeapDataHolderImpl::IsJsAvailable() const
{
    assert( core_api::is_main_thread() );
    return isJsAvailable_;
}

void HeapDataHolderImpl::Finalize()
{
    std::scoped_lock sl( cleanupLock_ );
    if ( !isJsAvailable_ )
    {
        return;
    }

    auto& heapManager = pNativeGlobal_->GetHeapManager();
    if ( hasData_ )
    {
        heapManager.RemoveData( pData_ );
    }
    heapManager.UnregisterUser( this );

    isJsAvailable_ = false;
}

void HeapDataHolderImpl::PrepareForGlobalGc()
{
    std::scoped_lock sl( cleanupLock_ );
    isJsAvailable_ = false;
}

} // namespace mozjs
