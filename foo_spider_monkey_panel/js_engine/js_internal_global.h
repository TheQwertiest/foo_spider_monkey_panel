#pragma once

#include <js/GCHashTable.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

struct JSContext;

namespace mozjs
{

class JsRealmInner;
class JsGlobalObject;

class JsInternalGlobal
{
public:
    /// @throw qwr::QwrException
    /// @throw smp::JsException
    static [[nodiscard]] std::unique_ptr<JsInternalGlobal> Create( JSContext* cx );

    ~JsInternalGlobal();

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] JSScript* GetCachedScript( const std::filesystem::path& absolutePath );

private:
    JsInternalGlobal( JSContext* cx, JS::HandleObject global );

private:
    JSContext* pJsCtx_ = nullptr;
    JS::PersistentRootedObject jsGlobal_;

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
        std::unordered_map<qwr::u8string, ValueType> data;

        void trace( JSTracer* trc )
        {
            for ( auto& entry: data )
            {
                JS::TraceEdge( trc, &entry.second.script, "map value" );
            }
        }
    };

    JS::PersistentRooted<JsHashMap> scriptCache_;
};

} // namespace mozjs
