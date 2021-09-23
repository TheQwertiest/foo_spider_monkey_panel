#pragma once

#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>

#include <array>

namespace mozjs
{

class HeapHelper : public IHeapUser
{
public:
    HeapHelper( JSContext* cx )
        : pJsCtx_( cx )
    {
        assert( cx );

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
        assert( pNativeGlobal_ );

        pNativeGlobal_->GetHeapManager().RegisterUser( this );

        isJsAvailable_ = true;
    }

    ~HeapHelper() override
    {
        Finalize();
    };

    template <typename T>
    [[nodiscard]] uint32_t Store( const T& jsObject )
    {
        assert( core_api::is_main_thread() );
        assert( isJsAvailable_ );
        return valueHeapIds_.emplace_back( pNativeGlobal_->GetHeapManager().Store( jsObject ) );
    }

    [[nodiscard]] JS::Heap<JS::Value>& Get( uint32_t objectId )
    {
        assert( core_api::is_main_thread() );
        assert( isJsAvailable_ );
        return pNativeGlobal_->GetHeapManager().Get( objectId );
    }

    bool IsJsAvailable() const
    {
        assert( core_api::is_main_thread() );
        return isJsAvailable_;
    }

    void Finalize()
    { // might be called from worker thread
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

    void PrepareForGlobalGc() final
    {
        std::scoped_lock sl( cleanupLock_ );
        isJsAvailable_ = false;
    }

private:
    JSContext* pJsCtx_ = nullptr;

    std::vector<uint32_t> valueHeapIds_;
    JsGlobalObject* pNativeGlobal_ = nullptr;

    mutable std::mutex cleanupLock_;
    bool isJsAvailable_ = false;
};

} // namespace mozjs
