#include <stdafx.h>

#include "js_panel_window.h"

#include <com_objects/track_drop_target.h>
#include <com_utils/com_destruction_handler.h>
#include <config/delayed_package_utils.h>
#include <config/package_utils.h>
#include <events/event_focus.h>
#include <events/event_js_callback.h>
#include <events/event_manager.h>
#include <events/event_mouse.h>
#include <fb2k/mainmenu_dynamic.h>
#include <js_engine/js_container.h>
#include <panel/drop_action_params.h>
#include <panel/edit_script.h>
#include <panel/message_manager.h>
#include <panel/modal_blocking_scope.h>
#include <ui/ui_conf.h>
#include <utils/art_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/image_helpers.h>

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

ui_helpers::container_window::class_data& js_panel_window::get_class_data() const
{
    static class_data my_class_data = {
        TEXT( SMP_WINDOW_CLASS_NAME ),
        L"",
        0,
        false,
        false,
        0,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        ConvertEdgeStyleToNativeFlags( settings_.edgeStyle ),
        CS_DBLCLKS,
        true,
        true,
        true,
        IDC_ARROW
    };

    return my_class_data;
}

void js_panel_window::Fail( const qwr::u8string& errorText )
{
    hasFailed_ = true;
    qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorText );

    if ( wnd_ )
    {              // can be null during startup
        Repaint(); ///< repaint to display error message
    }
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

void js_panel_window::ReloadScript()
{
    if ( pJsContainer_ )
    { // Panel might be not loaded at all, if settings are changed from Preferences.
        UnloadScript();
        if ( !ReloadSettings() )
        {
            return;
        }
        LoadScript( false );
    }
}

void js_panel_window::LoadSettings( stream_reader& reader, t_size size, abort_callback& abort, bool reloadPanel )
{
    const auto settings = [&] {
        try
        {
            return config::PanelSettings::Load( reader, size, abort );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Can't load panel settings. Your panel will be completely reset!\n"
                                                                         "Error: {}",
                                                                         e.what() ) );
            return config::PanelSettings{};
        }
    }();

    if ( !UpdateSettings( settings, reloadPanel ) )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Can't load panel settings. Your panel will be completely reset!" ) );
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
    {
        ReloadScript();
    }
    return true;
}

