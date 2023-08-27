#pragma once

#include <js_backend/engine/global_heap_manager.h>
#include <js_backend/objects/core/global_object.h>

namespace mozjs
{

// TODO: replace heap helper with JSHolder
class HeapHelper : public IHeapUser
{
public:
    HeapHelper( JSContext* cx );
    ~HeapHelper() override;

    template <typename T>
    [[nodiscard]] uint32_t Store( const T& jsObject )
    {
        assert( core_api::is_main_thread() );
        assert( isJsAvailable_ );
        return valueHeapIds_.emplace_back( pNativeGlobal_->GetHeapManager().Store( jsObject ) );
    }

    [[nodiscard]] JS::Heap<JS::Value>& Get( uint32_t objectId );
    // TODO: remove GetObject, just use return JS::Value& above and use it directly instead
    [[nodiscard]] JSObject* GetObject( uint32_t objectId );

    bool IsJsAvailable() const;

    // might be called from worker thread
    void Finalize();

    void PrepareForGlobalGc() final;

private:
    JSContext* pJsCtx_ = nullptr;

    std::vector<uint32_t> valueHeapIds_;
    JsGlobalObject* pNativeGlobal_ = nullptr;

    mutable std::mutex cleanupLock_;
    bool isJsAvailable_ = false;
};

} // namespace mozjs
