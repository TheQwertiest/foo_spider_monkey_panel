#include <stdafx.h>

#include "panel_window.h"

#include <com_objects/track_drop_target.h>
#include <com_utils/com_destruction_handler.h>
#include <config/smp_package/delayed_package_actions.h>
#include <events/event_basic.h>
#include <events/event_dispatcher.h>
#include <events/event_drag.h>
#include <events/event_js_callback.h>
#include <events/event_mouse.h>
#include <fb2k/mainmenu_dynamic.h>
#include <js_backend/engine/js_container.h>
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

PanelWindow::PanelWindow( IPanelAdaptor& impl )
    : uie::container_window_v3(
        get_window_config(),
        [this]( auto&&... args ) { return impl_.OnMessage( std::forward<decltype( args )>( args )... ); } )
    , impl_( impl )
    , panelType_( impl.GetPanelType() )
{
}

PanelWindow::~PanelWindow()
{
}

uie::container_window_v3_config PanelWindow::get_window_config()
{
    uie::container_window_v3_config windowConfig( TEXT( SMP_WINDOW_CLASS_NAME ) );

    windowConfig.use_transparent_background = false;
    windowConfig.extended_window_styles = ConvertEdgeStyleToNativeFlags( config_.panelSettings.edgeStyle );
    windowConfig.class_styles = CS_DBLCLKS;
    windowConfig.class_cursor = IDC_ARROW;

    return windowConfig;
}

void PanelWindow::UpdateConfig( const smp::config::PanelConfig& config, bool reloadPanel )
{
    config_ = config;
    if ( reloadPanel )
    {
        ReloadScript();
    }
}

void PanelWindow::LoadConfig( stream_reader& reader, t_size size, abort_callback& abort, bool reloadPanel )
{
    const auto config = [&] {
        try
        {
            return config::PanelConfig::Load( reader, size, abort );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Can't load panel settings. Your panel will be completely reset!\n"
                                                                         "Error: {}",
                                                                         e.what() ) );
            return config::PanelConfig{};
        }
    }();

    UpdateConfig( config, reloadPanel );
}

bool PanelWindow::SaveConfig( stream_writer& writer, abort_callback& abort ) const
{
    try
    {
        config_.Save( writer, abort );
        return true;
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        return false;
    }
}

bool PanelWindow::IsPanelIdOverridenByScript() const
{
    return isPanelIdOverridenByScript_;
}

void PanelWindow::Fail( const qwr::u8string& errorText )
{
    hasFailed_ = true;
    qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorText );

    if ( wnd_ )
    {              // can be null during startup
        Repaint(); ///< repaint to display error message
    }
    UnloadScript( true );
}

void PanelWindow::Repaint( bool force )
{
    if ( !force && !hRepaintTimer_ )
    { // paint message might be stalled if the message queue is not empty, we circumvent this via WM_TIMER
        hRepaintTimer_ = SetTimer( wnd_, NULL, USER_TIMER_MINIMUM, nullptr );
    }
    wnd_.RedrawWindow( nullptr, nullptr, RDW_INVALIDATE | ( force ? RDW_UPDATENOW : 0 ) );
}

void PanelWindow::RepaintRect( const CRect& rc, bool force )
{
    if ( !force && !hRepaintTimer_ )
    { // paint message might be stalled if the message queue is not empty, we circumvent this via WM_TIMER
        hRepaintTimer_ = SetTimer( wnd_, NULL, USER_TIMER_MINIMUM, nullptr );
    }
    wnd_.RedrawWindow( &rc, nullptr, RDW_INVALIDATE | ( force ? RDW_UPDATENOW : 0 ) );
}

