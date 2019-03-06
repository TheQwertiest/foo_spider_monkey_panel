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
        std::unordered_map<std::string, JS::Heap<JSScript*>> data;

        void trace( JSTracer* trc )
        {
            for ( auto entry : data )
            {
                JS::TraceEdge( trc, &entry.second, "map value" );
            }
        }
    };

    //using JsHashMap = JS::GCHashMap<const char*, JSScript*, js::CStringHasher>;
    JS::PersistentRooted<JsHashMap> scriptCache_;
};

} // namespace mozjs