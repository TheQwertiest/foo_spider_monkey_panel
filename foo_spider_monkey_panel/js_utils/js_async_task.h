#pragma once

#include <js_objects/internal/global_heap_manager.h>
#include <js_objects/global_object.h>

#include <array>

namespace mozjs
{

class JsAsyncTask
    : public IHeapUser
{
public:
    JsAsyncTask() = default;
    ~JsAsyncTask() override = default;

    virtual bool InvokeJs() = 0;
    virtual bool IsCanceled() const = 0;
};

template <typename... Args>
class JsAsyncTaskImpl
    : public JsAsyncTask
{
    static_assert( ( std::is_same_v<Args, JS::HandleValue> && ... ) );

public:
    /// throws JsException
    JsAsyncTaskImpl( JSContext* cx, Args... args )
        : pJsCtx_( cx )
    {
        assert( cx );

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
        assert( pNativeGlobal_ );

        valueHeapIds_ = { pNativeGlobal_->GetHeapManager().Store( args )... };

        pNativeGlobal_->GetHeapManager().RegisterUser( this );

        isJsAvailable_ = true;
    }

    ~JsAsyncTaskImpl() override
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
    };

    /// @details Assumes that JS environment is ready (global, realm and etc).
    bool InvokeJs() final
    {
        std::scoped_lock sl( cleanupLock_ );
        if ( !isJsAvailable_ )
        {
            return true;
        }

        JS::RootedObject jsGlobal( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
        assert( jsGlobal );

        return InvokeJsInternal( jsGlobal, std::make_index_sequence<sizeof...( Args )>{} );
    }

    void PrepareForGlobalGc() final
    {
        std::scoped_lock sl( cleanupLock_ );
        isJsAvailable_ = false;
    }

    bool IsCanceled() const final
    {
        std::scoped_lock sl( cleanupLock_ );
        return isJsAvailable_;
    }

private:
    virtual bool InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, Args... args ) = 0;

    template <size_t... Indices>
    bool InvokeJsInternal( JS::HandleObject jsGlobal, std::index_sequence<Indices...> )
    {
        return InvokeJsImpl( pJsCtx_, jsGlobal, JS::RootedValue{ pJsCtx_, pNativeGlobal_->GetHeapManager().Get( std::get<Indices>( valueHeapIds_ ) ) }... );
    }

private:
    JSContext* pJsCtx_ = nullptr;

    std::array<uint32_t, sizeof...( Args )> valueHeapIds_ = {};
    JsGlobalObject* pNativeGlobal_ = nullptr;

    mutable std::mutex cleanupLock_;
    bool isJsAvailable_ = false;
};

} // namespace mozjs
