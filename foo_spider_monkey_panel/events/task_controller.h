#pragma once

#include <events/event.h>

#include <mutex>
#include <set>

namespace smp
{

class TaskController;
class js_panel_window;

class Task
{
public:
    Task( EventPriority priority = EventPriority::kNormal );
    virtual ~Task() = default;

    EventPriority GetPriority() const;
    int64_t GetTaskNumber() const;
    virtual void Run( panel::js_panel_window& panelWindow ) = 0;

protected:
    friend class TaskController;

private:
    static std::atomic<uint64_t> g_currentTaskNumber;

    EventPriority priority_;
    int64_t taskNumber_;

    struct PriorityCompare
    {
        bool operator()( const std::shared_ptr<Task>& a,
                         const std::shared_ptr<Task>& b ) const;
    };
    std::set<std::shared_ptr<Task>, Task::PriorityCompare>::iterator taskIterator_;
    bool isInProgress_ = false;
};

class RunnableTask final : public Task
{
public:
    RunnableTask( std::shared_ptr<Runnable> pRunnable, EventPriority priority = EventPriority::kNormal );

    void Run( panel::js_panel_window& panelWindow ) override;

private:
    std::shared_ptr<Runnable> pRunnable_;
};

// https://searchfox.org/mozilla-central/source/xpcom/threads/TaskController.cpp
class TaskController : public std::enable_shared_from_this<TaskController>
{
public:
    TaskController() = default;

    void AddTask( std::shared_ptr<Task> pTask );
    void AddRunnable( std::shared_ptr<Runnable> pRunnable, EventPriority priority );

    bool HasTasks() const;

    bool ExecuteNextTask( panel::js_panel_window& panelWindow );

private:
    mutable std::mutex tasksMutex_;
    std::set<std::shared_ptr<Task>, Task::PriorityCompare> tasks_;
};

} // namespace smp
