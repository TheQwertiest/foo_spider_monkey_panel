#pragma once

#include <map>
#include <functional>
#include <mutex>

class js_panel_window;
struct JSContext;


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
    void MaybeGc();

private:
    JsEngine();
    JsEngine( const JsEngine& ) = delete;

private:
    bool Initialize();
    void Finalize();

private:
    enum class GcLevel: uint8_t
    {
        Incremental,
        Normal,
        Full
    };

    uint64_t GetCurrentTotalHeapSize();
    void PerformGc( GcLevel gcLevel );
    void PerformIncrementalGc();
    void PerformNormalGc();
    void PerformFullGc();
    void PrepareCompartmentsForGc( GcLevel gcLevel );
    void NotifyCompartmentsOnGcEnd();

private:
    JSContext * pJsCtx_ = nullptr;

    bool isInitialized_ = false;
    bool shouldShutdown_ = false;

    std::map<HWND, std::reference_wrapper<JsContainer>> registeredContainers_;
    
    uint32_t lastGcCheckTime_ = 0;
    uint64_t lastTotalHeapSize_ = 0;
};

}
