#include <stdafx.h>

#include "js_heap_helper.h"

#include <js_backend/objects/core/global_object.h>

#include <array>

namespace mozjs
{

HeapHelper::HeapHelper( JSContext* cx )
    : pJsCtx_( cx )
{
    assert( cx );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    pNativeGlobal_ = JsGlobalObject::ExtractNative( cx, jsGlobal );
    assert( pNativeGlobal_ );

    pNativeGlobal_->GetHeapManager().RegisterUser( this );

    isJsAvailable_ = true;
}

HeapHelper::~HeapHelper()
{
    Finalize();
};

JS::Heap<JS::Value>& HeapHelper::Get( uint32_t objectId )
{
    assert( core_api::is_main_thread() );
    assert( isJsAvailable_ );
    return pNativeGlobal_->GetHeapManager().Get( objectId );
}

JSObject* HeapHelper::GetObject( uint32_t objectId )
{
    assert( core_api::is_main_thread() );
    assert( isJsAvailable_ );
    return pNativeGlobal_->GetHeapManager().GetObject( objectId );
}

bool HeapHelper::IsJsAvailable() const
{
    assert( core_api::is_main_thread() );
    return isJsAvailable_;
}

void HeapHelper::Finalize()
{
    std::scoped_lock sl( cleanupLock_ );
    if ( !isJsAvailable_ )
    {
        return;
    }

    for ( auto heapId: valueHeapIds_ )
    {
        pNativeGlobal_->GetHeapManager().Remove( heapId );
    }
    pNativeGlobal_->GetHeapManager().UnregisterUser( this );

    isJsAvailable_ = false;
}

void HeapHelper::PrepareForGlobalGc()
{
    std::scoped_lock sl( cleanupLock_ );
    isJsAvailable_ = false;
}

} // namespace mozjs
