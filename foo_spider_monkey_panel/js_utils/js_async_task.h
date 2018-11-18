#pragma once

#include <js_objects/internal/global_heap_manager.h>

#include <array>

namespace mozjs
{

class JsGlobalObject;

class JsAsyncTask
{
public:
    JsAsyncTask() = default;
    virtual ~JsAsyncTask() = default;

    virtual void InvokeJs() = 0;
    virtual void PrepareForGlobalGc() = 0;
};

template <typename... Args>
class JsAsyncTaskImpl
    : public JsAsyncTask
{
public:
    JsAsyncTaskImpl( JSContext* cx, Args... args )
        : pJsCtx_( cx )
    {
        static_assert( ( std::is_same_v<Args, JS::HandleValue> && ... ) );

        assert( cx );

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
        assert( pNativeGlobal_ );

        valueHeapIds_ = { pNativeGlobal_->GetHeapManager().Store( args )... };

        isJsAvailable_ = true;
    }

    ~JsAsyncTaskImpl() override
    {
        if ( isJsAvailable_ )
        {
            for ( auto heapId : valueHeapIds_ )
            {
                pNativeGlobal_->GetHeapManager().Remove( heapId );
            }
        }
    };

    /// @details Assumes that JS environment is ready (global, compartment and etc).
    void InvokeJs() override
    {
        JS::RootedObject jsGlobal( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
        assert( jsGlobal );
        JS::RootedValue vFunc( pJsCtx_, pNativeGlobal_->GetHeapManager().Get( valueHeapIds_[0] ) );

        InvokeJsInternal( jsGlobal, std::make_index_sequence<sizeof...( Args )>{} );
    }

    void PrepareForGlobalGc() override
    {
        isJsAvailable_ = false;
    }

private:
    virtual void InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, Args... args ) = 0;

    template <size_t... Indices>
    void InvokeJsInternal( JS::HandleObject jsGlobal, std::index_sequence<Indices...> )
    {
        InvokeJsImpl( pJsCtx_, jsGlobal, JS::RootedValue{ pJsCtx_, pNativeGlobal_->GetHeapManager().Get( std::get<Indices>( valueHeapIds_ ) ) }... );
    }

private:
    JSContext* pJsCtx_ = nullptr;

    std::array<uint32_t, sizeof...( Args )> valueHeapIds_ = {};
    JsGlobalObject* pNativeGlobal_ = nullptr;

    bool isJsAvailable_ = false;
};

} // namespace mozjs
