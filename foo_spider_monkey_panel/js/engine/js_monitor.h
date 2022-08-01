#pragma once

#include <functional>
#include <map>
#include <mutex>

struct JSContext;

namespace smp
{

class HeartbeatWindow;
}

namespace mozjs
{

class JsContainer;
class JsInternalGlobal;

class JsMonitor final
{
public:
    JsMonitor();
    ~JsMonitor() = default;
    JsMonitor( const JsMonitor& ) = delete;
    JsMonitor& operator=( const JsMonitor& ) = delete;

    /// @detail Assumes that JSContext is freshly created
    ///
    /// @throw qwr::QwrException
    void Start( JSContext* cx );
    void Stop();

    void AddContainer( JsContainer& jsContainer );
    void RemoveContainer( JsContainer& jsContainer );

    void OnJsActionStart( JsContainer& jsContainer );
    void OnJsActionEnd( JsContainer& jsContainer );

    [[nodiscard]] bool OnInterrupt();

private:
    /// @throw qwr::QwrException
    void StartMonitorThread();
    void StopMonitorThread();

    [[nodiscard]] bool HasActivePopup( bool isMainThread ) const;

private:
    JSContext* pJsCtx_ = nullptr;
    HWND hFb2k_ = nullptr;
    const std::chrono::seconds slowScriptLimit_;

    struct ContainerData
    {
        ContainerData( JsContainer* pContainer )
            : pContainer( pContainer )
        {
        }

        JsContainer* pContainer;

        bool ignoreSlowScriptCheck = false;
        std::chrono::milliseconds slowScriptCheckpoint{};
        bool slowScriptSecondHalf = false;
    };

    std::unordered_map<JsContainer*, ContainerData> monitoredContainers_;

    std::mutex watcherDataMutex_;
    std::thread watcherThread_;
    std::atomic_bool shouldStopThread_ = false;
    std::condition_variable hasAction_;
    // Contains the same time as slowScriptCheckpoint in monitoredContainers_
    std::unordered_map<JsContainer*, std::chrono::milliseconds> activeContainers_;
    bool isInInterrupt_ = false;

    std::atomic_bool wasInModal_ = false;
};

} // namespace mozjs
