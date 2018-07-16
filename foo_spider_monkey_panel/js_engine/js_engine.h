#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <map>
#include <functional>
#include <mutex>

class js_panel_window;


namespace mozjs
{

class JsContainer;

class JsEngine final
{
public:
    ~JsEngine();

    static JsEngine& GetInstance();
    void PrepareForExit();

public:
    bool RegisterPanel( js_panel_window& parentPanel, JsContainer& jsContainer );
    void UnregisterPanel( js_panel_window& parentPanel );

public:
    void OnHeapAllocate( uint32_t size );
    void OnHeapDeallocate( uint32_t size );
    void MaybeIncrementalGC(JSContext* cx);

private:
    JsEngine();
    JsEngine( const JsEngine& ) = delete;

private:
    bool Initialize();
    void Finalize();

private:
    JSContext * pJsCtx_ = nullptr;

    bool isInitialized_ = false;
    bool shouldShutdown_ = false;

    std::map<HWND, std::reference_wrapper<JsContainer>> registeredContainers_;

    std::mutex gcLock_;
    uint32_t lastGcCheckTime_ = 0;
    uint64_t curNativeHeapSize_ = 0;
    uint64_t lastTotalHeapSize_ = 0;
};

}
