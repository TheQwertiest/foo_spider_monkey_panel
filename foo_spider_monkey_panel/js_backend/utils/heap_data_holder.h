#pragma once

#include <js_backend/engine/global_heap_manager.h>

namespace mozjs
{

class JsGlobalObject;
class HeapDataHolderImpl;

template <typename T>
class HeapDataHolder_Struct
{
public:
    HeapDataHolder_Struct( JSContext* cx, smp::not_null_unique<T> pData )
        : holder_( cx, std::move( pData ) )
    {
    }

    HeapDataHolder_Struct( HeapDataHolder_Struct&& other )
        : holder_( std::move( other.holder_ ) )
    {
    }

    [[nodiscard]] T& Get()
    {
        assert( core_api::is_main_thread() );
        return static_cast<T&>( holder_.Get() );
    }

private:
    HeapDataHolderImpl holder_;
};

template <typename T>
class HeapDataHolder_JsEntity
{
    struct Data : IHeapTraceableData
    {
        MJS_HEAP_TRACER_BEGIN
        MJS_HEAP_TRACE_VALUE( jsEntity, "data holder entity" )
        MJS_HEAP_TRACER_END

        Data( JS::Handle<T> jsEntity )
            : jsEntity( jsEntity )
        {
        }

        JS::Heap<T> jsEntity;
    };

public:
    HeapDataHolder_JsEntity( JSContext* cx, JS::Handle<T> jsEntity )
        : holder_( cx, smp::make_not_null_unique<Data>( jsEntity ) )
    {
    }

    HeapDataHolder_JsEntity( HeapDataHolder_JsEntity&& other )
        : holder_( std::move( other.holder_ ) )
    {
    }

    [[nodiscard]] T Get()
    {
        assert( core_api::is_main_thread() );
        return static_cast<Data&>( holder_.Get() ).jsEntity;
    }

private:
    HeapDataHolderImpl holder_;
};

using HeapDataHolder_Object = HeapDataHolder_JsEntity<JSObject*>;
using HeapDataHolder_Value = HeapDataHolder_JsEntity<JS::Value>;

class HeapDataHolderImpl : public IHeapUser
{
public:
    HeapDataHolderImpl( JSContext* cx, smp::not_null_unique<IHeapTraceableData> pData );
    HeapDataHolderImpl( const HeapDataHolderImpl& other ) = delete;
    HeapDataHolderImpl( HeapDataHolderImpl&& other );
    ~HeapDataHolderImpl() override;

    [[nodiscard]] IHeapTraceableData& Get();

    [[nodiscard]] bool IsJsAvailable() const;

    // might be called from worker thread
    void Finalize();

    void PrepareForGlobalGc() final;

private:
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<IHeapTraceableData*> pData_;
    JsGlobalObject* pNativeGlobal_ = nullptr;

    mutable std::mutex cleanupLock_;
    bool isJsAvailable_ = true;
    bool hasData_ = true;
};

} // namespace mozjs
