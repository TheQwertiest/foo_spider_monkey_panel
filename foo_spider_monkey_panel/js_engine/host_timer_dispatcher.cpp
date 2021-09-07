#include <stdafx.h>

#include "host_timer_dispatcher.h"

#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>
#include <panel/message_manager.h>
#include <panel/user_message.h>

#include <qwr/final_action.h>
#include <qwr/thread_helpers.h>
#include <qwr/winapi_error_helpers.h>

// TODO: move to JsEngine form global object

namespace smp
{

HostTimerDispatcher::HostTimerDispatcher()
    : m_hTimerQueue( CreateTimerQueue() )
{
    createThread();
}

HostTimerDispatcher::~HostTimerDispatcher()
{
    assert( !m_thread );
}

HostTimerDispatcher& HostTimerDispatcher::Get()
{
    static HostTimerDispatcher timerDispatcher;
    return timerDispatcher;
}

void HostTimerDispatcher::Finalize()
{
    stopThread();
}

uint32_t HostTimerDispatcher::setInterval( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs )
{
    return createTimer( hWnd, delay, true, cx, jsFunction, jsFuncArgs );
}

uint32_t HostTimerDispatcher::setTimeout( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs )
{
    return createTimer( hWnd, delay, false, cx, jsFunction, jsFuncArgs );
}

void HostTimerDispatcher::killTimer( uint32_t timerId )
{
    std::lock_guard<std::mutex> lock( m_timerMutex );

    auto it = m_timerMap.find( timerId );
    if ( m_timerMap.end() != it )
    {
        it->second->stop();
    }
}

void HostTimerDispatcher::onPanelUnload( HWND hWnd )
{
    std::vector<uint32_t> timersToDelete;

    {
        std::lock_guard<std::mutex> lock( m_timerMutex );

        for ( const auto& [timerId, timer]: m_timerMap )
        {
            if ( timer->GetHwnd() == hWnd )
            {
                timersToDelete.push_back( timerId );
            }
        }
    }

    for ( auto timerId: timersToDelete )
    {
        killTimer( timerId );
    }
}

void HostTimerDispatcher::onTimerExpire( uint32_t timerId )
{
    std::lock_guard<std::mutex> lock( m_timerMutex );

    m_timerMap.erase( timerId );
}

void HostTimerDispatcher::onTimerStopRequest( HWND hWnd, HANDLE hTimer, uint32_t timerId )
{
    {
        std::unique_lock lock( m_threadTaskMutex );

        ThreadTask threadTask = {};
        threadTask.taskId = ThreadTaskId::killTimerTask;
        threadTask.hWnd = hWnd;
        threadTask.hTimer = hTimer;
        threadTask.timerId = timerId;

        m_threadTaskList.push_front( threadTask );
    }
    m_cv.notify_all();
}

uint32_t HostTimerDispatcher::createTimer( HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs )
{
    assert( jsFunction );

    std::lock_guard<std::mutex> lock( m_timerMutex );

    const uint32_t id = [&] {
        uint32_t id = m_curTimerId++;
        while ( m_timerMap.contains( id ) )
        {
            id = m_curTimerId++;
        }
        return id;
    }();

    JS::RootedValue jsFuncValue( cx, JS::ObjectValue( *JS_GetFunctionObject( jsFunction ) ) );
    JS::RootedObject jsArrayObject( cx, JS_NewArrayObject( cx, jsFuncArgs ) );
    smp::JsException::ExpectTrue( jsArrayObject );
    JS::RootedValue jsArrayValue( cx, JS::ObjectValue( *jsArrayObject ) );

    auto [it, bDummy] = m_timerMap.emplace( id, std::make_unique<HostTimer>( hWnd, id, delay, isRepeated, std::make_unique<HostTimerTask>( cx, jsFuncValue, jsArrayValue ) ) );
    auto& [iDummy, timer] = *it;
    auto autoTimer = qwr::final_action( [&] { m_timerMap.erase( id ); } );

    timer->start( m_hTimerQueue );

    autoTimer.cancel();
    return id;
}

void HostTimerDispatcher::createThread()
{
    m_thread = std::make_unique<std::thread>( &HostTimerDispatcher::threadMain, this );
    qwr::SetThreadName( *m_thread, "SMP TimerHandler" );
}

void HostTimerDispatcher::stopThread()
{
    if ( !m_thread )
    {
        return;
    }

    {
        std::unique_lock<std::mutex> lock( m_threadTaskMutex );
        ThreadTask threadTask = {};
        threadTask.taskId = ThreadTaskId::shutdownTask;

        m_threadTaskList.push_front( threadTask );
    }
    m_cv.notify_all();

    if ( m_thread->joinable() )
    {
        m_thread->join();
    }

    m_thread.reset();
}

void HostTimerDispatcher::threadMain()
{
    while ( true )
    {
        const ThreadTask threadTask = [&] {
            std::unique_lock<std::mutex> lock( m_threadTaskMutex );

            m_cv.wait( lock, [&threadTaskList = m_threadTaskList] { return !threadTaskList.empty(); } );

            const auto threadTask = m_threadTaskList.front();
            m_threadTaskList.pop_front();
            return threadTask;
        }();

        switch ( threadTask.taskId )
        {
        case ThreadTaskId::killTimerTask:
        {
            assert( m_hTimerQueue );
            assert( threadTask.hTimer );
            (void)DeleteTimerQueueTimer( m_hTimerQueue, threadTask.hTimer, INVALID_HANDLE_VALUE );
            onTimerExpire( threadTask.timerId );
            break;
        }
        case ThreadTaskId::shutdownTask:
        {
            assert( m_hTimerQueue );
            (void)DeleteTimerQueueEx( m_hTimerQueue, INVALID_HANDLE_VALUE );
            m_hTimerQueue = nullptr;
            return;
        }
        default:
        {
            assert( 0 );
            break;
        }
        }
    }
}

HostTimerTask::HostTimerTask( JSContext* cx, JS::HandleValue funcValue, JS::HandleValue argArrayValue )
    : JsAsyncTaskImpl( cx, funcValue, argArrayValue )
{
}

bool HostTimerTask::InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue, JS::HandleValue argArrayValue )
{
    JS::RootedFunction jsFunc( cx, JS_ValueToFunction( cx, funcValue ) );
    JS::RootedObject jsArrayObject( cx, argArrayValue.toObjectOrNull() );
    assert( jsArrayObject );

    bool is;
    if ( !JS_IsArrayObject( cx, jsArrayObject, &is ) )
    {
        throw smp::JsException();
    }
    assert( is );

    uint32_t arraySize;
    if ( !JS_GetArrayLength( cx, jsArrayObject, &arraySize ) )
    {
        throw smp::JsException();
    }

    JS::RootedValueVector jsVector( cx );
    if ( arraySize )
    {
        if ( !jsVector.reserve( arraySize ) )
        {
            throw std::bad_alloc();
        }

        JS::RootedValue arrayElement( cx );
        for ( uint32_t i = 0; i < arraySize; ++i )
        {
            if ( !JS_GetElement( cx, jsArrayObject, i, &arrayElement ) )
            {
                throw smp::JsException();
            }

            if ( !jsVector.emplaceBack( arrayElement ) )
            {
                throw std::bad_alloc();
            }
        }
    }

    JS::RootedValue dummyRetVal( cx );
    return JS::Call( cx, jsGlobal, jsFunc, jsVector, &dummyRetVal );
}

