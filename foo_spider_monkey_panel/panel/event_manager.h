#pragma once

#include <panel/event.h>

#include <mutex>
#include <unordered_map>

// TODO: add on_size and mouse_move coalescing (https://searchfox.org/mozilla-central/source/dom/ipc/PBrowser.ipdl)
// TODO: add dynamic input task prioritization (kInputLow)

namespace smp::panel
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
    void AddWindow( HWND hWnd, js_panel_window& panelWindow );
    void RemoveWindow( HWND hWnd );

    void EnableEventQueue( HWND hWnd );
    void DisableEventQueue( HWND hWnd );

public:
    static bool IsRequestEventMessage( UINT msg );
    bool ProcessNextEvent( HWND hWnd );
    void RequestNextEvent( HWND hWnd );

public:
    // these can be invoked from worker threads
    void PutEvent( HWND hWnd, std::unique_ptr<Runnable> event, EventPriority priority = EventPriority::kNormal );
    void PutEventToOthers( HWND hWnd, std::unique_ptr<Runnable> event, EventPriority priority = EventPriority::kNormal );
    void PutEventToAll( std::unique_ptr<Runnable> event, EventPriority priority = EventPriority::kNormal );

private:
    std::mutex taskControllerMapMutex_;
    std::unordered_map<HWND, js_panel_window*> windowMap_;
    std::unordered_map<HWND, std::shared_ptr<TaskController>> taskControllerMap_;
};

} // namespace smp::panel
