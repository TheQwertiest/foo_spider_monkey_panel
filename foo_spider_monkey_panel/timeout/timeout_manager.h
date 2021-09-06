#pragma once

#include <events/event.h>
#include <timeout/time_types.h>

#include <chrono>
#include <map>
#include <unordered_map>

// TODO: add nesting clamp (<https://searchfox.org/mozilla-central/source/dom/base/TimeoutManager.cpp>)

namespace mozjs
{

class JsGlobalObject;
class JsAsyncTask;

} // namespace mozjs

namespace smp
{

class Timeout;
class TimeoutExecutor;

class TimeoutManager
{
public:
    TimeoutManager( std::shared_ptr<PanelTarget> pTarget );
    ~TimeoutManager();

    void Finalize();

    void SetLoadingStatus( bool isLoading );

    [[nodiscard]] uint32_t SetInterval( uint32_t interval, std::unique_ptr<mozjs::JsAsyncTask> pJsTask );
    [[nodiscard]] uint32_t SetTimeout( uint32_t delay, std::unique_ptr<mozjs::JsAsyncTask> pJsTask );

    void ClearTimeout( uint32_t timerId );
    void StopAllTimeouts();

    // Should be called only by TimeoutExecutor
    void RunTimeout( const TimeStamp& now,
                     const TimeStamp& targetDeadline );

private:
    [[nodiscard]] uint32_t CreateTimeout( uint32_t interval, bool isRepeated, std::unique_ptr<mozjs::JsAsyncTask> pJsTask );

    void MaybeSchedule( const TimeStamp& whenToTrigger );
    bool RescheduleTimeout( Timeout& timeout,
                            const TimeStamp& lastCallbackTime,
                            const TimeStamp& currentNow );

    [[nodiscard]] uint32_t CreateFiringId();
    void DestroyFiringId( uint32_t id );
    [[nodiscard]] bool IsValidFiringId( uint32_t id ) const;

private:
    class TimeoutStorage
    {
    public:
        using TimeoutList = std::list<std::shared_ptr<Timeout>>;
        using TimeoutIterator = TimeoutList::iterator;

        TimeoutStorage( TimeoutManager& pParent );

        [[nodiscard]] TimeoutIterator Get( uint32_t id );
        [[nodiscard]] TimeoutIterator GetFirst();
        [[nodiscard]] TimeoutIterator GetLast();
        [[nodiscard]] TimeoutIterator GetNext( const TimeoutIterator& it );
        [[nodiscard]] bool IsEnd( const TimeoutIterator& it ) const;
        void Insert( std::shared_ptr<Timeout> pTimeout );
        void Erase( TimeoutIterator it );
        [[nodiscard]] bool IsEmpty() const;
        void Clear();

    private:
        TimeoutManager& pParent_;

        TimeoutList schedule_;
        std::unordered_map<uint32_t, TimeoutIterator> idToIterator;
    };

private:
    std::shared_ptr<PanelTarget> pTarget_;

    bool isLoading_ = true;

    TimeoutStorage timeoutStorage_;
    std::vector<std::shared_ptr<Timeout>> delayedTimeouts_;

    uint32_t curTimerId_ = 1;

    static constexpr uint32_t kInvalidFiringId = 0;
    uint32_t nextFiringId = 1;
    std::vector<uint32_t> activeFiringIds_;

    std::shared_ptr<TimeoutExecutor> pExecutor_;
};

} // namespace smp
