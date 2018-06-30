#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <shared_mutex>


class js_panel_window;

namespace mozjs
{

class IHeapUser
{
public:
    IHeapUser()
    {

    }
    virtual ~IHeapUser()
    {

    }
    virtual void DisableHeapCleanup() = 0;
};

class JsContainer;

class JsGlobalObject
{
public:
    ~JsGlobalObject();

    static JSObject* Create( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel );

    static const JSClass& GetClass();

public:
    void Fail( std::string_view errorText);

    void RegisterHeapUser( IHeapUser* heapUser );
    void UnregisterHeapUser( IHeapUser* heapUser );

    uint32_t JsGlobalObject::StoreToHeap( JS::HandleValue valueToStore );
    JS::Heap<JS::Value>& GetFromHeap( uint32_t id );
    void RemoveFromHeap( uint32_t id );

    void RemoveHeapTracer();

private:
    JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel );
    JsGlobalObject( const JsGlobalObject& ) = delete;   
    JsGlobalObject& operator=( const JsGlobalObject& ) = delete;

    static void TraceHeapValue( JSTracer *trc, void *data );

private:
    JSContext * pJsCtx_ = nullptr;;
    JsContainer &parentContainer_;
    js_panel_window& parentPanel_;
    
    uint32_t currentHeapId_;    
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
    std::unordered_map<uint32_t, std::shared_ptr<HeapElement>> heapElements_;

    std::mutex heapUsersLock_;
    std::unordered_map<IHeapUser*, IHeapUser*> heapUsers_;
};

}
