#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

#include <js/GCHashTable.h>

struct JSContext;

namespace mozjs
{

class JsCompartmentInner;
class JsGlobalObject;

class JsInternalGlobal
{
public:
    static std::unique_ptr<JsInternalGlobal> Create( JSContext* cx ) noexcept( false );

    ~JsInternalGlobal();

    void OnSharedAllocate( uint32_t allocationSize );
    void OnSharedDeallocate( uint32_t allocationSize );

    JSScript* GetCachedScript( const std::filesystem::path& absolutePath ) noexcept( false );

private:
    JsInternalGlobal( JSContext* cx, JS::HandleObject global, JsCompartmentInner* pNativeCompartment );

private:
    JSContext* pJsCtx_ = nullptr;
    JS::PersistentRootedObject jsGlobal_;
    JsCompartmentInner* pNativeCompartment_ = nullptr;

    struct JsHashMap
    {
        struct ValueType
        {
            template <typename T1, typename T2>
            ValueType( T1&& arg1, T2&& arg2 )
                : script( std::forward<T1>( arg1 ) )
                , writeTime( std::forward<T2>( arg2 ) )
            {
            }

            JS::Heap<JSScript*> script;
            std::filesystem::file_time_type writeTime;
        };
        std::unordered_map<std::string, ValueType> data;

        void trace( JSTracer* trc )
        {
            for ( auto& entry : data )
            {
                JS::TraceEdge( trc, &entry.second.script, "map value" );
            }
        }
    };

    JS::PersistentRooted<JsHashMap> scriptCache_;
};

} // namespace mozjs