void PanelWindow::RepaintBackground( const CRect& updateRc )
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
    wnd_parent.RedrawWindow( &rc_parent, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW );

    {
        // Background bitmap
        CClientDC dc_parent{ wnd_parent };
        CDC dc_bg{ ::CreateCompatibleDC( dc_parent ) };
        gdi::ObjectSelector autoBmp( dc_bg, bmpBg_.m_hBitmap );

        // Paint BK
        dc_bg.BitBlt( rc_child.left, rc_child.top, rc_child.Width(), rc_child.Height(), dc_parent, pt.x, pt.y, SRCCOPY );
    }

    wnd_.SetWindowRgn( nullptr, FALSE );
    if ( smp::config::EdgeStyle::NoEdge != config_.panelSettings.edgeStyle )
    {
        wnd_.SendMessage( WM_NCPAINT, 1, 0 );
    }
}

const config::PanelConfig& PanelWindow::GetPanelConfig() const
{
    return config_;
}

const config::ResolvedPanelScriptSettings& PanelWindow::GetScriptSettings() const
{
    return scriptSettings_;
}

config::PanelProperties& PanelWindow::GetPanelProperties()
{
    return config_.properties;
}

qwr::u8string PanelWindow::GetPanelDescription( bool includeVersionAndAuthor )
{
    qwr::u8string ret = fmt::format( "{}", config_.panelSettings.id );

    if ( const auto& scriptName = scriptSettings_.GetScriptName();
         !scriptName.empty() )
    {
        ret += fmt::format( ": {}", scriptName );
        if ( includeVersionAndAuthor )
        {
            if ( const auto& scriptVersion = scriptSettings_.GetScriptVersion();
                 !scriptVersion.empty() )
            {
                ret += fmt::format( " v{}", scriptVersion );
            }
            if ( const auto& scriptAuthor = scriptSettings_.GetScriptAuthor();
                 !scriptAuthor.empty() )
            {
                ret += fmt::format( " by {}", scriptAuthor );
            }
        }
    }

    return ret;
}

HDC PanelWindow::GetHDC() const
{
    return hDc_;
}

HWND PanelWindow::GetHWND() const
{
    return wnd_;
}

POINT& PanelWindow::MaxSize()
{
    return maxSize_;
}

POINT& PanelWindow::MinSize()
{
    return minSize_;
}

int PanelWindow::GetHeight() const
{
    return height_;
}

int PanelWindow::GetWidth() const
{
    return width_;
}

TimeoutManager& PanelWindow::GetTimeoutManager()
{
    assert( pTimeoutManager_ );
    return *pTimeoutManager_;
}

t_size& PanelWindow::DlgCode()
{
    return dlgCode_;
}

PanelType PanelWindow::GetPanelType() const
{
    return panelType_;
}

DWORD PanelWindow::GetColour( unsigned type, const GUID& guid )
{
    return impl_.GetColour( type, guid );
}

HFONT PanelWindow::GetFont( unsigned type, const GUID& guid )
{
    return impl_.GetFont( type, guid );
}

void PanelWindow::SetSettings_ScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion )
{
    assert( scriptSettings_.GetSourceType() != config::ScriptSourceType::SmpPackage );

    auto& scriptRuntimeData = scriptSettings_.GetScriptRuntimeData();

    scriptRuntimeData.name = scriptName;
    scriptRuntimeData.author = scriptAuthor;
    scriptRuntimeData.version = scriptVersion;
}

void PanelWindow::SetSettings_PanelName( const qwr::u8string& panelName )
{
    assert( scriptSettings_.GetSourceType() != config::ScriptSourceType::SmpPackage );

    config_.panelSettings.id = panelName;
    isPanelIdOverridenByScript_ = true;
}

void PanelWindow::SetSettings_DragAndDropStatus( bool isEnabled )
{
    assert( scriptSettings_.GetSourceType() != config::ScriptSourceType::SmpPackage );

    auto& scriptRuntimeData = scriptSettings_.GetScriptRuntimeData();

    scriptRuntimeData.enableDragDrop = isEnabled;

    SetDragAndDropStatus( scriptRuntimeData.enableDragDrop );
}

void PanelWindow::SetSettings_CaptureFocusStatus( bool isEnabled )
{
    assert( scriptSettings_.GetSourceType() != config::ScriptSourceType::SmpPackage );

    auto& scriptRuntimeData = scriptSettings_.GetScriptRuntimeData();

    scriptRuntimeData.shouldGrabFocus = isEnabled;
}

