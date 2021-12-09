#include <stdafx.h>

#include "js_panel_window.h"

#include <com_objects/track_drop_target.h>
#include <com_utils/com_destruction_handler.h>
#include <config/delayed_package_utils.h>
#include <config/package_utils.h>
#include <events/event_basic.h>
#include <events/event_dispatcher.h>
#include <events/event_drag.h>
#include <events/event_js_callback.h>
#include <events/event_mouse.h>
#include <fb2k/mainmenu_dynamic.h>
#include <js_engine/js_container.h>
#include <panel/drag_action_params.h>
#include <panel/edit_script.h>
#include <panel/modal_blocking_scope.h>
#include <timeout/timeout_manager.h>
#include <ui/ui_conf.h>
#include <utils/art_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/image_helpers.h>
#include <utils/logging.h>

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

namespace
{

DWORD ConvertEdgeStyleToNativeFlags( smp::config::EdgeStyle edge_style )
{
    switch ( edge_style )
    {
    case smp::config::EdgeStyle::SunkenEdge:
        return WS_EX_CLIENTEDGE;
    case smp::config::EdgeStyle::GreyEdge:
        return WS_EX_STATICEDGE;
    default:
        return 0;
    }
}

} // namespace

namespace mozjs
{
class JsAsyncTask;
}

namespace smp::panel
{

js_panel_window::js_panel_window( PanelType instanceType )
    : panelType_( instanceType )
{
}

js_panel_window::~js_panel_window()
{
}

ui_helpers::container_window::class_data& js_panel_window::get_class_data() const
{
    static class_data my_class_data =
    {
        TEXT( SMP_WINDOW_CLASS_NAME ),                              // class name
        L"",                                                        // window title
        0,                                                          // refcount
        false,                                                      // class reg-d
        true,                                                       // want trasparent bg
        0,                                                          // extra wnd bytes
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,               // styles
        ConvertEdgeStyleToNativeFlags( settings_.edgeStyle ),       // extra syles
        CS_DBLCLKS,                                                 // class styles
        true,                                                       // fwd settings chg
        true,                                                       // fwd colors chg
        true,                                                       // fwd time chg
        IDC_ARROW                                                   // def cursor
    };

    return my_class_data;
}

void js_panel_window::Fail( const qwr::u8string& errorText )
{
    hasFailed_ = true;

    smp::config::advanced::js_suppress_error_popup.GetValue()
        ? qwr::ReportError( SMP_UNDERSCORE_NAME, errorText )
        : qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorText );

    if ( wnd_ )     // can be null during startup
        Repaint();  ///< repaint to display error message

    UnloadScript( true );
}

const config::ParsedPanelSettings& js_panel_window::GetSettings() const
{
    return settings_;
}

config::PanelProperties& js_panel_window::GetPanelProperties()
{
    return properties_;
}

TimeoutManager& js_panel_window::GetTimeoutManager()
{
    assert( pTimeoutManager_ );
    return *pTimeoutManager_;
}

void js_panel_window::ReloadScript()
{
    if ( pJsContainer_ )
    {
        // Panel might be not loaded at all, if settings are changed from Preferences.
        UnloadScript();

        if ( !ReloadSettings() )
            return;

        LoadScript( false );
    }
}

void js_panel_window::LoadSettings( stream_reader& reader, t_size size, abort_callback& abort, bool reloadPanel )
{
    const smp::config::PanelSettings settings = [&]
    {
        try
        {
            return config::PanelSettings::Load( reader, size, abort );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup
            (
                SMP_UNDERSCORE_NAME,
                fmt::format
                (
                    "Can't load panel settings. Your panel will be completely reset!\nError: {}",
                    e.what()
                )
            );

            return config::PanelSettings{};
        }
    }();

    if ( !UpdateSettings( settings, reloadPanel ) )
    {
        qwr::ReportErrorWithPopup
        (
            SMP_UNDERSCORE_NAME,
            "Can't load panel settings. Your panel will be completely reset!"
        );

        UpdateSettings( config::PanelSettings{}, reloadPanel );
    }
}

void js_panel_window::SetSettings( const smp::config::ParsedPanelSettings& settings )
{
    settings_ = settings;
    ReloadScript();
}

bool js_panel_window::UpdateSettings( const smp::config::PanelSettings& settings, bool reloadPanel )
{
    try
    {
        settings_ = config::ParsedPanelSettings::Parse( settings );
    }
    catch ( const qwr::QwrException& e )
    {
        Fail( e.what() );
        return false;
    }

    properties_ = settings.properties;

    if ( reloadPanel )
        ReloadScript();

    return true;
}

bool js_panel_window::SaveSettings( stream_writer& writer, abort_callback& abort ) const
{
    try
    {
        auto settings = settings_.GeneratePanelSettings();
        settings.properties = properties_;
        settings.Save( writer, abort );
        return true;
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        return false;
    }
}

bool js_panel_window::ReloadSettings()
{
    try
    {
        settings_ = config::ParsedPanelSettings::Reparse( settings_ );
        return true;
    }
    catch ( const qwr::QwrException& e )
    {
        Fail( e.what() );
        return false;
    }
}

bool js_panel_window::IsPanelIdOverridenByScript() const
{
    return isPanelIdOverridenByScript_;
}