HostTimer::HostTimer( HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated, std::shared_ptr<HostTimerTask> task )
    : task_( task )
    , hWnd_( hWnd )
    , id_( id )
    , delay_( delay )
    , isRepeated_( isRepeated )
{
    assert( task );
}

void HostTimer::start( HANDLE hTimerQueue )
{
    BOOL bRet = CreateTimerQueueTimer(
        &hTimer_,
        hTimerQueue,
        HostTimer::timerProc,
        this,
        delay_,
        ( isRepeated_ ? delay_ : 0 ),
        WT_EXECUTEINTIMERTHREAD | ( isRepeated_ ? 0 : WT_EXECUTEONLYONCE ) );
    qwr::error::CheckWinApi( bRet, "CreateTimerQueueTimer" );
}

void HostTimer::stop()
{
    isStopRequested_ = true;
}

VOID CALLBACK HostTimer::timerProc( PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/ )
{
    auto* timer = static_cast<HostTimer*>( lpParameter );

    if ( timer->isStopped_ )
    {
        return;
    }

    if ( timer->isStopRequested_ )
    {
        timer->isStopped_ = true;
        HostTimerDispatcher::Get().onTimerStopRequest( timer->hWnd_, timer->hTimer_, timer->id_ );

        return;
    }

    auto postTimerTask = [&timer] {
        SendMessage(
            timer->hWnd_,
            static_cast<int>( smp::InternalSyncMessage::timer_proc ),
            0,
            (LPARAM)( timer->task_.get() ) );
    };

    if ( timer->isRepeated_ )
    {
        postTimerTask();
    }
    else
    {
        timer->isStopped_ = true;
        postTimerTask();
        HostTimerDispatcher::Get().onTimerStopRequest( timer->hWnd_, timer->hTimer_, timer->id_ );
    }
}

HWND HostTimer::GetHwnd() const
{
    return hWnd_;
}

} // namespace smp
