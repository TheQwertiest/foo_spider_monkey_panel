#pragma once

#include <events/event.h>

#include <mutex>
#include <unordered_map>

// TODO: add on_size and mouse_move (drag_over as well?) coalescing (https://searchfox.org/mozilla-central/source/dom/ipc/PBrowser.ipdl)
// TODO: add dynamic input task prioritization (kInputLow)

namespace smp
{

class TaskController;
class js_panel_window;

class EventDispatcher
{
public:
    EventDispatcher() = default;
    EventDispatcher( const EventDispatcher& ) = delete;
    EventDispatcher& operator=( const EventDispatcher& ) = delete;

    static [[nodiscard]] EventDispatcher& Get();

public:
    void AddWindow( HWND hWnd, std::shared_ptr<PanelTarget> pTarget );
    void RemoveWindow( HWND hWnd );

    void NotifyAllAboutExit();

public:
    static [[nodiscard]] bool IsRequestEventMessage( UINT msg );
    bool ProcessNextEvent( HWND hWnd );
    void RequestNextEvent( HWND hWnd );
    void OnRequestEventMessageReceived( HWND hWnd );

public: // these can be invoked from worker threads
    void PutRunnable( HWND hWnd, std::shared_ptr<Runnable> pRunnable, EventPriority priority = EventPriority::kNormal );
    void PutEvent( HWND hWnd, std::unique_ptr<EventBase> pEvent, EventPriority priority = EventPriority::kNormal );

    /// @remark Be careful when using this:
    ///         - Event must be cloneable.
    ///         - Clone operation should not be CPU intensive (e.g. don't copy vectors, but rather wrap it in shared_ptr)
    void PutEventToAll( std::unique_ptr<EventBase> pEvent, EventPriority priority = EventPriority::kNormal );

    /// @remark Be careful when using this:
    ///         - Event must be cloneable.
    ///         - Clone operation should not be CPU intensive (e.g. don't copy vectors, but rather wrap it in shared_ptr)
    void PutEventToOthers( HWND hWnd, std::unique_ptr<EventBase> pEvent, EventPriority priority = EventPriority::kNormal );

public:
    // TODO: remove in v2
    /// @remark This is a compatibility hack for window.NotifyOthers()
    void NotifyOthers( HWND hWnd, std::unique_ptr<EventBase> pEvent );

private:
    void RequestNextEventImpl( HWND hWnd, TaskController& taskController, std::scoped_lock<std::mutex>& proof );

private:
    std::mutex taskControllerMapMutex_;
    std::unordered_map<HWND, std::shared_ptr<TaskController>> taskControllerMap_;

    using IsWaitingForNextMsg = bool;
    std::unordered_map<HWND, IsWaitingForNextMsg> nextEventMsgStatusMap_;
};

} // namespace smp