LRESULT js_panel_window::on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    // According to MSDN:
    //
    // If no filter is specified, messages are processed in the following order:
    // - Sent messages
    // - Posted messages
    // - Input (hardware) messages and system internal events
    // - Sent messages (again)
    // - WM_PAINT messages
    // - WM_TIMER messages
    //
    // Since we are constantly processing our own `event` messages, we need to take additional care
    // so as not to stall WM_PAINT and WM_TIMER messages.

    static uint32_t msgNestedCounter = 0;

    ++msgNestedCounter;

    const qwr::final_action autoComObjectDeleter( [&]
    {
        // delete only on exit as to avoid delaying processing of the current message due to reentrancy
        if ( --msgNestedCounter == 0)
            com::DeleteMarkedObjects();
    } );

    if ( EventDispatcher::IsRequestEventMessage( msg ) )
    {
        EventDispatcher::Get().OnRequestEventMessageReceived( wnd_ );

        if ( auto retVal = ProcessEvent();
             retVal.has_value() )
        {
            return *retVal;
        }
    }
    else
    {
        if ( auto retVal = ProcessSyncMessage( MSG{ hwnd, msg, wp, lp } );
             retVal.has_value() )
        {
            return *retVal;
        }
    }

    return DefWindowProc( hwnd, msg, wp, lp );
}

void js_panel_window::ExecuteEvent_Basic( EventId id )
{
    switch ( id )
    {
    case EventId::kScriptEdit:
    {
        EditScript();
        break;
    }
    case EventId::kScriptReload:
    {
        ReloadScript();
        break;
    }
    case EventId::kScriptShowConfigure:
    {
        ShowConfigure( wnd_ );
        break;
    }
    case EventId::kScriptShowConfigureLegacy:
    {
        switch ( settings_.GetSourceType() )
        {
        case config::ScriptSourceType::InMemory:
        {
            EditScript();
            break;
        }
        default:
        {
            ShowConfigure( wnd_ );
            break;
        }
        }
        break;
    }
    case EventId::kScriptShowProperties:
    {
        ShowConfigure( wnd_, ui::CDialogConf::Tab::properties );
        break;
    }
    case EventId::kWndPaint:
    {
        wnd_.GetClientRect( &clientRect );

        if ( isPaintInProgress_ )
            break;

        // skip redraw if updaterect is empty
        CRect updateRect;
        wnd_.GetUpdateRect( &updateRect, FALSE );

        if ( updateRect.IsRectEmpty() )
            break;

        isPaintInProgress_ = true;

        if ( settings_.isPseudoTransparent && isBgRepaintNeeded_ )
        {
            // Two pass redraw: paint BG > Repaint() > paint FG
            RepaintBackground( &updateRect ); ///< Calls Repaint() inside

            isBgRepaintNeeded_ = false;
            isPaintInProgress_ = false;

            Repaint( true );
            break;
        }

        [&]
        {
            CPaintDC dc{ wnd_ };
            if ( dc.RectVisible( updateRect ) )
                OnPaint( dc, dc.m_ps.rcPaint );
        }();

        isPaintInProgress_ = false;
        break;
    }
    case EventId::kWndRepaintBackground:
    {
        wnd_.GetClientRect( &clientRect );
        isBgRepaintNeeded_ = true;
        Repaint( true );
        break;
    }
    case EventId::kWndResize:
    {
        wnd_.GetClientRect( &clientRect );

        if ( !pJsContainer_ )
            break;

        OnSizeUser();
        break;
    }
    default:
        break;
    }
}

void js_panel_window::ExecuteEvent_JsTask( EventId id, Event_JsExecutor& task )
{
    const auto execJs = [&]( auto& jsTask ) -> std::optional<bool>
    {
        if ( !pJsContainer_ )
        {
            return false;
        }
        else
        {
            return jsTask.JsExecute( *pJsContainer_ );
        }
    };

    switch ( id )
    {
    case EventId::kMouseRightButtonUp:
    {
        const auto pEvent = task.AsMouseEvent();
        assert( pEvent );

        // Bypass the user code.
        const auto useDefaultContextMenu = [&] {
            if ( pEvent->IsShiftPressed() && pEvent->IsWinPressed() )
            {
                return true;
            }
            else
            {
                return !execJs( *pEvent ).value_or( false );
            }
        }();

        if ( useDefaultContextMenu )
        {
            EventDispatcher::Get().PutEvent( wnd_,
                                             std::make_unique<Event_Mouse>(
                                                 EventId::kMouseContextMenu,
                                                 pEvent->GetX(),
                                                 pEvent->GetY(),
                                                 0,
                                                 pEvent->GetModifiers() ),
                                             EventPriority::kInput );
        }

        break;
    }
    case EventId::kMouseContextMenu:
    {
        const auto pMouseEvent = task.AsMouseEvent();
        assert( pMouseEvent );

        OnContextMenu( pMouseEvent->GetX(), pMouseEvent->GetY() );

        break;
    }
    case EventId::kMouseDragEnter:
    {
        const auto pDragEvent = task.AsDragEvent();
        assert( pDragEvent );

        lastDragParams_.reset();

        if ( pJsContainer_ )
        {
            smp::panel::DragActionParams dragParams = pDragEvent->GetDragParams();
            const bool bRet = pJsContainer_->InvokeOnDragAction( "on_drag_enter",
                                                                 { pDragEvent->GetX(), pDragEvent->GetY() },
                                                                 pDragEvent->GetMask(),
                                                                 dragParams );
            if ( bRet )
            {
                lastDragParams_ = dragParams;
            }
        }

        pDragEvent->DisposeStoredData();

        break;
    }
    case EventId::kMouseDragLeave:
    {
        const auto pDragEvent = task.AsDragEvent();
        assert( pDragEvent );

        lastDragParams_.reset();
        pDragEvent->DisposeStoredData();

        if ( pJsContainer_ )
        {
            pJsContainer_->InvokeJsCallback( "on_drag_leave" );
        }

        break;
    }
    case EventId::kMouseDragOver:
    {
        const auto pDragEvent = task.AsDragEvent();
        assert( pDragEvent );

        if ( pJsContainer_ )
        {
            smp::panel::DragActionParams dragParams = pDragEvent->GetDragParams();
            const bool bRet = pJsContainer_->InvokeOnDragAction( "on_drag_over",
                                                                 { pDragEvent->GetX(), pDragEvent->GetY() },
                                                                 pDragEvent->GetMask(),
                                                                 dragParams );
            if ( bRet )
            {
                lastDragParams_ = dragParams;
            }
        }

        pDragEvent->DisposeStoredData();

        break;
    }
    case EventId::kMouseDragDrop:
    {
        const auto pDragEvent = task.AsDragEvent();
        assert( pDragEvent );

        if ( pJsContainer_ )
        {
            smp::panel::DragActionParams dragParams = pDragEvent->GetDragParams();
            const bool bRet = pJsContainer_->InvokeOnDragAction( "on_drag_drop",
                                                                 { pDragEvent->GetX(), pDragEvent->GetY() },
                                                                 pDragEvent->GetMask(),
                                                                 dragParams );
            if ( bRet )
            {
                smp::com::TrackDropTarget::ProcessDropEvent( pDragEvent->GetStoredData(), dragParams );
            }
        }

        lastDragParams_.reset();
        pDragEvent->DisposeStoredData();

        break;
    }
    case EventId::kInputFocus:
    {
        selectionHolder_ = ui_selection_manager::get()->acquire();
        // Note: selection holder is released in WM_KILLFOCUS processing

        execJs( task );
        break;
    }
    default:
    {
        execJs( task );
    }
    }
}