bool js_panel_window::SaveSettings( stream_writer& writer, abort_callback& abort ) const
{
    try
    {
        config::MaybeSavePackageData( settings_ );
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
    static uint32_t nestedCounter = 0;
    ++nestedCounter;

    com::DeleteMarkedObjects();

    qwr::final_action jobsRunner( [hWnd = wnd_.m_hWnd] {
        --nestedCounter;

        if ( !nestedCounter )
        { // Jobs (e.g. futures) should be drained only with empty JS stack and after the current task (as required by ES).
            // Also see https://developer.mozilla.org/en-US/docs/Web/JavaScript/EventLoop#Run-to-completion
            mozjs::JsContainer::RunJobs();
        }
        if ( !nestedCounter || modal::IsModalBlocked() )
        {
            MessageManager::Get().RequestNextAsyncMessage( hWnd );
            EventManager::Get().RequestNextEvent( hWnd );
        }
    } );

    if ( EventManager::IsRequestEventMessage( msg ) )
    {
        if ( nestedCounter == 1 || modal::IsModalBlocked() )
        {
            EventManager::Get().ProcessNextEvent( wnd_ );
        }
    }
    else if ( MessageManager::IsAsyncMessage( msg ) )
    {
        if ( nestedCounter == 1 || modal::IsModalBlocked() )
        {
            auto optMessage = MessageManager::Get().ClaimAsyncMessage( wnd_, msg, wp, lp );
            if ( optMessage )
            {
                const auto [asyncMsg, asyncWp, asyncLp] = *optMessage;
                auto retVal = process_async_messages( asyncMsg, asyncWp, asyncLp );
                if ( retVal )
                {
                    return *retVal;
                }
            }
        }
    }
    else
    {
        if ( auto retVal = process_sync_messages( hwnd, msg, wp, lp );
             retVal.has_value() )
        {
            return *retVal;
        }
    }

    return DefWindowProc( hwnd, msg, wp, lp );
}

void js_panel_window::ExecuteJsTask( EventId id, IEvent_JsTask& task )
{
    if ( !pJsContainer_ )
    {
        return;
    }

    switch ( id )
    {
    case EventId::kInputFocus:
    {
        const auto pEvent = task.AsFocusEvent();
        assert( pEvent );

        if ( pEvent->IsFocused() )
        {
            selectionHolder_ = ui_selection_manager::get()->acquire();
        }
        else
        {
            selectionHolder_.release();
        }

        pEvent->JsExecute( *pJsContainer_ );

        break;
    }
    case EventId::kMouseLeftButtonUp:
    case EventId::kMouseMiddleButtonUp:
    {
        task.JsExecute( *pJsContainer_ );

        ReleaseCapture();

        break;
    }
    case EventId::kMouseRightButtonUp:
    {
        const auto autoCapture = qwr::final_action( [] { ReleaseCapture(); } );

        const auto pEvent = task.AsMouseEvent();
        assert( pEvent );

        // Bypass the user code.
        const auto useDefaultContextMenu = [&] {
            if ( IsKeyPressed( VK_LSHIFT ) && IsKeyPressed( VK_LWIN ) )
            {
                return true;
            }
            else
            {
                return !pEvent->JsExecute( *pJsContainer_ ).value_or( false );
            }
        }();

        if ( useDefaultContextMenu )
        {
            EventManager::Get().PutEvent( wnd_,
                                          std::make_unique<Event_Mouse>(
                                              EventId::kMouseContextMenu,
                                              pEvent->GetX(),
                                              pEvent->GetY(),
                                              0 ),
                                          EventPriority::kInputHigh );
        }
        break;
    }
    case EventId::kMouseLeftButtonDown:
    case EventId::kMouseMiddleButtonDown:
    case EventId::kMouseRightButtonDown:
    {
        if ( settings_.shouldGrabFocus )
        {
            wnd_.SetFocus();
        }

        wnd_.SetCapture();

        task.JsExecute( *pJsContainer_ );

        break;
    }
    case EventId::kMouseLeave:
    {
        isMouseTracked_ = false;

        task.JsExecute( *pJsContainer_ );

        // Restore default cursor
        SetCursor( LoadCursor( nullptr, IDC_ARROW ) );

        break;
    }
    case EventId::kMouseMove:
    {
        if ( !isMouseTracked_ )
        {
            TRACKMOUSEEVENT tme{ sizeof( TRACKMOUSEEVENT ), TME_LEAVE, wnd_, HOVER_DEFAULT };
            TrackMouseEvent( &tme );
            isMouseTracked_ = true;

            // Restore default cursor
            SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
        }

        task.JsExecute( *pJsContainer_ );

        break;
    }
    case EventId::kMouseContextMenu:
    {
        const auto pMouseEvent = task.AsMouseEvent();
        assert( pMouseEvent );

        OpenDefaultContextManu( pMouseEvent->GetX(), pMouseEvent->GetY() );
        break;
    }
    case EventId::kWndPaint:
    {
        if ( isPaintInProgress_ )
        {
            break;
        }
        isPaintInProgress_ = true;

        if ( settings_.isPseudoTransparent && isBgRepaintNeeded_ )
        { // Two pass redraw: paint BG > Repaint() > paint FG
            CRect rc;
            wnd_.GetUpdateRect( &rc, FALSE );
            RepaintBackground( &rc ); ///< Calls Repaint() inside

            isBgRepaintNeeded_ = false;
            isPaintInProgress_ = false;

            Repaint( true );
            break;
        }
        {
            CPaintDC dc{ wnd_ };
            on_paint( dc, dc.m_ps.rcPaint );
        }

        isPaintInProgress_ = false;
        break;
    }
    case EventId::kWndResize:
    {
        CRect rc;
        wnd_.GetClientRect( &rc );
        on_size( rc.Width(), rc.Height() );
        break;
    }
    default:
        task.JsExecute( *pJsContainer_ );
    }
}

std::optional<LRESULT> js_panel_window::process_sync_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if ( auto retVal = process_main_messages( hwnd, msg, wp, lp );
         retVal.has_value() )
    {
        return *retVal;
    }

    if ( auto retVal = process_window_messages( msg, wp, lp );
         retVal.has_value() )
    {
        return *retVal;
    }

    if ( IsInEnumRange<InternalSyncMessage>( msg ) )
    {
        if ( auto retVal = process_internal_sync_messages( static_cast<InternalSyncMessage>( msg ), wp, lp );
             retVal.has_value() )
        {
            return *retVal;
        }
    }

    return std::nullopt;
}

