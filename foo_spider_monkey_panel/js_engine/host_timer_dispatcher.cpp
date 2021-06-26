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
    : hTimerQueue_( CreateTimerQueue() )
{
}

HostTimerDispatcher::~HostTimerDispatcher()
{
    if ( hTimerQueue_ )
    {
        (void)DeleteTimerQueueEx( hTimerQueue_, INVALID_HANDLE_VALUE );
    }
}

HostTimerDispatcher& HostTimerDispatcher::Get()
{
    static HostTimerDispatcher timerDispatcher;
    return timerDispatcher;
}

void HostTimerDispatcher::Finalize()
{
    if ( hTimerQueue_ )
    {
        (void)DeleteTimerQueueEx( hTimerQueue_, INVALID_HANDLE_VALUE );
    }
}

uint32_t HostTimerDispatcher::SetInterval( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs )
{
    return CreateTimer( hWnd, delay, true, cx, jsFunction, jsFuncArgs );
}

uint32_t HostTimerDispatcher::SetTimeout( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs )
{
    return CreateTimer( hWnd, delay, false, cx, jsFunction, jsFuncArgs );
}

void HostTimerDispatcher::StopTimer( uint32_t timerId )
{
    std::lock_guard<std::mutex> lock( timerMutex_ );

    auto it = timerMap_.find( timerId );
    if ( timerMap_.end() != it )
    {
        it->second->Stop();
    }
}

void HostTimerDispatcher::StopTimersForPanel( HWND hWnd )
{
    std::lock_guard<std::mutex> lock( timerMutex_ );

    for ( const auto& [timerId, timer]: timerMap_ )
    {
        if ( timer->GetHwnd() == hWnd )
        {
            timer->Stop();
        }
    }
}

void HostTimerDispatcher::EraseTimer( uint32_t timerId )
{
    std::lock_guard<std::mutex> lock( timerMutex_ );

    timerMap_.erase( timerId );
}

uint32_t HostTimerDispatcher::CreateTimer( HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs )
{
    assert( jsFunction );

    std::lock_guard<std::mutex> lock( timerMutex_ );

    const uint32_t id = [&] {
        uint32_t id = curTimerId_++;
        while ( timerMap_.contains( id ) )
        {
            id = curTimerId_++;
        }
        return id;
    }();

    JS::RootedValue jsFuncValue( cx, JS::ObjectValue( *JS_GetFunctionObject( jsFunction ) ) );
    JS::RootedObject jsArrayObject( cx, JS_NewArrayObject( cx, jsFuncArgs ) );
    smp::JsException::ExpectTrue( jsArrayObject );
    JS::RootedValue jsArrayValue( cx, JS::ObjectValue( *jsArrayObject ) );

    auto [it, bDummy] = timerMap_.emplace( id, std::make_shared<HostTimer>( hWnd, id, delay, isRepeated, std::make_unique<HostTimerTask>( cx, jsFuncValue, jsArrayValue ) ) );
    auto& [iDummy, timer] = *it;
    auto autoTimer = qwr::final_action( [&] { timerMap_.erase( id ); } );

    timer->Start( hTimerQueue_ );

    autoTimer.cancel();
    return id;
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

HostTimer::~HostTimer()
{
    if ( hTimerQueue_ && hTimer_ )
    {
        (void)DeleteTimerQueueTimer( hTimerQueue_, hTimer_, nullptr );
    }
}

std::shared_ptr<HostTimer> HostTimer::GetSelfSaver()
{
    return shared_from_this();
}

void HostTimer::Start( HANDLE hTimerQueue )
{
    assert( hTimerQueue );
    assert( !isStopRequested_ );

    hTimerQueue_ = hTimerQueue;
    BOOL bRet = CreateTimerQueueTimer(
        &hTimer_,
        hTimerQueue_,
        HostTimer::TimerProc,
        this,
        delay_,
        ( isRepeated_ ? delay_ : 0 ),
        WT_EXECUTEINTIMERTHREAD | ( isRepeated_ ? 0 : WT_EXECUTEONLYONCE ) );
    qwr::error::CheckWinApi( bRet, "CreateTimerQueueTimer" );
}

void HostTimer::Stop()
{
    isStopRequested_ = true;
}

VOID CALLBACK HostTimer::TimerProc( PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/ )
{
    auto* timer = static_cast<HostTimer*>( lpParameter );
    const auto selfSaver = timer->GetSelfSaver();

    if ( timer->isStopped_ )
    {
        return;
    }

    if ( timer->isStopRequested_ )
    {
        timer->isStopped_ = true;
        HostTimerDispatcher::Get().EraseTimer( timer->id_ );

        return;
    }

    auto execTimerTask = [&timer] {
        SendMessage(
            timer->hWnd_,
            static_cast<int>( smp::InternalSyncMessage::timer_proc ),
            0,
            ( LPARAM )( timer->task_.get() ) );
    };

    if ( timer->isRepeated_ )
    {
        execTimerTask();
    }
    else
    {
        timer->isStopped_ = true;
        execTimerTask();
        HostTimerDispatcher::Get().EraseTimer( timer->id_ );
    }
}

HWND HostTimer::GetHwnd() const
{
    return hWnd_;
}

} // namespace smp