bool js_panel_window::ExecuteEvent_JsCode( mozjs::JsAsyncTask& jsTask )
{
    if ( !pJsContainer_ )
    {
        return false;
    }

    return pJsContainer_->InvokeJsAsyncTask( jsTask );
}

void js_panel_window::OnProcessingEventStart()
{
    ++eventNestedCounter_;
}

void js_panel_window::OnProcessingEventFinish()
{
    --eventNestedCounter_;

    // Jobs (e.g. futures) should be drained only with empty JS stack and after the current task (as required by ES).
    // Also see https://developer.mozilla.org/en-US/docs/Web/JavaScript/EventLoop#Run-to-completion
    if ( !eventNestedCounter_ )
    {
        mozjs::JsContainer::RunJobs();
    }

    if ( !eventNestedCounter_ || modal::IsModalBlocked() )
    {
        EventDispatcher::Get().RequestNextEvent( wnd_ );
    }
}

std::optional<LRESULT> js_panel_window::ProcessEvent()
{
    OnProcessingEventStart();

    qwr::final_action onEventProcessed( [&] { OnProcessingEventFinish(); } );

    if ( const auto stalledMsgOpt = GetStalledMessage(); stalledMsgOpt )
    { // stalled messages always have a higher priority
        if ( auto retVal = ProcessStalledMessage( *stalledMsgOpt );
             retVal.has_value() )
        {
            return *retVal;
        }

        return DefWindowProc( wnd_, stalledMsgOpt->message, stalledMsgOpt->wParam, stalledMsgOpt->lParam );
    }
    else
    {
        if ( eventNestedCounter_ == 1 || modal::IsModalBlocked() )
        {
            EventDispatcher::Get().ProcessNextEvent( wnd_ );
        }

        return std::nullopt;
    }
}

void js_panel_window::ProcessEventManually( Runnable& runnable )
{
    OnProcessingEventStart();

    qwr::final_action onEventProcessed( [&] { OnProcessingEventFinish(); } );

    runnable.Run();
}

std::optional<MSG> js_panel_window::GetStalledMessage()
{
    MSG msg;
    bool hasMessage = PeekMessage( &msg, wnd_, WM_TIMER, WM_TIMER, PM_REMOVE );
    if ( !hasMessage )
    {
        return std::nullopt;
    }

    // means that WM_PAINT was invoked properly
    if ( !hRepaintTimer_ )
        return std::nullopt;

    KillTimer( wnd_, hRepaintTimer_ );
    hRepaintTimer_ = NULL;

    return msg;
}

