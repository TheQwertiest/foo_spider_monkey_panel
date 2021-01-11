#include <stdafx.h>

#include "js_panel_window.h"

#include <com_objects/host_drop_target.h>
#include <js_engine/js_container.h>
#include <panel/com_message_scope.h>
#include <panel/drop_action_params.h>
#include <panel/message_blocking_scope.h>
#include <panel/message_manager.h>
#include <ui/ui_conf_new.h>
#include <utils/art_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/image_helpers.h>

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

void js_panel_window::JsEngineFail( const std::u8string& errorText )
{
    qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorText );
    wnd_.SendMessage( static_cast<UINT>( InternalSyncMessage::script_error ), 0, 0 );
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
        JsEngineFail( e.what() );
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
        JsEngineFail( e.what() );
        return false;
    }
}

LRESULT js_panel_window::on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    static uint32_t nestedCounter = 0;
    ++nestedCounter;

    qwr::final_action jobsRunner( [hWnd = wnd_.m_hWnd] {
        --nestedCounter;

        if ( !nestedCounter )
        { // Jobs (e.g. futures) should be drained only with empty JS stack and after the current task (as required by ES).
            // Also see https://developer.mozilla.org/en-US/docs/Web/JavaScript/EventLoop#Run-to-completion
            mozjs::JsContainer::RunJobs();
        }
        if ( !nestedCounter || MessageBlockingScope::IsBlocking() )
        {
            message_manager::instance().RequestNextAsyncMessage( hWnd );
        }
    } );

    if ( message_manager::IsAsyncMessage( msg ) )
    {
        if ( nestedCounter == 1 || MessageBlockingScope::IsBlocking() )
        {
            auto optMessage = message_manager::instance().ClaimAsyncMessage( wnd_, msg, wp, lp );
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

    if ( IsInEnumRange<CallbackMessage>( msg ) )
    {
        return process_callback_messages( static_cast<CallbackMessage>( msg ) );
    }
    else if ( IsInEnumRange<PlayerMessage>( msg ) )
    {
        return process_player_messages( static_cast<PlayerMessage>( msg ), wp, lp );
    }
    else if ( IsInEnumRange<InternalAsyncMessage>( msg ) )
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
        on_panel_create( hwnd );
        return 0;
    }
    case WM_DESTROY:
    {
        on_panel_destroy();
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
        ReloadScript();
        return 0;

    case WM_ERASEBKGND:
    {
        on_erase_background();
        return 1;
    }
    case WM_PAINT:
    {
        if ( isPaintInProgress_ )
        {
            return std::nullopt;
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
            return 0;
        }

        if ( ComMessageScope::IsInScope() )
        { // we have entered message loop because of COM messaging, delay repaint event to avoid deadlocks
            isPaintInProgress_ = false;
            Repaint();
            return 0;
        }

        {
            CPaintDC dc{ wnd_ };
            on_paint( dc, dc.m_ps.rcPaint );
        }

        isPaintInProgress_ = false;
        return 0;
    }
    case WM_SIZE:
    {
        CRect rc;
        wnd_.GetClientRect( &rc );
        on_size( rc.Width(), rc.Height() );
        return 0;
    }
    case WM_GETMINMAXINFO:
    { // This message will be called before WM_CREATE as well,
        // but we don't need to handle it before panel creation,
        // since default values suit us just fine
        auto pmmi = reinterpret_cast<LPMINMAXINFO>( lp );
        memcpy( &pmmi->ptMaxTrackSize, &MaxSize(), sizeof( POINT ) );
        memcpy( &pmmi->ptMinTrackSize, &MinSize(), sizeof( POINT ) );
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
        on_mouse_button_down( msg, wp, lp );
        return std::nullopt;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    {
        if ( on_mouse_button_up( msg, wp, lp ) )
        {
            return 0;
        }
        return std::nullopt;
    }
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    {
        on_mouse_button_dblclk( msg, wp, lp );
        return std::nullopt;
    }
    case WM_CONTEXTMENU:
    {
        on_context_menu( GET_X_LPARAM( lp ), GET_Y_LPARAM( lp ) );
        return 1;
    }
    case WM_MOUSEMOVE:
    {
        on_mouse_move( wp, lp );
        return std::nullopt;
    }
    case WM_MOUSELEAVE:
    {
        on_mouse_leave();
        return std::nullopt;
    }
    case WM_MOUSEWHEEL:
    {
        on_mouse_wheel( wp );
        return std::nullopt;
    }
    case WM_MOUSEHWHEEL:
    {
        on_mouse_wheel_h( wp );
        return std::nullopt;
    }
    case WM_SETCURSOR:
    {
        return 1;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        on_key_down( wp );
        return 0;
    }
    case WM_KEYUP:
    {
        on_key_up( wp );
        return 0;
    }
    case WM_CHAR:
    {
        on_char( wp );
        return 0;
    }
    case WM_SETFOCUS:
    {
        on_focus( true );
        return std::nullopt;
    }
    case WM_KILLFOCUS:
    {
        on_focus( false );
        return std::nullopt;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::process_callback_messages( CallbackMessage msg )
{
    auto pCallbackData = message_manager::instance().ClaimCallbackMessageData( wnd_, msg );
    auto& callbackData = *pCallbackData;

    switch ( msg )
    {
    case CallbackMessage::fb_item_focus_change:
    {
        on_item_focus_change( callbackData );
        return 0;
    }
    case CallbackMessage::fb_item_played:
    {
        on_item_played( callbackData );
        return 0;
    }
    case CallbackMessage::fb_library_items_added:
    {
        on_library_items_added( callbackData );
        return 0;
    }
    case CallbackMessage::fb_library_items_changed:
    {
        on_library_items_changed( callbackData );
        return 0;
    }
    case CallbackMessage::fb_library_items_removed:
    {
        on_library_items_removed( callbackData );
        return 0;
    }
    case CallbackMessage::fb_metadb_changed:
    {
        on_metadb_changed( callbackData );
        return 0;
    }
    case CallbackMessage::fb_playback_edited:
    {
        on_playback_edited( callbackData );
        return 0;
    }
    case CallbackMessage::fb_playback_new_track:
    {
        on_playback_new_track( callbackData );
        return 0;
    }
    case CallbackMessage::fb_playback_seek:
    {
        on_playback_seek( callbackData );
        return 0;
    }
    case CallbackMessage::fb_playback_time:
    {
        on_playback_time( callbackData );
        return 0;
    }
    case CallbackMessage::fb_volume_change:
    {
        on_volume_change( callbackData );
        return 0;
    }
    case CallbackMessage::internal_get_album_art_done:
    {
        on_get_album_art_done( callbackData );
        return 0;
    }
    case CallbackMessage::internal_load_image_done:
    {
        on_load_image_done( callbackData );
        return 0;
    }
    case CallbackMessage::internal_load_image_promise_done:
    case CallbackMessage::internal_get_album_art_promise_done:
    case CallbackMessage::internal_timer_proc:
    {
        on_js_task( callbackData );
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::process_player_messages( PlayerMessage msg, WPARAM wp, LPARAM lp )
{
    switch ( msg )
    {
    case PlayerMessage::fb_always_on_top_changed:
    {
        on_always_on_top_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_cursor_follow_playback_changed:
    {
        on_cursor_follow_playback_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_dsp_preset_changed:
    {
        on_dsp_preset_changed();
        return 0;
    }
    case PlayerMessage::fb_output_device_changed:
    {
        on_output_device_changed();
        return 0;
    }
    case PlayerMessage::fb_playback_dynamic_info:
    {
        on_playback_dynamic_info();
        return 0;
    }
    case PlayerMessage::fb_playback_dynamic_info_track:
    {
        on_playback_dynamic_info_track();
        return 0;
    }
    case PlayerMessage::fb_playback_follow_cursor_changed:
    {
        on_playback_follow_cursor_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_playback_order_changed:
    {
        on_playback_order_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_playback_pause:
    {
        on_playback_pause( wp );
        return 0;
    }
    case PlayerMessage::fb_playback_queue_changed:
    {
        on_playback_queue_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_playback_stop:
    {
        on_playback_stop( wp );
        return 0;
    }
    case PlayerMessage::fb_playback_starting:
    {
        on_playback_starting( wp, lp );
        return 0;
    }
    case PlayerMessage::fb_playlist_item_ensure_visible:
    {
        on_playlist_item_ensure_visible( wp, lp );
        return 0;
    }
    case PlayerMessage::fb_playlist_items_added:
    {
        on_playlist_items_added( wp );
        return 0;
    }
    case PlayerMessage::fb_playlist_items_reordered:
    {
        on_playlist_items_reordered( wp );
        return 0;
    }
    case PlayerMessage::fb_playlist_items_removed:
    {
        on_playlist_items_removed( wp, lp );
        return 0;
    }
    case PlayerMessage::fb_playlist_items_selection_change:
    {
        on_playlist_items_selection_change();
        return 0;
    }
    case PlayerMessage::fb_playlist_stop_after_current_changed:
    {
        on_playlist_stop_after_current_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_playlist_switch:
    {
        on_playlist_switch();
        return 0;
    }
    case PlayerMessage::fb_playlists_changed:
    {
        on_playlists_changed();
        return 0;
    }
    case PlayerMessage::fb_replaygain_mode_changed:
    {
        on_replaygain_mode_changed( wp );
        return 0;
    }
    case PlayerMessage::fb_selection_changed:
    {
        on_selection_changed();
        return 0;
    }
    case PlayerMessage::ui_font_changed:
    {
        on_font_changed();
        return 0;
    }
    case PlayerMessage::ui_colours_changed:
    {
        on_colours_changed();
        return 0;
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
    case InternalSyncMessage::script_error:
    {
        on_script_error();
        return 0;
    }
    case InternalSyncMessage::terminate_script:
    {
        UnloadScript();
        return 0;
    }
    case InternalSyncMessage::update_size_on_reload:
    {
        on_size( width_, height_ );
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
    case InternalAsyncMessage::main_menu_item:
    {
        on_main_menu( wp );
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
    case InternalAsyncMessage::show_configure:
    {
        show_configure_popup( wnd_ );
        return 0;
    }
    case InternalAsyncMessage::show_properties:
    {
        show_property_popup( wnd_ );
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

void js_panel_window::show_configure_popup( HWND parent )
{
    if ( !modal_dialog_scope::can_create() )
    {
        return;
    }

    modal_dialog_scope scope( parent );

    //ui::CDialogConf dlg( this );
    ui::CDialogConfNew dlg( this );
    (void)dlg.DoModal( parent );
}

void js_panel_window::show_property_popup( HWND parent )
{
    if ( !modal_dialog_scope::can_create() )
    {
        return;
    }

    modal_dialog_scope scope( parent );

    ui::CDialogConfNew dlg( this, ui::CDialogConfNew::Tab::properties );
    (void)dlg.DoModal( parent );
}

void js_panel_window::build_context_menu( HMENU hMenu, int x, int y, uint32_t id_base )
{
    CMenuHandle menu{ hMenu };
    menu.AppendMenu( MF_STRING, id_base + 1, L"&Reload" );
    menu.AppendMenu( MF_SEPARATOR, UINT_PTR{}, LPCWSTR{} );
    menu.AppendMenu( MF_STRING, id_base + 2, L"&Open component folder" );
    menu.AppendMenu( MF_STRING, id_base + 3, L"&Open documentation" );
    menu.AppendMenu( MF_SEPARATOR, UINT_PTR{}, LPCWSTR{} );
    menu.AppendMenu( MF_STRING, id_base + 4, L"&Properties" );
    menu.AppendMenu( MF_STRING, id_base + 5, L"&Configure..." );
}

void js_panel_window::execute_context_menu_command( uint32_t id, uint32_t id_base )
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
        const auto htmlHelp = qwr::path::Component() / L"docs/html/index.html";
        ShellExecute( nullptr, L"open", htmlHelp.c_str(), nullptr, nullptr, SW_SHOW );
        break;
    }
    case 4:
    {
        show_property_popup( wnd_ );
        break;
    }
    case 5:
    {
        show_configure_popup( wnd_ );
        break;
    }
    }
}

std::u8string js_panel_window::GetPanelId()
{
    return settings_.panelId;
}

std::u8string js_panel_window::GetPanelDescription( bool includeVersionAndAuthor )
{
    std::u8string ret = fmt::format( "{}", settings_.panelId );

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

void js_panel_window::SetScriptInfo( const std::u8string& scriptName, const std::u8string& scriptAuthor, const std::u8string& scriptVersion )
{
    settings_.scriptName = scriptName;
    settings_.scriptAuthor = scriptAuthor;
    settings_.scriptVersion = scriptVersion;
}

void js_panel_window::SetDragAndDropStatus( bool isEnabled )
{
    settings_.enableDragDrop = isEnabled;
    if ( isEnabled )
    {
        dropTargetHandler_.Attach( new com_object_impl_t<com::HostDropTarget>( wnd_ ) );

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
    if ( smp::config::EdgeStyle::NoEdge != settings_.edgeStyle )
    {
        wnd_.SendMessage( WM_NCPAINT, 1, 0 );
    }
}

bool js_panel_window::LoadScript( bool isFirstLoad )
{
    pfc::hires_timer timer;
    timer.start();

    message_manager::instance().EnableAsyncMessages( wnd_ );

    const auto extstyle = [&]() {
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
        if ( !pJsContainer_->ExecuteScript( *settings_.script ) )
        { // error reporting handled inside
            return false;
        }
    }
    else
    {
        assert( settings_.scriptPath );
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
        wnd_.SendMessage( static_cast<UINT>( InternalSyncMessage::update_size_on_reload ), 0, 0 );
    }

    return true;
}

void js_panel_window::UnloadScript()
{
    pJsContainer_->InvokeJsCallback( "on_script_unload" );
    message_manager::instance().DisableAsyncMessages( wnd_ );
    selectionHolder_.release();
    try
    {
        SetDragAndDropStatus( false );
    }
    catch ( const qwr::QwrException& )
    {
    }
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

void js_panel_window::on_context_menu( int x, int y )
{
    if ( MessageBlockingScope::IsBlocking() )
    {
        return;
    }

    MessageBlockingScope scope;

    CMenu menu = CreatePopupMenu();
    constexpr uint32_t base_id = 0;
    build_context_menu( menu, x, y, base_id );

    const uint32_t ret = menu.TrackPopupMenu( TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, x, y, wnd_, nullptr );
    execute_context_menu_command( ret, base_id );
}

void js_panel_window::on_erase_background()
{
    if ( settings_.isPseudoTransparent )
    {
        message_manager::instance().post_msg( wnd_, static_cast<UINT>( InternalAsyncMessage::refresh_bg ) );
    }
}

void js_panel_window::on_panel_create( HWND hWnd )
{
    wnd_ = hWnd;
    hDc_ = wnd_.GetDC();

    CRect rc;
    wnd_.GetClientRect( &rc );
    width_ = rc.Width();
    height_ = rc.Height();

    CreateDrawContext();

    message_manager::instance().AddWindow( wnd_ );

    pJsContainer_ = std::make_shared<mozjs::JsContainer>( *this );
    LoadScript( true );
}

void js_panel_window::on_panel_destroy()
{
    // Careful when changing invocation order here!

    UnloadScript();
    pJsContainer_.reset();

    message_manager::instance().RemoveWindow( wnd_ );
    DeleteDrawContext();
    ReleaseDC( wnd_, hDc_ );
}

void js_panel_window::on_script_error()
{
    Repaint();
    UnloadScript();
}

void js_panel_window::on_js_task( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<std::shared_ptr<mozjs::JsAsyncTask>>();
    pJsContainer_->InvokeJsAsyncTask( *std::get<0>( data ) );
}

void js_panel_window::on_always_on_top_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_always_on_top_changed",
                                     static_cast<bool>( wp ) );
}

void js_panel_window::on_char( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_char",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_colours_changed()
{
    pJsContainer_->InvokeJsCallback( "on_colours_changed" );
}

void js_panel_window::on_cursor_follow_playback_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_cursor_follow_playback_changed",
                                     static_cast<bool>( wp ) );
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

void js_panel_window::on_dsp_preset_changed()
{
    pJsContainer_->InvokeJsCallback( "on_dsp_preset_changed" );
}

void js_panel_window::on_focus( bool isFocused )
{
    if ( isFocused )
    {
        selectionHolder_ = ui_selection_manager::get()->acquire();
    }
    else
    {
        selectionHolder_.release();
    }
    pJsContainer_->InvokeJsCallback( "on_focus",
                                     static_cast<bool>( isFocused ) );
}

void js_panel_window::on_font_changed()
{
    pJsContainer_->InvokeJsCallback( "on_font_changed" );
}

void js_panel_window::on_get_album_art_done( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_ptr, uint32_t, std::unique_ptr<Gdiplus::Bitmap>, std::u8string>();
    pJsContainer_->InvokeJsCallback( "on_get_album_art_done",
                                     std::get<0>( data ),
                                     std::get<1>( data ),
                                     std::move( std::get<2>( data ) ),
                                     std::get<3>( data ) );
}

void js_panel_window::on_item_focus_change( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<t_size, t_size, t_size>();
    pJsContainer_->InvokeJsCallback( "on_item_focus_change",
                                     static_cast<int32_t>( std::get<0>( data ) ),
                                     static_cast<int32_t>( std::get<1>( data ) ),
                                     static_cast<int32_t>( std::get<2>( data ) ) );
}

void js_panel_window::on_item_played( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_ptr>();
    pJsContainer_->InvokeJsCallback( "on_item_played",
                                     std::get<0>( data ) );
}

void js_panel_window::on_key_down( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_key_down",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_key_up( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_key_up",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_load_image_done( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<uint32_t, std::unique_ptr<Gdiplus::Bitmap>, std::u8string>();
    pJsContainer_->InvokeJsCallback( "on_load_image_done",
                                     std::get<0>( data ),
                                     std::move( std::get<1>( data ) ),
                                     std::get<2>( data ) );
}

void js_panel_window::on_library_items_added( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_list>();
    pJsContainer_->InvokeJsCallback( "on_library_items_added",
                                     std::get<0>( data ) );
}

void js_panel_window::on_library_items_changed( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_list>();
    pJsContainer_->InvokeJsCallback( "on_library_items_changed",
                                     std::get<0>( data ) );
}

void js_panel_window::on_library_items_removed( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_list>();
    pJsContainer_->InvokeJsCallback( "on_library_items_removed",
                                     std::get<0>( data ) );
}

void js_panel_window::on_main_menu( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_main_menu",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_metadb_changed( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_list, bool>();
    pJsContainer_->InvokeJsCallback( "on_metadb_changed",
                                     std::get<0>( data ),
                                     std::get<1>( data ) );
}

void js_panel_window::on_mouse_button_dblclk( UINT msg, WPARAM wp, LPARAM lp )
{
    switch ( msg )
    {
    case WM_LBUTTONDBLCLK:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_lbtn_dblclk",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }

    case WM_MBUTTONDBLCLK:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_mbtn_dblclk",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }

    case WM_RBUTTONDBLCLK:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_rbtn_dblclk",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }
    default:
        assert( false );
        break;
    }
}

void js_panel_window::on_mouse_button_down( UINT msg, WPARAM wp, LPARAM lp )
{
    if ( settings_.shouldGrabFocus )
    {
        wnd_.SetFocus();
    }

    wnd_.SetCapture();

    switch ( msg )
    {
    case WM_LBUTTONDOWN:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_lbtn_down",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }
    case WM_MBUTTONDOWN:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_mbtn_down",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }
    case WM_RBUTTONDOWN:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_rbtn_down",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }
    default:
        assert( false );
        break;
    }
}

bool js_panel_window::on_mouse_button_up( UINT msg, WPARAM wp, LPARAM lp )
{
    bool ret = false;

    switch ( msg )
    {
    case WM_LBUTTONUP:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_lbtn_up",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }
    case WM_MBUTTONUP:
    {
        pJsContainer_->InvokeJsCallback( "on_mouse_mbtn_up",
                                         static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                         static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                         static_cast<uint32_t>( wp ) );
        break;
    }
    case WM_RBUTTONUP:
    {
        // Bypass the user code.
        if ( IsKeyPressed( VK_LSHIFT ) && IsKeyPressed( VK_LWIN ) )
        {
            break;
        }

        auto autoRet = pJsContainer_->InvokeJsCallback<bool>( "on_mouse_rbtn_up",
                                                              static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                                              static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                                              static_cast<uint32_t>( wp ) );
        ret = autoRet.value_or( false );
        break;
    }
    default:
        assert( false );
        break;
    }

    ReleaseCapture();
    return ret;
}

void js_panel_window::on_mouse_leave()
{
    isMouseTracked_ = false;

    pJsContainer_->InvokeJsCallback( "on_mouse_leave" );

    // Restore default cursor
    SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
}

void js_panel_window::on_mouse_move( WPARAM wp, LPARAM lp )
{
    if ( !isMouseTracked_ )
    {
        TRACKMOUSEEVENT tme{ sizeof( TRACKMOUSEEVENT ), TME_LEAVE, wnd_, HOVER_DEFAULT };
        TrackMouseEvent( &tme );
        isMouseTracked_ = true;

        // Restore default cursor
        SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
    }

    pJsContainer_->InvokeJsCallback( "on_mouse_move",
                                     static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                     static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_mouse_wheel( WPARAM wp )
{ // TODO: missing param doc
    pJsContainer_->InvokeJsCallback( "on_mouse_wheel",
                                     static_cast<int8_t>( GET_WHEEL_DELTA_WPARAM( wp ) > 0 ? 1 : -1 ),
                                     static_cast<int32_t>( GET_WHEEL_DELTA_WPARAM( wp ) ),
                                     static_cast<int32_t>( WHEEL_DELTA ) );
}

void js_panel_window::on_mouse_wheel_h( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_mouse_wheel_h",
                                     static_cast<int8_t>( GET_WHEEL_DELTA_WPARAM( wp ) > 0 ? 1 : -1 ) );
}

void js_panel_window::on_notify_data( WPARAM wp, LPARAM lp )
{
    pJsContainer_->InvokeOnNotify( wp, lp );
}

void js_panel_window::on_output_device_changed()
{
    pJsContainer_->InvokeJsCallback( "on_output_device_changed" );
}

void js_panel_window::on_paint( HDC dc, const CRect& updateRc )
{
    if ( !dc || !bmp_ )
    {
        return;
    }

    CDC memDc{ CreateCompatibleDC( dc ) };
    gdi::ObjectSelector autoBmp( memDc, bmp_.m_hBitmap );

    if ( mozjs::JsContainer::JsStatus::EngineFailed == pJsContainer_->GetStatus()
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
            memDc.FillRect( &rc, ( HBRUSH )( COLOR_WINDOW + 1 ) );
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

void js_panel_window::on_playback_dynamic_info()
{
    pJsContainer_->InvokeJsCallback( "on_playback_dynamic_info" );
}

void js_panel_window::on_playback_dynamic_info_track()
{
    pJsContainer_->InvokeJsCallback( "on_playback_dynamic_info_track" );
}

void js_panel_window::on_playback_edited( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_ptr>();
    pJsContainer_->InvokeJsCallback( "on_playback_edited",
                                     std::get<0>( data ) );
}

void js_panel_window::on_playback_follow_cursor_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_follow_cursor_changed",
                                     static_cast<bool>( wp ) );
}

void js_panel_window::on_playback_new_track( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<metadb_handle_ptr>();
    pJsContainer_->InvokeJsCallback( "on_playback_new_track",
                                     std::get<0>( data ) );
}

void js_panel_window::on_playback_order_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_order_changed",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_playback_pause( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_pause",
                                     static_cast<bool>( wp != 0 ) );
}

void js_panel_window::on_playback_queue_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_queue_changed",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_playback_seek( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<double>();
    pJsContainer_->InvokeJsCallback( "on_playback_seek",
                                     std::get<0>( data ) );
}

void js_panel_window::on_playback_starting( WPARAM wp, LPARAM lp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_starting",
                                     static_cast<uint32_t>( (playback_control::t_track_command)wp ),
                                     static_cast<bool>( lp != 0 ) );
}

void js_panel_window::on_playback_stop( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_stop",
                                     static_cast<uint32_t>( (playback_control::t_stop_reason)wp ) );
}

void js_panel_window::on_playback_time( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<double>();
    pJsContainer_->InvokeJsCallback( "on_playback_time",
                                     std::get<0>( data ) );
}

void js_panel_window::on_playlist_item_ensure_visible( WPARAM wp, LPARAM lp )
{
    pJsContainer_->InvokeJsCallback( "on_playlist_item_ensure_visible",
                                     static_cast<uint32_t>( wp ),
                                     static_cast<uint32_t>( lp ) );
}

void js_panel_window::on_playlist_items_added( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playlist_items_added",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_playlist_items_removed( WPARAM wp, LPARAM lp )
{
    pJsContainer_->InvokeJsCallback( "on_playlist_items_removed",
                                     static_cast<uint32_t>( wp ),
                                     static_cast<uint32_t>( lp ) );
}

void js_panel_window::on_playlist_items_reordered( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playlist_items_reordered",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_playlist_items_selection_change()
{
    pJsContainer_->InvokeJsCallback( "on_playlist_items_selection_change" );
}

void js_panel_window::on_playlist_stop_after_current_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playlist_stop_after_current_changed",
                                     static_cast<bool>( wp ) );
}

void js_panel_window::on_playlist_switch()
{
    pJsContainer_->InvokeJsCallback( "on_playlist_switch" );
}

void js_panel_window::on_playlists_changed()
{
    pJsContainer_->InvokeJsCallback( "on_playlists_changed" );
}

void js_panel_window::on_replaygain_mode_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_replaygain_mode_changed",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_selection_changed()
{
    pJsContainer_->InvokeJsCallback( "on_selection_changed" );
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
        message_manager::instance().post_msg( wnd_, static_cast<UINT>( InternalAsyncMessage::refresh_bg ) );
    }
    else
    {
        Repaint();
    }
}

void js_panel_window::on_volume_change( CallbackData& callbackData )
{
    auto& data = callbackData.GetData<float>();
    pJsContainer_->InvokeJsCallback( "on_volume_change",
                                     std::get<0>( data ) );
}

} // namespace smp::panel
