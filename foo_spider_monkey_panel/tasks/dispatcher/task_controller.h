#pragma once

#include <panel/panel_fwd.h>
#include <tasks/dispatcher/event_priority.h>
#include <tasks/events/event.h>

#include <mutex>
#include <set>

namespace smp
{

class TaskController;

}

namespace smp
{

class Task
{
public:
    Task( EventPriority priority = EventPriority::kNormal );
    virtual ~Task() = default;

    virtual void Run() = 0;

protected:
    friend class TaskController;

private:
    static std::atomic<uint64_t> g_currentTaskNumber;

    const EventPriority priority_;
    const uint64_t taskNumber_;

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

    void Run() override;

private:
    std::shared_ptr<Runnable> pRunnable_;
};

class TaskController : public std::enable_shared_from_this<TaskController>
{
public:
    TaskController( smp::not_null_shared<panel::PanelAccessor> pTarget );

    [[nodiscard]] smp::not_null_shared<panel::PanelAccessor> GetTarget();

    void AddTask( std::shared_ptr<Task> pTask );
    void AddRunnable( std::shared_ptr<Runnable> pRunnable, EventPriority priority );

    [[nodiscard]] bool HasTasks() const;

    bool ExecuteNextTask();

private:
    smp::not_null_shared<panel::PanelAccessor> pTarget_;

    mutable std::mutex tasksMutex_;
    std::set<std::shared_ptr<Task>, Task::PriorityCompare> tasks_;
};

} // namespace smp