std::optional<LRESULT> js_panel_window::ProcessStalledMessage( const MSG& msg )
{
    switch ( msg.message )
    {
    case WM_TIMER:
    {
        wnd_.RedrawWindow( nullptr, nullptr, RDW_UPDATENOW );
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::ProcessSyncMessage( const MSG& msg )
{
    if ( auto retVal = ProcessCreationMessage( msg );
         retVal.has_value() )
    {
        return *retVal;
    }

    if ( auto retVal = ProcessWindowMessage( msg );
         retVal.has_value() )
    {
        return *retVal;
    }

    if ( IsInEnumRange<InternalSyncMessage>( msg.message ) )
    {
        if ( auto retVal = ProcessInternalSyncMessage( static_cast<InternalSyncMessage>( msg.message ), msg.wParam, msg.lParam );
             retVal.has_value() )
        {
            return *retVal;
        }
    }

    return std::nullopt;
}

std::optional<LRESULT> js_panel_window::ProcessCreationMessage( const MSG& msg )
{
    switch ( msg.message )
    {
    case WM_CREATE:
    {
        OnCreate( msg.hwnd );
        return 0;
    }
    case WM_DESTROY:
    {
        OnDestroy();
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::ProcessWindowMessage( const MSG& msg )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( msg.message )
    {
    case WM_DISPLAYCHANGE:
    case WM_THEMECHANGED:
    {
        EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kScriptReload ), EventPriority::kControl );
        return 0;
    }
    case WM_ERASEBKGND:
    {
        if ( settings_.isPseudoTransparent )
        {
            auto pEvent = std::make_unique<Event_Basic>( EventId::kWndRepaintBackground );
            pEvent->SetTarget( pTarget_ );
            ProcessEventManually( *pEvent );
        }

        return 1;
    }
    case WM_PAINT:
    {
        wnd_.GetClientRect( &clientRect );

        if ( hRepaintTimer_ )
        {
            KillTimer( wnd_, hRepaintTimer_ );
            hRepaintTimer_ = NULL;
        }

        if ( isPaintInProgress_ )
        {
            return std::nullopt;
        }

        auto pEvent = std::make_unique<Event_Basic>( EventId::kWndPaint );
        pEvent->SetTarget( pTarget_ );
        ProcessEventManually( *pEvent );

        return 0;
    }
    case WM_SIZE:
    {
        wnd_.GetClientRect( &clientRect );
        OnSizeDefault();

        auto pEvent = std::make_unique<Event_Basic>( EventId::kWndResize );
        pEvent->SetTarget( pTarget_ );
        ProcessEventManually( *pEvent );

        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        // This message will be called before WM_CREATE as well,
        // but we don't need to handle it before panel creation,
        // since default values suit us just fine
        auto pmmi = reinterpret_cast<LPMINMAXINFO>( msg.lParam );
        pmmi->ptMaxTrackSize = MaxSize();
        pmmi->ptMinTrackSize = MinSize();
        return 0;
    }
    case WM_GETDLGCODE:
    {
        return DlgCode();
    }
    case WM_LBUTTONDOWN:
    {
        static const std::unordered_map<int, EventId> kMsgToEventId{
            { WM_LBUTTONDOWN, EventId::kMouseLeftButtonDown },
            { WM_MBUTTONDOWN, EventId::kMouseMiddleButtonDown },
            { WM_RBUTTONDOWN, EventId::kMouseRightButtonDown }
        };

        if ( settings_.shouldGrabFocus )
        {
            wnd_.SetFocus();
        }

        SetCaptureMouseState( true );

        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             kMsgToEventId.at( msg.message ),
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    {
        static const std::unordered_map<int, EventId> kMsgToEventId{
            { WM_LBUTTONUP, EventId::kMouseLeftButtonUp },
            { WM_MBUTTONUP, EventId::kMouseMiddleButtonUp }
        };

        if ( isMouseCaptured_ )
        {
            SetCaptureMouseState( false );
        }

        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             kMsgToEventId.at( msg.message ),
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_RBUTTONUP:
    {
        if ( isMouseCaptured_ )
        {
            SetCaptureMouseState( false );
        }

        EventDispatcher::Get().PutEvent( wnd_,
                                         std::make_unique<Event_Mouse>(
                                             EventId::kMouseRightButtonUp,
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ),
                                             GetHotkeyModifierFlags() ),
                                         EventPriority::kInput );

        return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
        // check if the panel is currently crashed and try to reload instead
        if ( HasFailed() )
        {
            ReloadScript();
            return std::nullopt;
        }

        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kMouseLeftButtonDoubleClick,
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_MBUTTONDBLCLK:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kMouseMiddleButtonDoubleClick,
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_RBUTTONDBLCLK:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kMouseRightButtonDoubleClick,
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_CONTEXTMENU:
    {
        // WM_CONTEXTMENU receives screen coordinates
        POINT p{ GET_X_LPARAM( msg.wParam ), GET_Y_LPARAM( msg.lParam ) };
        ScreenToClient( wnd_, &p );
        EventDispatcher::Get().PutEvent( wnd_,
                                         std::make_unique<Event_Mouse>(
                                             EventId::kMouseContextMenu,
                                             p.x,
                                             p.y,
                                             0,
                                             GetHotkeyModifierFlags() ),
                                         EventPriority::kInput );

        return 1;
    }
    case WM_MOUSEMOVE:
    {
        if ( !isMouseTracked_ )
        {
            isMouseTracked_ = true;

            TRACKMOUSEEVENT tme{ sizeof( TRACKMOUSEEVENT ), TME_LEAVE, wnd_, HOVER_DEFAULT };
            TrackMouseEvent( &tme );

            // Restore default cursor
            SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
        }

        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kMouseMove,
                                             static_cast<int32_t>( GET_X_LPARAM( msg.lParam ) ),
                                             static_cast<int32_t>( GET_Y_LPARAM( msg.lParam ) ),
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_MOUSELEAVE:
    {
        isMouseTracked_ = false;

        // Restore default cursor
        SetCursor( LoadCursor( nullptr, IDC_ARROW ) );

        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback( EventId::kMouseLeave ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_MOUSEWHEEL:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kMouseVerticalWheel,
                                             static_cast<int8_t>( GET_WHEEL_DELTA_WPARAM( msg.wParam ) > 0 ? 1 : -1 ),
                                             static_cast<int32_t>( GET_WHEEL_DELTA_WPARAM( msg.wParam ) ),
                                             static_cast<int32_t>( WHEEL_DELTA ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_MOUSEHWHEEL:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kMouseHorizontalWheel,
                                             static_cast<int8_t>( GET_WHEEL_DELTA_WPARAM( msg.wParam ) > 0 ? 1 : -1 ) ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_SETCURSOR:
    {
        return 1;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kKeyboardKeyDown,
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return 0;
    }
    case WM_KEYUP:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kKeyboardKeyUp,
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return 0;
    }
    case WM_CHAR:
    {
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kKeyboardChar,
                                             static_cast<uint32_t>( msg.wParam ) ),
                                         EventPriority::kInput );
        return 0;
    }
    case WM_SETFOCUS:
    {
        // Note: selection holder is acquired during event processing
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kInputFocus,
                                             true ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_KILLFOCUS:
    {
        selectionHolder_.release();
        EventDispatcher::Get().PutEvent( wnd_,
                                         GenerateEvent_JsCallback(
                                             EventId::kInputBlur,
                                             false ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::ProcessInternalSyncMessage( InternalSyncMessage msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( msg )
    {
    case InternalSyncMessage::legacy_notify_others:
    {
        // this event is sent via EventDispatcher, hence we don't need to set target manually
        ProcessEventManually( *reinterpret_cast<EventBase*>( lp ) );
        return 0;
    }
    case InternalSyncMessage::script_fail:
    {
        Fail( *reinterpret_cast<const qwr::u8string*>( lp ) );
        return 0;
    }
    case InternalSyncMessage::prepare_for_exit:
    {
        UnloadScript();
        return 0;
    }
    case InternalSyncMessage::ui_script_editor_saved:
    {
        ReloadScript();
        return 0;
    }
    case InternalSyncMessage::wnd_drag_drop:
    {
        if ( isMouseCaptured_ )
        {
            SetCaptureMouseState( false );
        }

        return 0;
    }
    case InternalSyncMessage::wnd_drag_enter:
    {
        isDraggingInside_ = true;
        return 0;
    }
    case InternalSyncMessage::wnd_drag_leave:
    {
        isDraggingInside_ = false;
        return 0;
    }
    case InternalSyncMessage::wnd_internal_drag_start:
    {
        hasInternalDrag_ = true;
        return 0;
    }
    case InternalSyncMessage::wnd_internal_drag_stop:
    {
        if ( !isDraggingInside_ )
        {
            isMouseTracked_ = false;
        }
        if ( isMouseCaptured_ )
        {
            SetCaptureMouseState( false );
        }

        hasInternalDrag_ = false;

        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

void js_panel_window::EditScript()
{
    switch ( settings_.GetSourceType() )
    {
    case config::ScriptSourceType::InMemory:
    case config::ScriptSourceType::File:
    case config::ScriptSourceType::Sample:
    {
        try
        {
            panel::EditScript( wnd_, settings_ );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
        break;
    }
    case config::ScriptSourceType::Package:
    {
        ShowConfigure( wnd_, ui::CDialogConf::Tab::package );
        break;
    }
    default:
    {
        assert( false );
        break;
    }
    }
}

void js_panel_window::ShowConfigure( HWND parent, ui::CDialogConf::Tab tab )
{
    if ( !modal_dialog_scope::can_create() )
    {
        return;
    }

    modal::ModalBlockingScope scope( parent, true );

    ui::CDialogConf dlg( this, tab );
    (void)dlg.DoModal( parent );
}

void js_panel_window::GenerateContextMenu( HMENU hMenu, int x, int y, uint32_t id_base )
{
    namespace fs = std::filesystem;

    try
    {
        CMenuHandle menu{ hMenu };

        auto curIdx = id_base;

        menu.AppendMenu( MF_STRING, ++curIdx, L"&Reload" );
        menu.AppendMenu( MF_SEPARATOR, UINT_PTR{}, LPCWSTR{} );
        menu.AppendMenu( MF_STRING, ++curIdx, L"&Open component folder" );
        menu.AppendMenu( MF_STRING, ++curIdx, L"&Open documentation" );
        menu.AppendMenu( MF_SEPARATOR, UINT_PTR{}, LPCWSTR{} );
        if ( settings_.GetSourceType() == config::ScriptSourceType::Package )
        {
            ++curIdx;

            const auto scriptFiles = config::GetPackageScriptFiles( settings_ );
            const auto scriptsDir = config::GetPackageScriptsDir( settings_ );

            CMenu cSubMenu;
            cSubMenu.CreatePopupMenu();

            auto scriptIdx = id_base + 100;

            for ( const auto& file: scriptFiles )
            {
                const std::filesystem::path relativePath = [&]
                    {
                    return ( file.filename() == "main.js" )
                        ? fs::path( "main.js" )
                        : fs::relative( file, scriptsDir );
                }();

                cSubMenu.AppendMenu( MF_STRING, ++scriptIdx, relativePath.c_str() );
            }

            menu.AppendMenu( MF_STRING, cSubMenu, L"&Edit panel script" );
            cSubMenu.Detach(); ///< AppendMenu takes ownership
        }
        else
        {
            menu.AppendMenu( MF_STRING, ++curIdx, L"&Edit panel script..." );
        }

        menu.AppendMenu( MF_STRING, ++curIdx, L"&Panel properties..." );
        menu.AppendMenu( MF_STRING, ++curIdx, L"&Configure panel..." );
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

void js_panel_window::ExecuteContextMenu( uint32_t id, uint32_t id_base )
{
    try
    {
        switch ( id - id_base )
        {
        case 1:
        {
            EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kScriptReload ), EventPriority::kControl );
            break;
        }
        case 2:
        {
            ShellExecute( nullptr, L"open", qwr::path::Component().c_str(), nullptr, nullptr, SW_SHOW );
            break;
        }
        case 3:
        {
            ShellExecute( nullptr, L"open", path::JsDocsIndex().c_str(), nullptr, nullptr, SW_SHOW );
            break;
        }
        case 4:
        {
            EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kScriptEdit ), EventPriority::kControl );
            break;
        }
        case 5:
        {
            EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kScriptShowProperties ), EventPriority::kControl );
            break;
        }
        case 6:
        {
            EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kScriptShowConfigure ), EventPriority::kControl );
            break;
        }
        }

        if ( id - id_base > 100 )
        {
            assert( settings_.GetSourceType() == config::ScriptSourceType::Package );

            const auto scriptFiles = config::GetPackageScriptFiles( settings_ );
            const auto fileIdx = std::min( id - id_base - 100, scriptFiles.size() ) - 1;

            panel::EditPackageScript( wnd_, scriptFiles[fileIdx], settings_ );
            ReloadScript();
        }
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

qwr::u8string js_panel_window::GetPanelId()
{
    return settings_.panelId;
}

qwr::u8string js_panel_window::GetPanelDescription( bool includeVersionAndAuthor )
{
    qwr::u8string ret = fmt::format( "{}", settings_.panelId );

    if ( !settings_.scriptName.empty() )
    {
        ret += fmt::format( ": {}", settings_.scriptName );

        if ( includeVersionAndAuthor )
        {
            if ( !settings_.scriptVersion.empty() )
                ret += fmt::format( " v{}", settings_.scriptVersion );

            if ( !settings_.scriptAuthor.empty() )
                ret += fmt::format( " by {}", settings_.scriptAuthor );
        }
    }

    return ret;
}

HDC js_panel_window::GetHDC() const
{
    return hDc_;
}

HWND js_panel_window::GetHWND() const
{
    return wnd_;
}

POINT& js_panel_window::MaxSize()
{
    return maxSize_;
}

POINT& js_panel_window::MinSize()
{
    return minSize_;
}

int js_panel_window::GetHeight() const
{
    return clientRect.Height();
}

int js_panel_window::GetWidth() const
{
    return clientRect.Width();
}

t_size& js_panel_window::DlgCode()
{
    return dlgCode_;
}

PanelType js_panel_window::GetPanelType() const
{
    return panelType_;
}

void js_panel_window::SetSettings_ScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion )
{
    assert( settings_.GetSourceType() != config::ScriptSourceType::Package );

    settings_.scriptName = scriptName;
    settings_.scriptAuthor = scriptAuthor;
    settings_.scriptVersion = scriptVersion;
}

void js_panel_window::SetSettings_PanelName( const qwr::u8string& panelName )
{
    assert( settings_.GetSourceType() != config::ScriptSourceType::Package );

    settings_.panelId = panelName;
    isPanelIdOverridenByScript_ = true;
}

void js_panel_window::SetSettings_DragAndDropStatus( bool isEnabled )
{
    assert( settings_.GetSourceType() != config::ScriptSourceType::Package );

    settings_.enableDragDrop = isEnabled;

    SetDragAndDropStatus( settings_.enableDragDrop );
}

void js_panel_window::SetSettings_CaptureFocusStatus( bool isEnabled )
{
    assert( settings_.GetSourceType() != config::ScriptSourceType::Package );

    settings_.shouldGrabFocus = isEnabled;
}

void js_panel_window::ResetLastDragParams()
{
    lastDragParams_.reset();
}

const std::optional<DragActionParams>& js_panel_window::GetLastDragParams() const
{
    return lastDragParams_;
}

bool js_panel_window::HasInternalDrag() const
{
    return hasInternalDrag_;
}

void js_panel_window::Repaint( bool force )
{
    if ( !force && !hRepaintTimer_ )
    { // paint message might be stalled if the message queue is not empty, we circumvent this via WM_TIMER
        hRepaintTimer_ = SetTimer( wnd_, NULL, USER_TIMER_MINIMUM, nullptr );
    }

    wnd_.RedrawWindow( nullptr, nullptr, RDW_INVALIDATE | ( force ? RDW_UPDATENOW : 0 ) );
}

void js_panel_window::RepaintRect( const CRect& requestedRect, bool force )
{
    if ( !force && !hRepaintTimer_ )
    { // paint message might be stalled if the message queue is not empty, we circumvent this via WM_TIMER
        hRepaintTimer_ = SetTimer( wnd_, NULL, USER_TIMER_MINIMUM, nullptr );
    }
    wnd_.RedrawWindow( &requestedRect, nullptr, RDW_INVALIDATE | ( force ? RDW_UPDATENOW : 0 ) );
}

void js_panel_window::RepaintBackground( const CRect& updateRect )
{
    CWindow parentWND = GetAncestor( wnd_, GA_PARENT );

    if ( !parentWND || IsIconic( core_api::get_main_window() ) || !wnd_.IsWindowVisible() )
        return;

    HWND hWND = nullptr;

    // HACK: for Tabbed controls : Find siblings
    while ( hWND = FindWindowEx( parentWND, hWND, nullptr, nullptr ) )
    {
        if ( hWND == wnd_ )
            continue;

        std::array<wchar_t, 64> buff = {};
        GetClassName( hWND, buff.data(), buff.size() );

        if ( wcsstr( buff.data(), L"SysTabControl32" ) )
        {
            parentWND = hWND;
            break;
        }
    }

    CRect childRect{ clientRect };
    CRgn childRgn{ ::CreateRectRgnIndirect( &childRect ) };
    childRgn.CombineRgn( ::CreateRectRgnIndirect( &updateRect ), RGN_DIFF );

    CRect combineRect{};
    childRgn.GetRgnBox( &combineRect );

    // bail out if the computed region('s bounding box) is empty
    if ( combineRect.IsRectEmpty() )
        return;

    CPoint position{ 0, 0 };
    wnd_.ClientToScreen( &position );
    parentWND.ScreenToClient( &position );

    CRect parentRect{ childRect };
    wnd_.ClientToScreen( &parentRect );
    parentWND.ScreenToClient( &parentRect );

    // Force Repaint
    wnd_.SetWindowRgn( childRgn, FALSE );
    parentWND.RedrawWindow( &parentRect, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );

    [&]
    {
        // Background bitmap
        CClientDC parentDC{ parentWND };
        CDC bgDC{ ::CreateCompatibleDC( parentDC ) };
        gdi::ObjectSelector autoBmp( bgDC, bmpBg_.m_hBitmap );

        // Paint BK
        BitBlt
        (
            bgDC,     childRect.left, childRect.top, childRect.Width(), childRect.Height(),
            parentDC, position.x,     position.y,
            SRCCOPY
        );
    }();

    wnd_.SetWindowRgn( nullptr, FALSE );

    if ( smp::config::EdgeStyle::NoEdge != settings_.edgeStyle )
        wnd_.SendMessage( WM_NCPAINT, 1, 0 );
}

bool js_panel_window::LoadScript( bool isFirstLoad )
{
    pfc::hires_timer timer;
    timer.start();

    hasFailed_ = false;
    isPanelIdOverridenByScript_ = false;

    try
    {
        SetDragAndDropStatus( settings_.enableDragDrop );
    }
    catch ( const qwr::QwrException& e )
    {
        smp::utils::LogWarning( e.what() );
    }
    DynamicMainMenuManager::Get().RegisterPanel( wnd_, settings_.panelId );

    const auto extstyle = [&] {
        DWORD extstyle = wnd_.GetWindowLongPtr( GWL_EXSTYLE );

        extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
        extstyle |= ConvertEdgeStyleToNativeFlags( settings_.edgeStyle );

        return extstyle;
    }();

    wnd_.SetWindowLongPtr( GWL_EXSTYLE, extstyle );
    wnd_.GetClientRect( &clientRect );
    wnd_.SetWindowPos( nullptr, 0, 0, clientRect.right, clientRect.bottom, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

    maxSize_ = { INT_MAX, INT_MAX };
    minSize_ = { 0, 0 };
    wnd_.PostMessage( static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_all, 0 );

    // error reporting handled inside
    if ( !pJsContainer_->Initialize() )
        return false;

    pTimeoutManager_->SetLoadingStatus( true );

    if ( settings_.script )
    {
        modal::WhitelistedScope scope; // Initial script execution must always be whitelisted

        // error reporting handled inside
        if ( !pJsContainer_->ExecuteScript( *settings_.script ) )
            return false;
    }
    else
    {
        if ( settings_.GetSourceType() == config::ScriptSourceType::Package )
        {
            assert( settings_.packageId );
            config::MarkPackageAsInUse( *settings_.packageId );
        }

        modal::WhitelistedScope scope; // Initial script execution must always be whitelisted
        assert( settings_.scriptPath );
        if ( !pJsContainer_->ExecuteScriptFile( *settings_.scriptPath ) )
        { // error reporting handled inside
            return false;
        }
    }

    pTimeoutManager_->SetLoadingStatus( false );

    FB2K_console_formatter() << fmt::format
    (
        SMP_NAME_WITH_VERSION " ({}): initialized in {} ms",
        GetPanelDescription(),
        static_cast<uint32_t>( timer.query() * 1000 )
    );

    if ( !isFirstLoad )
    { // Reloading script won't trigger WM_SIZE, so invoke it explicitly.
        // We need to go through message loop to handle all JS logic correctly (e.g. jobs).
        EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kWndResize ), EventPriority::kResize );
    }

    return true;
}

void js_panel_window::UnloadScript( bool force )
{
    // possible during startup config load
    if ( !pJsContainer_ )
        return;

    // should not go in JS again when forced to unload (e.g. in case of an error)
    if ( !force )
        pJsContainer_->InvokeJsCallback( "on_script_unload" );

    DynamicMainMenuManager::Get().UnregisterPanel( wnd_ );
    pJsContainer_->Finalize();
    pTimeoutManager_->StopAllTimeouts();
    selectionHolder_.release();

    try
    {
        SetDragAndDropStatus( false );
    }
    catch ( const qwr::QwrException& )
    {
    }
}

void js_panel_window::CreateDrawContext()
{
    DeleteDrawContext();

    bmp_.CreateCompatibleBitmap( hDc_, clientRect.Width(), clientRect.Height() );

    if ( settings_.isPseudoTransparent )
    {
        bmpBg_.CreateCompatibleBitmap( hDc_, clientRect.Width(), clientRect.Height() );
    }
}

void js_panel_window::DeleteDrawContext()
{
    if ( bmp_ )
    {
        bmp_.DeleteObject();
    }

    if ( bmpBg_ )
    {
        bmpBg_.DeleteObject();
    }
}

void js_panel_window::OnContextMenu( int x, int y )
{
    if ( modal::IsModalBlocked() )
    {
        return;
    }

    modal::MessageBlockingScope scope;

    POINT p{ x, y };
    wnd_.ClientToScreen( &p );

    CMenu menu = CreatePopupMenu();
    constexpr uint32_t base_id = 0;
    GenerateContextMenu( menu, p.x, p.y, base_id );

    // yup, WinAPI at it's best: BOOL is used as an integer index here
    const uint32_t ret = menu.TrackPopupMenu( TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, p.x, p.y, wnd_, nullptr );
    ExecuteContextMenu( ret, base_id );
}

void js_panel_window::OnCreate( HWND hWnd )
{
    wnd_ = hWnd;
    hDc_ = wnd_.GetDC();

    wnd_.GetClientRect( &clientRect );

    CreateDrawContext();

    pTarget_ = std::make_shared<PanelTarget>( *this );
    EventDispatcher::Get().AddWindow( wnd_, pTarget_ );

    pJsContainer_ = std::make_shared<mozjs::JsContainer>( *this );
    pTimeoutManager_ = std::make_unique<TimeoutManager>( pTarget_ );

    LoadScript( true );
}

void js_panel_window::OnDestroy()
{
    // Careful when changing invocation order here!
    UnloadScript();

    if ( pTarget_ )
        pTarget_->UnlinkPanel();

    if ( pTimeoutManager_ )
    {
        pTimeoutManager_->Finalize();
        pTimeoutManager_.reset();
    }

    EventDispatcher::Get().RemoveWindow( wnd_ );

    if ( hRepaintTimer_ )
    {
        KillTimer( wnd_, hRepaintTimer_ );
        hRepaintTimer_ = NULL;
    }

    pJsContainer_.reset();

    DeleteDrawContext();

    ReleaseDC( wnd_, hDc_ );
}

bool js_panel_window::HasFailed()
{
    return hasFailed_ || !pJsContainer_
           || mozjs::JsContainer::JsStatus::EngineFailed == pJsContainer_->GetStatus()
           || mozjs::JsContainer::JsStatus::Failed == pJsContainer_->GetStatus();
}

void js_panel_window::OnPaint( HDC hDC, const CRect& updateRect )
{
    if ( !hDC || !bmp_ )
        return;

    CDC cDC{ CreateCompatibleDC( hDC ) };
    gdi::ObjectSelector autoBmp( cDC, bmp_.m_hBitmap );

    if ( HasFailed() )
    {
        OnPaintErrorScreen( cDC );
    }
    else
    {
        if ( settings_.isPseudoTransparent )
        {
            CDC bDC{ CreateCompatibleDC( hDC ) };
            gdi::ObjectSelector autoBg( bDC, bmpBg_.m_hBitmap );

            BitBlt
            (
                cDC, updateRect.left, updateRect.top, updateRect.Width(), updateRect.Height(),
                bDC, updateRect.left, updateRect.top, SRCCOPY
            );
        }
        else
            cDC.FillRect( clientRect, (HBRUSH)( COLOR_WINDOW + 1 ) );

        OnPaintJs( cDC, updateRect );
    }

    BitBlt
    (
        hDC, updateRect.left, updateRect.top, updateRect.Width(), updateRect.Height(),
        cDC, updateRect.left, updateRect.top,
        SRCCOPY
    );
}

void js_panel_window::OnPaintErrorScreen( HDC hDC )
{
    CDCHandle cDC{ hDC };
    CBrush brush;

    gdi::ObjectSelector brushSelector( cDC, brush.CreateSolidBrush( RGB( 0xcc, 0x11, 0x22 ) ) );
    cDC.FillRect( clientRect, brush );

    CFont font;
    gdi::ObjectSelector fontSelector( cDC, font.CreateFontW
    (
        20,                             // cap height px > 0 (= default) > em height px
        0,                              // width/aspect ratio (aka condensed / wide)
        0,                              // escapement, angle of rotation from baseline (0.1 degree units)
        0,                              // "orientation"
        FW_BOLD,                        // weight
        FALSE,                          // italic
        FALSE,                          // underline
        FALSE,                          // strikeout
        DEFAULT_CHARSET,                // charset
        OUT_TT_PRECIS,                  // draw precision
        CLIP_DEFAULT_PRECIS,            // clip precision
        CLEARTYPE_QUALITY,              // render quality
        DEFAULT_PITCH | FF_DONTCARE,    // pitch (fixed / proportional / script / etc)
        /* L"Tahoma"*/ NULL             // face (NULL = SYSTEM)
    ) );

    cDC.SetTextColor( RGB( 0xff, 0xcc, 0x33 ) );
    cDC.SetBkMode( TRANSPARENT );
    cDC.DrawTextW( L"Aw, crashed :(", -1, clientRect, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );
}

void js_panel_window::OnPaintJs( HDC memDC, const CRect& updateRect )
{
    // GM_ADVANCED is needed for GDI transforms (and DirectWrite textOrientations)
    qwr::error::CheckWinApi( ERROR != SetGraphicsMode( memDC, GM_ADVANCED ), "SetGraphicsMode" );

    // copy textcontrast to the GDI+ graphics object
    // range 1000 - 2200, defaults differ: GDI is 1900 vs 1400 in GDI+ (according to docs)
    UINT textContrast = 1000;
    SystemParametersInfoW( SPI_GETFONTSMOOTHINGCONTRAST, 0, &textContrast, 0 );

    Gdiplus::Graphics graphics( memDC );
    graphics.SetTextContrast( textContrast );

    // note: default cliprect (which is intersected here) is the paintrect from WM_PAINT
    graphics.IntersectClip( Gdiplus::Rect{ updateRect.left, updateRect.top, updateRect.Width(), updateRect.Height() } );

    // check if the redraw would be visible at all
    if ( graphics.IsClipEmpty() || graphics.IsVisibleClipEmpty() )
        return;

    pJsContainer_->InvokeOnPaint( graphics );
}

void js_panel_window::OnSizeDefault()
{
    CreateDrawContext();
}

void js_panel_window::OnSizeUser()
{
    pJsContainer_->InvokeJsCallback( "on_size", clientRect.Width(), clientRect.Height() );

    if ( !settings_.isPseudoTransparent )
    {
        Repaint();
        return;
    }

        EventDispatcher::Get()
            .PutEvent
            (
                wnd_,
                std::make_unique<Event_Basic>( EventId::kWndRepaintBackground ),
                EventPriority::kRedraw
            );
}

void js_panel_window::SetCaptureMouseState( bool shouldCapture )
{
    if ( shouldCapture )
    {
        ::SetCapture( wnd_ );
    }
    else
    {
        ::ReleaseCapture();
    }
    isMouseCaptured_ = shouldCapture;
}

void js_panel_window::SetDragAndDropStatus( bool isEnabled )
{
    isDraggingInside_ = false;
    hasInternalDrag_ = false;
    lastDragParams_.reset();

    if ( isEnabled )
    {
        // if we have a handler already, skip
        if ( dropTargetHandler_ )
            return;

        dropTargetHandler_.Attach( new com::ComPtrImpl<com::TrackDropTarget>( *this ) );
        qwr::error::CheckHR( dropTargetHandler_->RegisterDragDrop(), "RegisterDragDrop" );
    }
    else
    {
        // release the handler if held
        if ( !dropTargetHandler_ )
            return;

        dropTargetHandler_->RevokeDragDrop();
        dropTargetHandler_.Release();
    }
}

} // namespace smp::panel
