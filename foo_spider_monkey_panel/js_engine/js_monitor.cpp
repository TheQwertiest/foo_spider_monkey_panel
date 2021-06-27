// A lot of logic ripped from <js/xpconnect/src/XPCJSContext.cpp>

#include <stdafx.h>

#include "js_monitor.h"

#include <fb2k/advanced_config.h>
#include <js_engine/js_container.h>
#include <js_engine/js_engine.h>
#include <panel/js_panel_window.h>
#include <panel/modal_blocking_scope.h>
#include <ui/ui_slow_script.h>

#include <qwr/delayed_executor.h>
#include <qwr/final_action.h>
#include <qwr/thread_helpers.h>

using namespace smp;

namespace
{

constexpr auto kMonitorRate = std::chrono::seconds( 1 );

auto GetLowResTime()
{
    return std::chrono::milliseconds( GetTickCount64() );
}

} // namespace

namespace mozjs
{

JsMonitor::JsMonitor()
    : slowScriptLimit_( smp::config::advanced::performance_max_runtime.GetValue() )
{ // JsMonitor might be created before fb2k is fully initialized
    qwr::DelayedExecutor::GetInstance().AddTask( [&hFb2k = hFb2k_] { hFb2k = core_api::get_main_window(); } );
}

void JsMonitor::Start( JSContext* cx )
{
    pJsCtx_ = cx;
    if ( slowScriptLimit_ != std::chrono::seconds::zero() )
    {
        StartMonitorThread();
    }
}

void JsMonitor::Stop()
{
    if ( slowScriptLimit_ != std::chrono::seconds::zero() )
    {
        StopMonitorThread();
    }
    pJsCtx_ = nullptr;
}

void JsMonitor::AddContainer( JsContainer& jsContainer )
{
    assert( !monitoredContainers_.contains( &jsContainer ) );
    monitoredContainers_.emplace( &jsContainer, &jsContainer );
}

void JsMonitor::RemoveContainer( JsContainer& jsContainer )
{
    assert( monitoredContainers_.contains( &jsContainer ) );
    monitoredContainers_.erase( &jsContainer );
}

void JsMonitor::OnJsActionStart( JsContainer& jsContainer )
{
    auto it = monitoredContainers_.find( &jsContainer );
    assert( it != monitoredContainers_.cend() );

    auto& [key, data] = *it;
    if ( data.ignoreSlowScriptCheck )
    {
        return;
    }
    const auto curTime = GetLowResTime();
    data.slowScriptCheckpoint = curTime;

    {
        std::unique_lock<std::mutex> ul( watcherDataMutex_ );
        activeContainers_.emplace( &jsContainer, curTime );
        hasAction_.notify_one();
    }
}

void JsMonitor::OnJsActionEnd( JsContainer& jsContainer )
{
    auto it = monitoredContainers_.find( &jsContainer );
    assert( it != monitoredContainers_.cend() );

    it->second.slowScriptSecondHalf = false;

    {
        std::unique_lock<std::mutex> ul( watcherDataMutex_ );
        if ( const auto itActive = activeContainers_.find( &jsContainer );
             itActive != activeContainers_.cend() )
        {
            // container might or might not be in `activeContainers_` depending on if and when it's `ignoreSlowScriptCheck` was set
            activeContainers_.erase( itActive );
        }
    }
}

bool JsMonitor::OnInterrupt()
{
    if ( !pJsCtx_ ///< might be invoked before monitor was started
         || slowScriptLimit_ == std::chrono::seconds::zero() )
    {
        return true;
    }

    {
        std::unique_lock<std::mutex> lock( watcherDataMutex_ );
        if ( isInInterrupt_ )
        {
            return true;
        }
        isInInterrupt_ = true;
    }
    qwr::final_action autoBool( [&] {
        std::unique_lock<std::mutex> lock( watcherDataMutex_ );
        isInInterrupt_ = false;
    } );

    const auto curTime = GetLowResTime();

    { // Action might've been blocked by modal window
        const bool isInModal = HasActivePopup( true );

        if ( wasInModal_ && !isInModal )
        {
            for ( auto& [pContainer, containerData]: monitoredContainers_ )
            {
                containerData.slowScriptCheckpoint = curTime;
            }
            {
                std::unique_lock<std::mutex> lock( watcherDataMutex_ );
                for ( auto& [pContainer, startTime]: activeContainers_ )
                {
                    startTime = curTime;
                }
            }
        }
        wasInModal_ = isInModal;

        if ( isInModal )
        {
            return true;
        }
    }

    auto containerDataToProcess = [&]() {
        std::unique_lock<std::mutex> lock( watcherDataMutex_ );
        std::vector<std::pair<JsContainer*, ContainerData*>> dataToProcess;
        for ( auto& [pContainer, containerData]: monitoredContainers_ )
        {
            const auto it = ranges::find_if( activeContainers_, [pContainer = pContainer]( auto& elem ) {
                return ( elem.first == pContainer );
            } );
            if ( activeContainers_.cend() != it )
            {
                dataToProcess.emplace_back( pContainer, &containerData );
            }
        }
        return dataToProcess;
    }();
    for ( auto [pContainer, pContainerData]: containerDataToProcess )
    {
        auto& containerData = *pContainerData;

        if ( containerData.ignoreSlowScriptCheck
             || ( curTime - containerData.slowScriptCheckpoint ) < slowScriptLimit_ / 2.0 )
        {
            continue;
        }

        // In order to guard against time changes or laptops going to sleep, we
        // don't trigger the slow script warning until (limit/2) seconds have
        // elapsed twice.
        if ( !containerData.slowScriptSecondHalf )
        { // use current time, since we might wait on warning dialog
            containerData.slowScriptCheckpoint = GetLowResTime();
            containerData.slowScriptSecondHalf = true;
            continue;
        }

        if ( JsContainer::JsStatus::EngineFailed == pContainer->GetStatus()
             || JsContainer::JsStatus::Failed == pContainer->GetStatus() )
        { // possible if the interrupt was requested again after the script was aborted,
            // but before the container was removed from active
            continue;
        }

        smp::ui::CDialogSlowScript::Data dlgData;
        {
            qwr::u8string panelName;
            HWND parentHwnd;
            switch ( pContainer->GetStatus() )
            {
            case JsContainer::JsStatus::Working:
            {
                auto& parentPanel = pContainer->GetParentPanel();
                panelName = parentPanel.GetPanelDescription( false );
                parentHwnd = parentPanel.GetHWND();
                break;
            }
            case JsContainer::JsStatus::Ready:
            { // possible if script destroyed the parent panel (e.g. by switching layout)
                parentHwnd = GetActiveWindow();
                break;
            }
            default:
                assert( false );
                parentHwnd = GetActiveWindow();
            }

            qwr::u8string scriptInfo;
            JS::AutoFilename filename;
            unsigned lineno;
            if ( !JS::DescribeScriptedCaller( pJsCtx_, &filename, &lineno ) )
            {
                JS_ClearPendingException( pJsCtx_ );
                scriptInfo = "<failed to fetch script info>";
            }
            else
            {
                if ( filename.get() )
                {
                    if ( strlen( filename.get() ) )
                    {
                        scriptInfo += filename.get();
                    }
                    else
                    {
                        scriptInfo += "<unknown file>";
                    }
                    scriptInfo += ": " + std::to_string( lineno );
                }
            }

            smp::ui::CDialogSlowScript dlg( panelName, scriptInfo, dlgData );
            // TODO: fix dialog centering (that is lack of thereof)
            (void)dlg.DoModal( parentHwnd );
        }

        containerData.ignoreSlowScriptCheck = !dlgData.askAgain;

        if ( dlgData.stop )
        { // TODO: this might stop the script different from the one in currently iterated container,
            // we should get the container corresponding to the currently active realm.
            // Example: panel_1(reported): window.NotifyOthers > panel_2(stopped): on_notify_data
            JS_ReportErrorUTF8( pJsCtx_, "Script aborted by user" );
            return false;
        }

        containerData.slowScriptCheckpoint = GetLowResTime();
        containerData.slowScriptSecondHalf = false;
    }

    return true;
}

void JsMonitor::StartMonitorThread()
{
    shouldStopThread_ = false;
    watcherThread_ = std::thread( [&] {
        while ( !shouldStopThread_ )
        {
            // We want to avoid showing the slow script dialog if the user's laptop
            // goes to sleep in the middle of running a script. To ensure this, we
            // invoke the interrupt callback after only half the timeout has
            // elapsed. The callback simply records the fact that it was called in
            // the mSlowScriptSecondHalf flag. Then we wait another (timeout/2)
            // seconds and invoke the callback again. This time around it sees
            // mSlowScriptSecondHalf is set and so it shows the slow script
            // dialog. If the computer is put to sleep during one of the (timeout/2)
            // periods, the script still has the other (timeout/2) seconds to
            // finish.

            std::this_thread::sleep_for( kMonitorRate );
            bool hasPotentiallySlowScripts = false;
            {
                std::unique_lock<std::mutex> lock( watcherDataMutex_ );

                if ( activeContainers_.empty() )
                {
                    hasAction_.wait( lock, [&] { return ( shouldStopThread_ || ( !activeContainers_.empty() && !isInInterrupt_ ) ); } );
                }
                else if ( isInInterrupt_ )
                { // Can't interrupt
                    continue;
                }

                if ( shouldStopThread_ )
                {
                    break;
                }

                hasPotentiallySlowScripts = [&] {
                    if ( HasActivePopup( false ) )
                    { // popup detected, delay monitoring
                        wasInModal_ = true;
                        return false;
                    }

                    const auto curTime = GetLowResTime();

                    const auto it = ranges::find_if( activeContainers_, [&curTime, &slowScriptLimit = slowScriptLimit_]( auto& elem ) {
                        auto& [pContainer, startTime] = elem;
                        return ( ( curTime - startTime ) > slowScriptLimit / 2.0 );
                    } );

                    return ( it != activeContainers_.cend() );
                }();
            }

            if ( hasPotentiallySlowScripts )
            {
                JS_RequestInterruptCallback( pJsCtx_ );
            }
        }
    } );
    qwr::SetThreadName( watcherThread_, "SMP Watcher" );
}

void JsMonitor::StopMonitorThread()
{
    {
        std::unique_lock<std::mutex> lock( watcherDataMutex_ );
        shouldStopThread_ = true;
        hasAction_.notify_one();
    }

    if ( watcherThread_.joinable() )
    {
        watcherThread_.join();
    }
}

bool JsMonitor::HasActivePopup( bool isMainThread ) const
{
    if ( modal::IsInWhitelistedModal() )
    {
        return false;
    }

    if ( modal::IsModalBlocked() )
    {
        return true;
    }

    if ( hFb2k_ && GetLastActivePopup( hFb2k_ ) != hFb2k_ )
    {
        return true;
    }

    return false;
}

} // namespace mozjs
