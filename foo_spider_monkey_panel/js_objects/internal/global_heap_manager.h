#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <shared_mutex>
#include <optional>
#include <set>


class js_panel_window;
class ActiveX;

namespace mozjs
{

class IHeapUser
{
public:
    IHeapUser() = default;
    virtual ~IHeapUser() = default;
    virtual void DisableHeapCleanup() = 0;
};

class GlobalHeapManager
{
public:
    ~GlobalHeapManager();
    GlobalHeapManager(const GlobalHeapManager& ) = delete;
    GlobalHeapManager& operator=( const GlobalHeapManager& ) = delete;

    static std::unique_ptr<GlobalHeapManager> Create( JSContext * cx );
    void RemoveTracer();
public:
    void RegisterUser( IHeapUser* heapUser );
    void UnregisterUser( IHeapUser* heapUser );

    uint32_t Store( JS::HandleValue valueToStore );
    JS::Heap<JS::Value>& Get( uint32_t id );
    void Remove( uint32_t id );

private:
    GlobalHeapManager( JSContext * cx );

    static void TraceHeapValue( JSTracer *trc, void *data );

private: 
    JSContext * pJsCtx_ = nullptr;;

    uint32_t currentHeapId_ = 0;
    struct HeapElement
    {
        HeapElement( JS::HandleValue inValue )
            : inUse( true )
            , value( inValue )
        {
        }

        bool inUse;
        JS::Heap<JS::Value> value;
    };
    std::mutex heapElementsLock_;
    std::unordered_map<uint32_t, std::unique_ptr<HeapElement>> heapElements_;

    std::mutex heapUsersLock_;
    std::unordered_map<IHeapUser*, IHeapUser*> heapUsers_;
};

}