void PanelWindow::ResetLastDragParams()
{
    lastDragParams_.reset();
}

const std::optional<DragActionParams>& PanelWindow::GetLastDragParams() const
{
    return lastDragParams_;
}

bool PanelWindow::HasInternalDrag() const
{
    return hasInternalDrag_;
}

void PanelWindow::ExecuteEvent_Basic( EventId id )
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
        switch ( scriptSettings_.GetSourceType() )
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
        if ( isPaintInProgress_ )
        {
            break;
        }
        isPaintInProgress_ = true;

        if ( config_.panelSettings.isPseudoTransparent && isBgRepaintNeeded_ )
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
            OnPaint( dc, dc.m_ps.rcPaint );
        }

        isPaintInProgress_ = false;

        break;
    }
    case EventId::kWndRepaintBackground:
    {
        isBgRepaintNeeded_ = true;
        Repaint( true );

        break;
    }
    case EventId::kWndResize:
    {
        if ( !pJsContainer_ )
        {
            break;
        }

        CRect rc;
        wnd_.GetClientRect( &rc );
        OnSizeUser( rc.Width(), rc.Height() );

        break;
    }
    default:
        break;
    }
}

void PanelWindow::ExecuteEvent_JsTask( EventId id, Event_JsExecutor& task )
{
    const auto execJs = [&]( auto& jsTask ) -> std::optional<bool> {
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
            auto dragParams = pDragEvent->GetDragParams();
            const auto bRet = pJsContainer_->InvokeOnDragAction( "on_drag_enter",
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
            auto dragParams = pDragEvent->GetDragParams();
            const auto bRet = pJsContainer_->InvokeOnDragAction( "on_drag_over",
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
            auto dragParams = pDragEvent->GetDragParams();
            const auto bRet = pJsContainer_->InvokeOnDragAction( "on_drag_drop",
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

bool PanelWindow::ExecuteEvent_JsCode( mozjs::JsAsyncTask& jsTask )
{
    if ( !pJsContainer_ )
    {
        return false;
    }

    return pJsContainer_->InvokeJsAsyncTask( jsTask );
}

void PanelWindow::OnSizeLimitChanged( LPARAM lp )
{
    return impl_.OnSizeLimitChanged( lp );
}

LRESULT PanelWindow::OnMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    // According to MSDN:
    ////
    // If no filter is specified, messages are processed in the following order:
    // - Sent messages
    // - Posted messages
    // - Input (hardware) messages and system internal events
    // - Sent messages (again)
    // - WM_PAINT messages
    // - WM_TIMER messages
    ////
    // Since we are constantly processing our own `event` messages, we need to take additional care
    // so as not to stall WM_PAINT and WM_TIMER messages.

    static uint32_t msgNestedCounter = 0;
    ++msgNestedCounter;
    const qwr::final_action autoComObjectDeleter( [&] {
        // delete only on exit as to avoid delaying processing of the current message due to reentrancy
        --msgNestedCounter;
        if ( !msgNestedCounter )
        {
            com::DeleteMarkedObjects();
        }
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

void PanelWindow::EditScript()
{
    switch ( scriptSettings_.GetSourceType() )
    {
    case config::ScriptSourceType::InMemory:
    {
        try
        {
            panel::EditScript( wnd_, std::get<config::RawInMemoryScript>( config_.scriptSource ).script );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
        break;
    }
    case config::ScriptSourceType::File:
    case config::ScriptSourceType::Sample:
    {
        try
        {
            panel::EditScriptFile( wnd_, scriptSettings_ );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
        break;
    }
    case config::ScriptSourceType::SmpPackage:
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

void PanelWindow::ShowConfigure( HWND parent, ui::CDialogConf::Tab tab )
{
    if ( !modal_dialog_scope::can_create() )
    {
        return;
    }

    modal::ModalBlockingScope scope( parent, true );

    ui::CDialogConf dlg( this, tab );
    (void)dlg.DoModal( parent );
}

void PanelWindow::GenerateContextMenu( HMENU hMenu, int x, int y, uint32_t id_base )
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
        if ( scriptSettings_.GetSourceType() == config::ScriptSourceType::SmpPackage )
        {
            const auto& package = scriptSettings_.GetSmpPackage();

            ++curIdx;

            const auto scriptFiles = package.GetScriptFiles();
            const auto scriptsDir = package.GetScriptsDir();

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

void PanelWindow::ExecuteContextMenu( uint32_t id, uint32_t id_base )
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
            assert( scriptSettings_.GetSourceType() == config::ScriptSourceType::SmpPackage );

            const auto& package = scriptSettings_.GetSmpPackage();

            const auto scriptFiles = package.GetScriptFiles();
            const auto fileIdx = std::min( id - id_base - 100, scriptFiles.size() ) - 1;

            panel::EditPackageScript( wnd_, scriptFiles[fileIdx] );
            ReloadScript();
        }
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

void PanelWindow::SetCaptureMouseState( bool shouldCapture )
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

void PanelWindow::SetDragAndDropStatus( bool isEnabled )
{
    isDraggingInside_ = false;
    hasInternalDrag_ = false;
    lastDragParams_.reset();
    if ( isEnabled )
    {
        if ( !dropTargetHandler_ )
        {
            dropTargetHandler_.Attach( new com::ComPtrImpl<com::TrackDropTarget>( *this ) );

            HRESULT hr = dropTargetHandler_->RegisterDragDrop();
            qwr::error::CheckHR( hr, "RegisterDragDrop" );
        }
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

bool PanelWindow::ReloadScriptSettings()
{
    try
    {
        scriptSettings_ = config::ResolvedPanelScriptSettings::ResolveSource( config_.scriptSource );
        return true;
    }
    catch ( const qwr::QwrException& e )
    {
        Fail( e.what() );
        return false;
    }
}

void PanelWindow::LoadScript( bool isFirstLoad )
{
    pfc::hires_timer timer;
    timer.start();

    hasFailed_ = false;
    isPanelIdOverridenByScript_ = false;

    try
    {
        SetDragAndDropStatus( scriptSettings_.ShouldEnableDragDrop() );
    }
    catch ( const qwr::QwrException& e )
    {
        smp::utils::LogWarning( e.what() );
    }
    DynamicMainMenuManager::Get().RegisterPanel( wnd_, config_.panelSettings.id );

    const auto extstyle = [&]() {
        DWORD extstyle = wnd_.GetWindowLongPtr( GWL_EXSTYLE );
        extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
        extstyle |= ConvertEdgeStyleToNativeFlags( config_.panelSettings.edgeStyle );

        return extstyle;
    }();

    wnd_.SetWindowLongPtr( GWL_EXSTYLE, extstyle );
    wnd_.SetWindowPos( nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

    maxSize_ = { INT_MAX, INT_MAX };
    minSize_ = { 0, 0 };
    wnd_.PostMessage( static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_all, 0 );

    if ( !pJsContainer_->Initialize() )
    { // error reporting handled inside
        return;
    }

    pTimeoutManager_->SetLoadingStatus( true );

    switch ( scriptSettings_.GetSourceType() )
    {
    case config::ScriptSourceType::InMemory:
    {
        modal::WhitelistedScope scope; // Initial script execution must always be whitelisted
        if ( !pJsContainer_->ExecuteScript( scriptSettings_.GetScript(), scriptSettings_.IsModuleScript() ) )
        { // error reporting handled inside
            return;
        }
        break;
    }
    case config::ScriptSourceType::SmpPackage:
    case config::ScriptSourceType::ModulePackage:
    case config::ScriptSourceType::Sample:
    case config::ScriptSourceType::File:
    {
        if ( scriptSettings_.GetSourceType() == config::ScriptSourceType::SmpPackage )
        {
            config::MarkPackageAsInUse( scriptSettings_.GetSmpPackage().id );
        }

        modal::WhitelistedScope scope; // Initial script execution must always be whitelisted
        if ( !pJsContainer_->ExecuteScriptFile( scriptSettings_.GetScriptPath(), scriptSettings_.IsModuleScript() ) )
        { // error reporting handled inside
            return;
        }
        break;
    }
    }

    pTimeoutManager_->SetLoadingStatus( false );

    FB2K_console_formatter() << fmt::format(
        SMP_NAME_WITH_VERSION " ({}): initialized in {} ms",
        GetPanelDescription(),
        static_cast<uint32_t>( timer.query() * 1000 ) );

    if ( !isFirstLoad )
    { // Reloading script won't trigger WM_SIZE, so invoke it explicitly.
        auto pEvent = std::make_unique<Event_Basic>( EventId::kWndResize );
        pEvent->SetTarget( pTarget_ );
        ProcessEventManually( *pEvent );
    }
}

void PanelWindow::UnloadScript( bool force )
{
    if ( !pJsContainer_ )
    { // possible during startup config load
        return;
    }

    if ( !force )
    { // should not go in JS again when forced to unload (e.g. in case of an error)
        pJsContainer_->InvokeJsCallback( "on_script_unload" );
    }

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

void PanelWindow::ReloadScript()
{
    if ( !pJsContainer_ )
    { // Panel might be not loaded at all, if settings are changed from Preferences.
        return;
    }

    UnloadScript();
    if ( !ReloadScriptSettings() )
    {
        return;
    }
    LoadScript( false );
}

void PanelWindow::CreateDrawContext()
{
    DeleteDrawContext();

    bmp_.CreateCompatibleBitmap( hDc_, width_, height_ );

    if ( config_.panelSettings.isPseudoTransparent )
    {
        bmpBg_.CreateCompatibleBitmap( hDc_, width_, height_ );
    }
}

void PanelWindow::DeleteDrawContext()
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

void PanelWindow::OnProcessingEventStart()
{
    ++eventNestedCounter_;
}

void PanelWindow::OnProcessingEventFinish()
{
    --eventNestedCounter_;

    if ( !eventNestedCounter_ )
    { // Jobs (e.g. futures) should be drained only with empty JS stack and after the current task (as required by ES).
        // Also see https://developer.mozilla.org/en-US/docs/Web/JavaScript/EventLoop#Run-to-completion
        mozjs::JsContainer::RunJobs();
    }

    if ( !eventNestedCounter_ || modal::IsModalBlocked() )
    {
        EventDispatcher::Get().RequestNextEvent( wnd_ );
    }
}

std::optional<LRESULT> PanelWindow::ProcessEvent()
{
    OnProcessingEventStart();
    qwr::final_action onEventProcessed( [&] {
        OnProcessingEventFinish();
    } );

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

void PanelWindow::ProcessEventManually( Runnable& runnable )
{
    OnProcessingEventStart();
    qwr::final_action onEventProcessed( [&] {
        OnProcessingEventFinish();
    } );

    runnable.Run();
}

std::optional<MSG> PanelWindow::GetStalledMessage()
{
    MSG msg;
    bool hasMessage = PeekMessage( &msg, wnd_, WM_TIMER, WM_TIMER, PM_REMOVE );
    if ( !hasMessage )
    {
        return std::nullopt;
    }

    if ( !hRepaintTimer_ )
    { // means that WM_PAINT was invoked properly
        return std::nullopt;
    }

    KillTimer( wnd_, hRepaintTimer_ );
    hRepaintTimer_ = NULL;

    return msg;
}

std::optional<LRESULT> PanelWindow::ProcessStalledMessage( const MSG& msg )
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

std::optional<LRESULT> PanelWindow::ProcessSyncMessage( const MSG& msg )
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

std::optional<LRESULT> PanelWindow::ProcessCreationMessage( const MSG& msg )
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

std::optional<LRESULT> PanelWindow::ProcessWindowMessage( const MSG& msg )
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
        if ( config_.panelSettings.isPseudoTransparent )
        {
            EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kWndRepaintBackground ), EventPriority::kRedraw );
        }
        return 1;
    }
    case WM_PAINT:
    {
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
        CRect rc;
        wnd_.GetClientRect( &rc );
        OnSizeDefault( rc.Width(), rc.Height() );

        auto pEvent = std::make_unique<Event_Basic>( EventId::kWndResize );
        pEvent->SetTarget( pTarget_ );
        ProcessEventManually( *pEvent );

        return 0;
    }
    case WM_GETMINMAXINFO:
    { // This message will be called before WM_CREATE as well,
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
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        static const std::unordered_map<int, EventId> kMsgToEventId{
            { WM_LBUTTONDOWN, EventId::kMouseLeftButtonDown },
            { WM_MBUTTONDOWN, EventId::kMouseMiddleButtonDown },
            { WM_RBUTTONDOWN, EventId::kMouseRightButtonDown }
        };

        if ( scriptSettings_.ShouldGrabFocus() )
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

std::optional<LRESULT> PanelWindow::ProcessInternalSyncMessage( InternalSyncMessage msg, WPARAM wp, LPARAM lp )
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

void PanelWindow::OnContextMenu( int x, int y )
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

void PanelWindow::OnCreate( HWND hWnd )
{
    wnd_ = hWnd;
    hDc_ = wnd_.GetDC();

    CRect rc;
    wnd_.GetClientRect( &rc );
    width_ = rc.Width();
    height_ = rc.Height();

    CreateDrawContext();

    pTarget_ = std::make_shared<PanelTarget>( *this );
    EventDispatcher::Get().AddWindow( wnd_, pTarget_ );

    pJsContainer_ = std::make_shared<mozjs::JsContainer>( *this );
    pTimeoutManager_ = std::make_unique<TimeoutManager>( pTarget_ );

    if ( !ReloadScriptSettings() )
    { // this might happen when script source fails to resolve
        return;
    }
    LoadScript( true );
}

void PanelWindow::OnDestroy()
{
    // Careful when changing invocation order here!

    UnloadScript();

    if ( pTarget_ )
    {
        pTarget_->UnlinkPanel();
    }

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

void PanelWindow::OnPaint( HDC dc, const CRect& updateRc )
{
    if ( !dc || !bmp_ )
    {
        return;
    }

    CDC memDc{ CreateCompatibleDC( dc ) };
    gdi::ObjectSelector autoBmp( memDc, bmp_.m_hBitmap );

    if ( hasFailed_
         || !pJsContainer_
         || mozjs::JsContainer::JsStatus::EngineFailed == pJsContainer_->GetStatus()
         || mozjs::JsContainer::JsStatus::Failed == pJsContainer_->GetStatus() )
    {
        OnPaintErrorScreen( memDc );
    }
    else
    {
        if ( config_.panelSettings.isPseudoTransparent )
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

        OnPaintJs( memDc, updateRc );
    }

    BitBlt( dc, 0, 0, width_, height_, memDc, 0, 0, SRCCOPY );
}

void PanelWindow::OnPaintErrorScreen( HDC memdc )
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

void PanelWindow::OnPaintJs( HDC memdc, const CRect& updateRc )
{
    Gdiplus::Graphics gr( memdc );

    // SetClip() may improve performance slightly
    gr.SetClip( Gdiplus::Rect{ updateRc.left,
                               updateRc.top,
                               updateRc.Width(),
                               updateRc.Height() } );

    pJsContainer_->InvokeOnPaint( gr );
}

void PanelWindow::OnSizeDefault( uint32_t w, uint32_t h )
{
    width_ = w;
    height_ = h;

    DeleteDrawContext();
    CreateDrawContext();
}

void PanelWindow::OnSizeUser( uint32_t w, uint32_t h )
{
    pJsContainer_->InvokeJsCallback( "on_size",
                                     static_cast<uint32_t>( w ),
                                     static_cast<uint32_t>( h ) );

    if ( config_.panelSettings.isPseudoTransparent )
    {
        EventDispatcher::Get().PutEvent( wnd_, std::make_unique<Event_Basic>( EventId::kWndRepaintBackground ), EventPriority::kRedraw );
    }
    else
    {
        Repaint();
    }
}

} // namespace smp::panel