std::optional<LRESULT> js_panel_window::process_async_messages( UINT msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    if ( IsInEnumRange<InternalAsyncMessage>( msg ) )
    {
        return process_internal_async_messages( static_cast<InternalAsyncMessage>( msg ), wp, lp );
    }

    return std::nullopt;
}

std::optional<LRESULT> js_panel_window::process_main_messages( HWND hwnd, UINT msg, WPARAM, LPARAM )
{
    switch ( msg )
    {
    case WM_CREATE:
    {
        OnCreate( hwnd );
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

std::optional<LRESULT> js_panel_window::process_window_messages( UINT msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( msg )
    {
    case WM_DISPLAYCHANGE:
    case WM_THEMECHANGED:
    {
        ReloadScript();
        return 0;
    }
    case WM_ERASEBKGND:
    {
        if ( settings_.isPseudoTransparent )
        {
            MessageManager::Get().PostMsg( wnd_, static_cast<UINT>( InternalAsyncMessage::refresh_bg ) );
        }
        return 1;
    }
    case WM_PAINT:
    {
        if ( isPaintInProgress_ )
        {
            return std::nullopt;
        }
        EventManager::Get().PutEvent( wnd_, GenerateEvent_JsCallback( EventId::kWndPaint ), EventPriority::kRedraw );
        return 0;
    }
    case WM_SIZE:
    {
        // Update size immediately in case event fails to execute
        CRect rc;
        wnd_.GetClientRect( &rc );
        width_ = rc.Width();
        height_ = rc.Height();

        EventManager::Get().PutEvent( wnd_, GenerateEvent_JsCallback( EventId::kWndResize ), EventPriority::kResize );
        return 0;
    }
    case WM_GETMINMAXINFO:
    { // This message will be called before WM_CREATE as well,
        // but we don't need to handle it before panel creation,
        // since default values suit us just fine
        auto pmmi = reinterpret_cast<LPMINMAXINFO>( lp );
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
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseLeftButtonDown,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_MBUTTONDOWN:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseMiddleButtonDown,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_RBUTTONDOWN:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseRightButtonDown,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_LBUTTONUP:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseLeftButtonUp,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_MBUTTONUP:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseMiddleButtonUp,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_RBUTTONUP:
    {
        EventManager::Get().PutEvent( wnd_,
                                      std::make_unique<Event_Mouse>(
                                          EventId::kMouseRightButtonUp,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseLeftButtonDoubleClick,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_MBUTTONDBLCLK:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseMiddleButtonDoubleClick,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_RBUTTONDBLCLK:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseRightButtonDoubleClick,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_CONTEXTMENU:
    {
        // WM_CONTEXTMENU receives screen coordinates
        POINT p{ GET_X_LPARAM( lp ), GET_Y_LPARAM( lp ) };
        ScreenToClient( wnd_, &p );
        EventManager::Get().PutEvent( wnd_,
                                      std::make_unique<Event_Mouse>(
                                          EventId::kMouseContextMenu,
                                          p.x,
                                          p.y,
                                          0 ),
                                      EventPriority::kInputHigh );
        return 1;
    }
    case WM_MOUSEMOVE:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseMove,
                                          static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                          static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_MOUSELEAVE:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback( EventId::kMouseLeave ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_MOUSEWHEEL:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseVerticalWheel,
                                          static_cast<int8_t>( GET_WHEEL_DELTA_WPARAM( wp ) > 0 ? 1 : -1 ),
                                          static_cast<int32_t>( GET_WHEEL_DELTA_WPARAM( wp ) ),
                                          static_cast<int32_t>( WHEEL_DELTA ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_MOUSEHWHEEL:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kMouseHorizontalWheel,
                                          static_cast<int8_t>( GET_WHEEL_DELTA_WPARAM( wp ) > 0 ? 1 : -1 ) ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_SETCURSOR:
    {
        return 1;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kKeyboardKeyDown,
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return 0;
    }
    case WM_KEYUP:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kKeyboardKeyUp,
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return 0;
    }
    case WM_CHAR:
    {
        EventManager::Get().PutEvent( wnd_,
                                      GenerateEvent_JsCallback(
                                          EventId::kKeyboardChar,
                                          static_cast<uint32_t>( wp ) ),
                                      EventPriority::kInputHigh );
        return 0;
    }
    case WM_SETFOCUS:
    {
        EventManager::Get().PutEvent( wnd_,
                                      std::make_unique<Event_Focus>(
                                          EventId::kInputFocus,
                                          true ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    case WM_KILLFOCUS:
    {
        EventManager::Get().PutEvent( wnd_,
                                      std::make_unique<Event_Focus>(
                                          EventId::kInputFocus,
                                          false ),
                                      EventPriority::kInputHigh );
        return std::nullopt;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::process_internal_sync_messages( InternalSyncMessage msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( msg )
    {
    case InternalSyncMessage::notify_data:
    {
        on_notify_data( wp, lp );
        return 0;
    }
    case InternalSyncMessage::script_fail:
    {
        Fail( *reinterpret_cast<const qwr::u8string*>( lp ) );
        return 0;
    }
    case InternalSyncMessage::terminate_script:
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
        on_drag_drop( lp );
        return 0;
    }
    case InternalSyncMessage::wnd_drag_enter:
    {
        on_drag_enter( lp );
        return 0;
    }
    case InternalSyncMessage::wnd_drag_leave:
    {
        on_drag_leave();
        return 0;
    }
    case InternalSyncMessage::wnd_drag_over:
    {
        on_drag_over( lp );
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::process_internal_async_messages( InternalAsyncMessage msg, WPARAM wp, LPARAM )
{
    switch ( msg )
    {
    case InternalAsyncMessage::edit_script:
    {
        EditScript();
        return 0;
    }
    case InternalAsyncMessage::refresh_bg:
    {
        isBgRepaintNeeded_ = true;
        Repaint( true );
        return 0;
    }
    case InternalAsyncMessage::reload_script:
    {
        ReloadScript();
        return 0;
    }
    case InternalAsyncMessage::show_configure_legacy:
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
        return 0;
    }
    case InternalAsyncMessage::show_configure:
    {
        ShowConfigure( wnd_ );
        return 0;
    }
    case InternalAsyncMessage::show_properties:
    {
        ShowConfigure( wnd_, ui::CDialogConf::Tab::properties );
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
                const auto relativePath = [&] {
                    if ( file.filename() == "main.js" )
                    {
                        return fs::path( "main.js" );
                    }
                    else
                    {
                        return fs::relative( file, scriptsDir );
                    }
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
            ReloadScript();
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
            panel::EditScript( wnd_, settings_ );
            break;
        }
        case 5:
        {
            ShowConfigure( wnd_, ui::CDialogConf::Tab::properties );
            break;
        }
        case 6:
        {
            ShowConfigure( wnd_ );
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
            {
                ret += fmt::format( " v{}", settings_.scriptVersion );
            }
            if ( !settings_.scriptAuthor.empty() )
            {
                ret += fmt::format( " by {}", settings_.scriptAuthor );
            }
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
    return height_;
}

int js_panel_window::GetWidth() const
{
    return width_;
}

t_size& js_panel_window::DlgCode()
{
    return dlgCode_;
}

PanelType js_panel_window::GetPanelType() const
{
    return panelType_;
}

void js_panel_window::SetScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion )
{
    settings_.scriptName = scriptName;
    settings_.scriptAuthor = scriptAuthor;
    settings_.scriptVersion = scriptVersion;
}

void js_panel_window::SetPanelName( const qwr::u8string& panelName )
{
    settings_.panelId = panelName;
    isPanelIdOverridenByScript_ = true;
}

void js_panel_window::SetDragAndDropStatus( bool isEnabled )
{
    settings_.enableDragDrop = isEnabled;
    if ( isEnabled )
    {
        dropTargetHandler_.Attach( new com::ComPtrImpl<com::TrackDropTarget>( wnd_ ) );

        HRESULT hr = dropTargetHandler_->RegisterDragDrop();
        qwr::error::CheckHR( hr, "RegisterDragDrop" );
    }
    else
    {
        if ( dropTargetHandler_ )
        {
            dropTargetHandler_->RevokeDragDrop();
            dropTargetHandler_.Release();
        }
    }
}

void js_panel_window::SetCaptureFocusStatus( bool isEnabled )
{
    settings_.shouldGrabFocus = isEnabled;
}

void js_panel_window::Repaint( bool force /*= false */ )
{
    wnd_.RedrawWindow( nullptr, nullptr, RDW_INVALIDATE | ( force ? RDW_UPDATENOW : 0 ) );
}

void js_panel_window::RepaintRect( const CRect& rc, bool force )
{
    wnd_.RedrawWindow( &rc, nullptr, RDW_INVALIDATE | ( force ? RDW_UPDATENOW : 0 ) );
}

void js_panel_window::RepaintBackground( const CRect& updateRc )
{
    CWindow wnd_parent = GetAncestor( wnd_, GA_PARENT );

    if ( !wnd_parent || IsIconic( core_api::get_main_window() ) || !wnd_.IsWindowVisible() )
    {
        return;
    }

    // HACK: for Tab control
    // Find siblings
    HWND hwnd = nullptr;
    while ( ( hwnd = FindWindowEx( wnd_parent, hwnd, nullptr, nullptr ) ) )
    {
        if ( hwnd == wnd_ )
        {
            continue;
        }
        std::array<wchar_t, 64> buff;
        GetClassName( hwnd, buff.data(), buff.size() );
        if ( wcsstr( buff.data(), L"SysTabControl32" ) )
        {
            wnd_parent = hwnd;
            break;
        }
    }

    CRect rc_child{ 0, 0, static_cast<int>( width_ ), static_cast<int>( height_ ) };
    CRgn rgn_child{ ::CreateRectRgnIndirect( &rc_child ) };
    {
        CRgn rgn{ ::CreateRectRgnIndirect( &updateRc ) };
        rgn_child.CombineRgn( rgn, RGN_DIFF );
    }

    CPoint pt{ 0, 0 };
    wnd_.ClientToScreen( &pt );
    wnd_parent.ScreenToClient( &pt );

    CRect rc_parent{ rc_child };
    wnd_.ClientToScreen( &rc_parent );
    wnd_parent.ScreenToClient( &rc_parent );

    // Force Repaint
    wnd_.SetWindowRgn( rgn_child, FALSE );
    wnd_parent.RedrawWindow( &rc_parent, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );

    {
        // Background bitmap
        CClientDC dc_parent{ wnd_parent };
        CDC dc_bg{ ::CreateCompatibleDC( dc_parent ) };
        gdi::ObjectSelector autoBmp( dc_bg, bmpBg_.m_hBitmap );

        // Paint BK
        dc_bg.BitBlt( rc_child.left, rc_child.top, rc_child.Width(), rc_child.Height(), dc_parent, pt.x, pt.y, SRCCOPY );
    }

    wnd_.SetWindowRgn( nullptr, FALSE );
    if ( smp::config::EdgeStyle::NoEdge != settings_.edgeStyle )
    {
        wnd_.SendMessage( WM_NCPAINT, 1, 0 );
    }
}

bool js_panel_window::LoadScript( bool isFirstLoad )
{
    pfc::hires_timer timer;
    timer.start();

    hasFailed_ = false;
    isPanelIdOverridenByScript_ = false;

    DynamicMainMenuManager::Get().RegisterPanel( wnd_, settings_.panelId );
    MessageManager::Get().EnableAsyncMessages( wnd_ );
    EventManager::Get().EnableEventQueue( wnd_ );

    const auto extstyle = [&] {
        DWORD extstyle = wnd_.GetWindowLongPtr( GWL_EXSTYLE );
        extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
        extstyle |= ConvertEdgeStyleToNativeFlags( settings_.edgeStyle );

        return extstyle;
    }();

    wnd_.SetWindowLongPtr( GWL_EXSTYLE, extstyle );
    wnd_.SetWindowPos( nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

    maxSize_ = { INT_MAX, INT_MAX };
    minSize_ = { 0, 0 };
    wnd_.PostMessage( static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_all, 0 );

    if ( !pJsContainer_->Initialize() )
    { // error reporting handled inside
        return false;
    }

    if ( settings_.script )
    {
        modal::WhitelistedScope scope; // Initial script execution must always be whitelisted
        if ( !pJsContainer_->ExecuteScript( *settings_.script ) )
        { // error reporting handled inside
            return false;
        }
    }
    else
    {
        if ( settings_.GetSourceType() == config::ScriptSourceType::Package )
        {
            assert( settings_.packageId );
            config::MarkPackageAsInUse( *settings_.packageId );
        }

        assert( settings_.scriptPath );
        modal::WhitelistedScope scope; // Initial script execution must always be whitelisted
        if ( !pJsContainer_->ExecuteScriptFile( *settings_.scriptPath ) )
        { // error reporting handled inside
            return false;
        }
    }

    FB2K_console_formatter() << fmt::format(
        SMP_NAME_WITH_VERSION " ({}): initialized in {} ms",
        GetPanelDescription(),
        static_cast<uint32_t>( timer.query() * 1000 ) );

    if ( !isFirstLoad )
    { // Reloading script won't trigger WM_SIZE, so invoke it explicitly.
        // We need to go through message loop to handle all JS logic correctly (e.g. jobs).
        EventManager::Get().PutEvent( wnd_, GenerateEvent_JsCallback( EventId::kWndResize ), EventPriority::kResize );
    }

    return true;
}

void js_panel_window::UnloadScript( bool force )
{
    if ( !pJsContainer_ )
    { // possible during startup config load
        return;
    }

    if ( !force )
    { // should not go in JS again when forced to unload (e.g. in case of an error)
        pJsContainer_->InvokeJsCallback( "on_script_unload" );
    }

    MessageManager::Get().DisableAsyncMessages( wnd_ );
    EventManager::Get().DisableEventQueue( wnd_ );
    selectionHolder_.release();
    try
    {
        SetDragAndDropStatus( false );
    }
    catch ( const qwr::QwrException& )
    {
    }
    DynamicMainMenuManager::Get().UnregisterPanel( wnd_ );
    pJsContainer_->Finalize();
}

void js_panel_window::CreateDrawContext()
{
    DeleteDrawContext();

    bmp_.CreateCompatibleBitmap( hDc_, width_, height_ );

    if ( settings_.isPseudoTransparent )
    {
        bmpBg_.CreateCompatibleBitmap( hDc_, width_, height_ );
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

void js_panel_window::OpenDefaultContextManu( int x, int y )
{
    if ( modal::IsModalBlocked() )
    {
        return;
    }

    modal::MessageBlockingScope scope;

    POINT p{ x, y };
    ClientToScreen( wnd_, &p );

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

    CRect rc;
    wnd_.GetClientRect( &rc );
    width_ = rc.Width();
    height_ = rc.Height();

    CreateDrawContext();

    MessageManager::Get().AddWindow( wnd_ );
    EventManager::Get().AddWindow( wnd_, *this );

    pJsContainer_ = std::make_shared<mozjs::JsContainer>( *this );
    LoadScript( true );
}

void js_panel_window::OnDestroy()
{
    // Careful when changing invocation order here!

    UnloadScript();
    pJsContainer_.reset();

    MessageManager::Get().RemoveWindow( wnd_ );
    EventManager::Get().RemoveWindow( wnd_ );

    DeleteDrawContext();
    ReleaseDC( wnd_, hDc_ );
}

void js_panel_window::on_drag_drop( LPARAM lp )
{
    auto actionParams = reinterpret_cast<DropActionMessageParams*>( lp );
    pJsContainer_->InvokeOnDragAction( "on_drag_drop",
                                       actionParams->pt,
                                       actionParams->keyState,
                                       actionParams->actionParams );
}

void js_panel_window::on_drag_enter( LPARAM lp )
{
    auto actionParams = reinterpret_cast<DropActionMessageParams*>( lp );
    pJsContainer_->InvokeOnDragAction( "on_drag_enter",
                                       actionParams->pt,
                                       actionParams->keyState,
                                       actionParams->actionParams );
}

void js_panel_window::on_drag_leave()
{
    pJsContainer_->InvokeJsCallback( "on_drag_leave" );
}

void js_panel_window::on_drag_over( LPARAM lp )
{
    auto actionParams = reinterpret_cast<DropActionMessageParams*>( lp );
    pJsContainer_->InvokeOnDragAction( "on_drag_over",
                                       actionParams->pt,
                                       actionParams->keyState,
                                       actionParams->actionParams );
}

void js_panel_window::on_notify_data( WPARAM wp, LPARAM lp )
{
    pJsContainer_->InvokeOnNotify( wp, lp );
}

void js_panel_window::on_paint( HDC dc, const CRect& updateRc )
{
    if ( !dc || !bmp_ )
    {
        return;
    }

    CDC memDc{ CreateCompatibleDC( dc ) };
    gdi::ObjectSelector autoBmp( memDc, bmp_.m_hBitmap );

    if ( hasFailed_
         || mozjs::JsContainer::JsStatus::EngineFailed == pJsContainer_->GetStatus()
         || mozjs::JsContainer::JsStatus::Failed == pJsContainer_->GetStatus() )
    {
        on_paint_error( memDc );
    }
    else
    {
        if ( settings_.isPseudoTransparent )
        {
            CDC bgDc{ CreateCompatibleDC( dc ) };
            gdi::ObjectSelector autoBgBmp( bgDc, bmpBg_.m_hBitmap );

            memDc.BitBlt( updateRc.left,
                          updateRc.top,
                          updateRc.Width(),
                          updateRc.Height(),
                          bgDc,
                          updateRc.left,
                          updateRc.top,
                          SRCCOPY );
        }
        else
        {
            CRect rc{ 0, 0, static_cast<int>( width_ ), static_cast<int>( height_ ) };
            memDc.FillRect( &rc, (HBRUSH)( COLOR_WINDOW + 1 ) );
        }

        on_paint_user( memDc, updateRc );
    }

    BitBlt( dc, 0, 0, width_, height_, memDc, 0, 0, SRCCOPY );
}

void js_panel_window::on_paint_error( HDC memdc )
{
    CDCHandle cdc{ memdc };
    CFont font;
    font.CreateFont( 20,
                     0,
                     0,
                     0,
                     FW_BOLD,
                     FALSE,
                     FALSE,
                     FALSE,
                     DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE,
                     L"Tahoma" );
    gdi::ObjectSelector autoFontSelector( cdc, font.m_hFont );

    LOGBRUSH lbBack = { BS_SOLID, RGB( 225, 60, 45 ), 0 };
    CBrush brush;
    brush.CreateBrushIndirect( &lbBack );

    CRect rc{ 0, 0, static_cast<int>( width_ ), static_cast<int>( height_ ) };
    cdc.FillRect( &rc, brush );
    cdc.SetBkMode( TRANSPARENT );

    cdc.SetTextColor( RGB( 255, 255, 255 ) );
    cdc.DrawText( L"Aw, crashed :(", -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );
}

void js_panel_window::on_paint_user( HDC memdc, const CRect& updateRc )
{
    Gdiplus::Graphics gr( memdc );

    // SetClip() may improve performance slightly
    gr.SetClip( Gdiplus::Rect{ updateRc.left,
                               updateRc.top,
                               updateRc.Width(),
                               updateRc.Height() } );

    pJsContainer_->InvokeOnPaint( gr );
}

void js_panel_window::on_size( uint32_t w, uint32_t h )
{
    width_ = w;
    height_ = h;

    DeleteDrawContext();
    CreateDrawContext();

    pJsContainer_->InvokeJsCallback( "on_size",
                                     static_cast<uint32_t>( w ),
                                     static_cast<uint32_t>( h ) );

    if ( settings_.isPseudoTransparent )
    {
        MessageManager::Get().PostMsg( wnd_, static_cast<UINT>( InternalAsyncMessage::refresh_bg ) );
    }
    else
    {
        Repaint();
    }
}

} // namespace smp::panel
