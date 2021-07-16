#pragma once

#include <events/event.h>

#include <mutex>
#include <unordered_map>

// TODO: add on_size and mouse_move coalescing (https://searchfox.org/mozilla-central/source/dom/ipc/PBrowser.ipdl)
// TODO: add dynamic input task prioritization (kInputLow)

namespace smp
{

class TaskController;
class js_panel_window;

class EventManager
{
public:
    EventManager() = default;
    EventManager( const EventManager& ) = delete;
    EventManager& operator=( const EventManager& ) = delete;

    static EventManager& Get();

public:
    void AddWindow( HWND hWnd );
    void RemoveWindow( HWND hWnd );

    void ClearEventQueue( HWND hWnd, std::shared_ptr<PanelTarget> pTarget );
    void DisableEventQueue( HWND hWnd );

public:
    static bool IsRequestEventMessage( UINT msg );
    bool ProcessNextEvent( HWND hWnd );
    void RequestNextEvent( HWND hWnd );

public: // these can be invoked from worker threads
    void PutEvent( HWND hWnd, std::unique_ptr<EventBase> pEvent, EventPriority priority = EventPriority::kNormal );

    /// @remark Be careful when using this:
    ///         - Event must be cloneable.
    ///         - Clone operation should not be CPU intensive (e.g. don't copy vectors, but rather wrap it in shared_ptr)
    void PutEventToAll( std::unique_ptr<EventBase> pEvent, EventPriority priority = EventPriority::kNormal );

private:
    std::mutex taskControllerMapMutex_;
    std::unordered_map<HWND, std::shared_ptr<TaskController>> taskControllerMap_;
};

} // namespace smp
