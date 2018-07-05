#include "stdafx.h"
#include "host_timer_dispatcher.h"

#include <js_objects/global_object.h>

#include "user_message.h"

HostTimerDispatcher::HostTimerDispatcher()
{
    m_curTimerId = 1;
    m_hTimerQueue = CreateTimerQueue();
}

HostTimerDispatcher::~HostTimerDispatcher()
{
    stopThread();
    // Clear all references
    m_taskMap.clear();
}

HostTimerDispatcher& HostTimerDispatcher::Get()
{
    static HostTimerDispatcher timerDispatcher;
    return timerDispatcher;
}

uint32_t HostTimerDispatcher::setInterval(HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction )
{
    return createTimer(hWnd, delay, true, cx, jsFunction );
}

uint32_t HostTimerDispatcher::setTimeout(HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction )
{
    return createTimer(hWnd, delay, false, cx, jsFunction );
}

void HostTimerDispatcher::killTimer(uint32_t timerId)
{
    {
        std::lock_guard<std::mutex> lock(m_timerMutex);

        auto timerIter = m_timerMap.find(timerId);
        if (m_timerMap.end() != timerIter)
        {
            timerIter->second->stop();
        }
    }

    auto taskIter = m_taskMap.find(timerId);
    if (m_taskMap.end() != taskIter)
    {
        taskIter->second->release();
    }
}

void HostTimerDispatcher::onPanelUnload(HWND hWnd)
{
    std::vector<uint32_t> timersToDelete;

    {
        std::lock_guard<std::mutex> lock(m_timerMutex);
        
        for (const auto& elem : m_timerMap)
        {
            if (elem.second->GetHwnd() == hWnd)
            {
                timersToDelete.push_back(elem.first);
            }
        }
    }

    for (auto timerId : timersToDelete)
    {
        killTimer(timerId);
    }
}

void HostTimerDispatcher::onInvokeMessage( uint32_t timerId)
{
    if (m_taskMap.count(timerId))
    {
        m_taskMap[timerId]->invoke();
    }
}

void HostTimerDispatcher::onTimerExpire(uint32_t timerId)
{
    std::unique_lock<std::mutex> lock(m_timerMutex);

    m_timerMap.erase(timerId);
}

void HostTimerDispatcher::onTimerStopRequest(HWND hWnd, HANDLE hTimer, uint32_t timerId)
{
    std::unique_lock<std::mutex> lock(m_threadTaskMutex);

    ThreadTask threadTask = {};
    threadTask.taskId = ThreadTaskId::killTimerTask;
    threadTask.hWnd = hWnd;
    threadTask.hTimer = hTimer;
    threadTask.timerId = timerId;

    m_threadTaskList.push_front(threadTask);
    m_cv.notify_one();
}

void HostTimerDispatcher::onTaskComplete(uint32_t timerId)
{
    m_taskMap.erase(timerId);
}

uint32_t HostTimerDispatcher::createTimer(HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction )
{
    if ( !jsFunction )
    {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_timerMutex);

    uint32_t id = m_curTimerId++;
    while (m_taskMap.count(id) && m_timerMap.count(id))
    {
        id = m_curTimerId++;
    }

    m_timerMap.emplace(id, new HostTimer(hWnd, id, delay, isRepeated));

    auto & curTask = m_taskMap.emplace(id, new HostTimerTask( id, cx, jsFunction ));
    curTask.first->second->acquire();

    if (!m_timerMap[id]->start(m_hTimerQueue))
    {
        m_timerMap.erase(id);
        m_taskMap.erase(id);
        return 0;
    }

    return id;
}

void HostTimerDispatcher::createThread()
{
    m_thread = new std::thread(&HostTimerDispatcher::threadMain, this);
}

void HostTimerDispatcher::stopThread()
{
    if (!m_thread)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_threadTaskMutex);
        ThreadTask threadTask = {};
        threadTask.taskId = ThreadTaskId::shutdownTask;

        m_threadTaskList.push_front(threadTask);
        m_cv.notify_one();
    }

    if (m_thread->joinable())
    {
        m_thread->join();
    }

    delete m_thread;
    m_thread = NULL;
}

void HostTimerDispatcher::threadMain()
{
    while (true)
    {
        ThreadTask threadTask;
        {
            std::unique_lock<std::mutex> lock(m_threadTaskMutex);

            while (m_threadTaskList.empty())
            {
                m_cv.wait(lock);
            }

            if (m_threadTaskList.empty())
            {
                continue;
            }

            threadTask = m_threadTaskList.front();
            m_threadTaskList.pop_front();
        }

        switch (threadTask.taskId)
        {
        case ThreadTaskId::killTimerTask:
        {
            DeleteTimerQueueTimer(m_hTimerQueue, threadTask.hTimer, INVALID_HANDLE_VALUE);
            onTimerExpire(threadTask.timerId);
            break;
        }
        case ThreadTaskId::shutdownTask:
        {
            DeleteTimerQueueEx(m_hTimerQueue, INVALID_HANDLE_VALUE);
            m_hTimerQueue = NULL;
            return;
        }
        default:
        {
            assert(0);
            break;
        }
        }
    }
}

HostTimerTask::HostTimerTask( uint32_t timerId, JSContext* cx, JS::HandleFunction jsFunction )
    : m_timerId(timerId)
    , pJsCtx_(cx)
{
    assert( cx );

    JS::RootedObject funcObject( cx, JS_GetFunctionObject( jsFunction ) );
    JS::RootedValue funcValue( cx, JS::ObjectValue( *funcObject ) );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );
    JS::RootedValue globalValue( cx, JS::ObjectValue( *jsGlobal ) );

    pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::GetClass(), nullptr ) );
    assert( pNativeGlobal_ );

    funcId_ = pNativeGlobal_->StoreToHeap( funcValue );
    globalId_ = pNativeGlobal_->StoreToHeap( globalValue );
}

HostTimerTask::~HostTimerTask()
{    
}

void HostTimerTask::acquire()
{
    ++m_refCount;
}

void HostTimerTask::release()
{
    if (!m_refCount)
    {
        return;
    }

    --m_refCount;
    if (!m_refCount)
    {
        pNativeGlobal_->RemoveFromHeap( funcId_ );
        pNativeGlobal_->RemoveFromHeap( globalId_ );
        HostTimerDispatcher::Get().onTaskComplete(m_timerId);
    }
}

void HostTimerTask::invoke()
{
    acquire();
    
    JSAutoRequest ar( pJsCtx_ );
    JS::RootedObject jsGlobal( pJsCtx_, pNativeGlobal_->GetFromHeap( globalId_ ).toObjectOrNull() );
    assert( jsGlobal );
    JSAutoCompartment ac( pJsCtx_, jsGlobal );

    JS::RootedValue vFunc( pJsCtx_, pNativeGlobal_->GetFromHeap( funcId_ ) );
    JS::RootedFunction rFunc( pJsCtx_, JS_ValueToFunction( pJsCtx_, vFunc ) );

    JS::RootedValue retVal( pJsCtx_ );
    // Should be checked for exceptions by the caller
    JS::Call( pJsCtx_, jsGlobal, rFunc, JS::HandleValueArray::empty(), &retVal );

    release();
}

HostTimer::HostTimer(HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated)
{
    m_hTimer = 0;

    m_hWnd = hWnd;
    m_delay = delay;
    m_isRepeated = isRepeated;
    m_id = id;

    m_isStopRequested = false;
    m_isStopped = false;
}

HostTimer::~HostTimer()
{

}

bool HostTimer::start(HANDLE hTimerQueue)
{
    return !!CreateTimerQueueTimer(
        &m_hTimer,
        hTimerQueue,
        HostTimer::timerProc,
        this,
        m_delay,
        m_isRepeated ? m_delay : 0,
        WT_EXECUTEINTIMERTHREAD | (m_isRepeated ? 0 : WT_EXECUTEONLYONCE));
}

void HostTimer::stop()
{
    m_isStopRequested = true;
}

VOID CALLBACK HostTimer::timerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    HostTimer* timer = (HostTimer*)lpParameter;

    if (timer->m_isStopped)
    {
        return;
    }

    if (timer->m_isStopRequested)
    {
        timer->m_isStopped = true;
        HostTimerDispatcher::Get().onTimerStopRequest(timer->m_hWnd, timer->m_hTimer, timer->m_id);

        return;
    }

    if (!timer->m_isRepeated)
    {
        timer->m_isStopped = true;
        SendMessage(timer->m_hWnd, UWM_TIMER, timer->m_id, 0);
        HostTimerDispatcher::Get().onTimerStopRequest(timer->m_hWnd, timer->m_hTimer, timer->m_id);

        return;
    }

    SendMessage(timer->m_hWnd, UWM_TIMER, timer->m_id, 0);
}

HWND HostTimer::GetHwnd() const
{
    return m_hWnd;
}

HANDLE HostTimer::GetHandle() const
{
    return m_hTimer;
}
