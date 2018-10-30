#include <stdafx.h>
#include "js_panel_window.h"
#include "panel_manager.h"
#include "popup_msg.h"

#include <ui/ui_conf.h>
#include <ui/ui_property.h>

#include <js_engine/js_container.h>
#include <js_utils/scope_helper.h>
#include <js_utils/art_helper.h>
#include <js_utils/image_helper.h>
#include <js_utils/gdi_helpers.h>

#include <drop_action_params.h>

using namespace smp;

js_panel_window::js_panel_window( PanelType instanceType )
    : panelType_( instanceType )
    , m_script_info( get_config_guid() )
{
}

ui_helpers::container_window::class_data& js_panel_window::get_class_data() const
{
    static class_data my_class_data =
        {
            _T( SMP_WINDOW_CLASS_NAME ),
            L"",
            0,
            false,
            false,
            0,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            edge_style_from_config( get_edge_style() ),
            CS_DBLCLKS,
            true,
            true,
            true,
            IDC_ARROW
        };

    return my_class_data;
}

void js_panel_window::update_script( const char* code )
{
    if ( code )
    {
        get_script_code() = code;
    }

    if ( pJsContainer_ )
    { // Panel might be not loaded at all, if settings are changed from Preferences.
        script_unload();
        script_load();
    }
}

void js_panel_window::JsEngineFail( const pfc::string8_fast& errorText )
{
    popup_msg::g_show( errorText, SMP_NAME );
    MessageBeep( MB_ICONASTERISK );

    SendMessage( hWnd_, static_cast<UINT>( InternalMessage::script_error ), 0, 0 );
}

LRESULT js_panel_window::on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{     
    // Microtasks (e.g. futures) should be drained only with empty JS stack and after the task (as required by ES).
    static uint32_t nestedCounter = 0;
    ++nestedCounter;

    mozjs::scope::final_action jobsRunner( [&pJsContainer = pJsContainer_, &nestedCounter = nestedCounter] {
        --nestedCounter;

        if ( pJsContainer && !nestedCounter )
        {
            pJsContainer->RunJobs();
        }
    } );

    if ( auto retVal = on_main_message( hwnd, msg, wp, lp ); retVal.has_value() )
    {
        return retVal.value();
    }

    if ( auto retVal = on_window_message( hwnd, msg, wp, lp ); retVal.has_value() )
    {
        return retVal.value();
    }

    if ( msg >= static_cast<UINT>( CallbackMessage::firstMessage )
         && msg <= static_cast<UINT>( CallbackMessage::lastMessage ) )
    {
        if ( auto retVal = on_callback_message( hwnd, msg, wp, lp ); retVal.has_value() )
        {
            return retVal.value();
        }
    }
    else if ( msg >= static_cast<UINT>( PlayerMessage::firstMessage )
              && msg <= static_cast<UINT>( PlayerMessage::lastMessage ) )
    {
        if ( auto retVal = on_player_message( hwnd, msg, wp, lp ); retVal.has_value() )
        {
            return retVal.value();
        }
    }
    else if ( msg >= static_cast<UINT>( InternalMessage::firstMessage )
              && msg <= static_cast<UINT>( InternalMessage::lastMessage ) )
    {
        if ( auto retVal = on_internal_message( hwnd, msg, wp, lp ); retVal.has_value() )
        {
            return retVal.value();
        }
    }

    return uDefWindowProc( hwnd, msg, wp, lp );
}

std::optional<LRESULT> js_panel_window::on_main_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
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

std::optional<LRESULT> js_panel_window::on_window_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( msg )
    {
    case WM_DISPLAYCHANGE:
    case WM_THEMECHANGED:
        update_script();
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

        if ( get_pseudo_transparent() && isBgRepaintNeeded_ )
        { // Two pass redraw: paint BG > Repaint() > paint FG
            RECT rc;
            GetUpdateRect( hWnd_, &rc, FALSE );
            RepaintBackground( &rc ); ///< Calls Repaint() inside

            isBgRepaintNeeded_ = false;
            isPaintInProgress_ = false;

            Repaint( true );
            return 0;
        }

        PAINTSTRUCT ps;
        HDC dc = BeginPaint( hWnd_, &ps );
        on_paint( dc, &ps.rcPaint );
        EndPaint( hWnd_, &ps );

        isPaintInProgress_ = false;
        return 0;
    }
    case WM_SIZE:
    {
        RECT rect;
        GetClientRect( hWnd_, &rect );
        on_size( rect.right - rect.left, rect.bottom - rect.top );
        if ( get_pseudo_transparent() )
        {
            PostMessage( hWnd_, static_cast<UINT>( InternalMessage::refresh_bg ), 0, 0 );
        }
        else
        {
            Repaint();
        }
        return 0;
    }
    case WM_GETMINMAXINFO:
    { // This message will be called before WM_CREATE as well,
        // but we don't need to handle it before panel creation,
        // since default values suit us just fine
        LPMINMAXINFO pmmi = reinterpret_cast<LPMINMAXINFO>( lp );
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

std::optional<LRESULT> js_panel_window::on_callback_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    std::shared_ptr<panel::CallBackDataBase> callbackData( *reinterpret_cast<std::shared_ptr<panel::CallBackDataBase>*>( wp ) );
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( static_cast<CallbackMessage>( msg ) )
    {
    case CallbackMessage::fb_item_focus_change:
    {
        on_item_focus_change( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_item_played:
    {
        on_item_played( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_library_items_added:
    {
        on_library_items_added( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_library_items_changed:
    {
        on_library_items_changed( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_library_items_removed:
    {
        on_library_items_removed( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_metadb_changed:
    {
        on_metadb_changed( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_playback_edited:
    {
        on_playback_edited( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_playback_new_track:
    {
        on_playback_new_track( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_playback_seek:
    {
        on_playback_seek( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_playback_time:
    {
        on_playback_time( callbackData->DataPtr() );
        return 0;
    }
    case CallbackMessage::fb_volume_change:
    {
        on_volume_change( callbackData->DataPtr() );
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

std::optional<LRESULT> js_panel_window::on_player_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( static_cast<PlayerMessage>( msg ) )
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
    case PlayerMessage::fb_playback_starting:
    {
        on_playback_starting( wp, lp );
        return 0;
    }
    case PlayerMessage::fb_playback_stop:
    {
        on_playback_stop( wp );
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
    case PlayerMessage::fb_playlist_items_removed:
    {
        on_playlist_items_removed( wp, lp );
        return 0;
    }
    case PlayerMessage::fb_playlist_items_reordered:
    {
        on_playlist_items_reordered( wp );
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
    case PlayerMessage::wnd_drag_drop:
    {
        on_drag_drop( lp );
        return 0;
    }
    case PlayerMessage::wnd_drag_enter:
    {
        on_drag_enter( lp );
        return 0;
    }
    case PlayerMessage::wnd_drag_leave:
    {
        on_drag_leave();
        return 0;
    }
    case PlayerMessage::wnd_drag_over:
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

std::optional<LRESULT> js_panel_window::on_internal_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if ( !pJsContainer_ )
    {
        return std::nullopt;
    }

    switch ( static_cast<InternalMessage>( msg ) )
    {
    case InternalMessage::main_menu_item:
    {
        on_main_menu( wp );
        return 0;
    }
    case InternalMessage::get_album_art_done:
    {
        on_get_album_art_done( lp );
        return 0;
    }
    case InternalMessage::load_image_done:
    {
        on_load_image_done( lp );
        return 0;
    }
    case InternalMessage::notify_data:
    {
        on_notify_data( wp, lp );
        return 0;
    }
    case InternalMessage::refresh_bg:
    {
        isBgRepaintNeeded_ = true;
        Repaint( true );
        return 0;
    }
    case InternalMessage::reload_script:
    {
        update_script();
        return 0;
    }
    case InternalMessage::script_error:
    {
        on_script_error();
        return 0;
    }
    case InternalMessage::terminate_script:
    {
        script_unload();
        return 0;
    }
    case InternalMessage::show_configure:
    {
        show_configure_popup( hWnd_ );
        return 0;
    }
    case InternalMessage::show_properties:
    {
        show_property_popup( hWnd_ );
        return 0;
    }
    case InternalMessage::update_size:
    {
        on_size( width_, height_ );
        if ( get_pseudo_transparent() )
        {
            PostMessage( hWnd_, static_cast<UINT>( InternalMessage::refresh_bg ), 0, 0 );
        }
        else
        {
            Repaint();
        }
        return 0;
    }
    case InternalMessage::timer_proc:
    {
        pJsContainer_->InvokeTimerFunction( static_cast<uint32_t>( wp ) );
        return 0;
    }
    default:
    {
        return std::nullopt;
    }
    }
}

bool js_panel_window::show_configure_popup( HWND parent )
{
    modal_dialog_scope scope;
    if ( !scope.can_create() )
        return false;
    scope.initialize( parent );

    CDialogConf dlg( this );
    return ( dlg.DoModal( parent ) == IDOK );
}

bool js_panel_window::show_property_popup( HWND parent )
{
    modal_dialog_scope scope;
    if ( !scope.can_create() )
        return false;
    scope.initialize( parent );

    CDialogProperty dlg( this );
    return ( dlg.DoModal( parent ) == IDOK );
}

void js_panel_window::build_context_menu( HMENU menu, int x, int y, int id_base )
{
    ::AppendMenu( menu, MF_STRING, id_base + 1, _T( "&Reload" ) );
    ::AppendMenu( menu, MF_SEPARATOR, 0, 0 );
    ::AppendMenu( menu, MF_STRING, id_base + 2, _T( "&Open component folder" ) );
    ::AppendMenu( menu, MF_SEPARATOR, 0, 0 );
    ::AppendMenu( menu, MF_STRING, id_base + 3, _T( "&Properties" ) );
    ::AppendMenu( menu, MF_STRING, id_base + 4, _T( "&Configure..." ) );
}

void js_panel_window::execute_context_menu_command( int id, int id_base )
{
    switch ( id - id_base )
    {
    case 1:
    {
        update_script();
        break;
    }
    case 2:
    {
        pfc::stringcvt::string_os_from_utf8 folder( helpers::get_fb2k_component_path() );
        ShellExecute( nullptr, _T( "open" ), folder, nullptr, nullptr, SW_SHOW );
    }
    break;
    case 3:
        show_property_popup( hWnd_ );
        break;
    case 4:
        show_configure_popup( hWnd_ );
        break;
    }
}

GUID js_panel_window::GetGUID()
{
    return get_config_guid();
}

HDC js_panel_window::GetHDC() const
{
    return hDc_;
}

HWND js_panel_window::GetHWND() const
{
    return hWnd_;
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

smp::PanelTooltipParam& js_panel_window::GetPanelTooltipParam()
{
    return panelTooltipParam_;
}

PanelInfo& js_panel_window::ScriptInfo()
{
    return m_script_info;
}

t_size& js_panel_window::DlgCode()
{
    return dlgCode_;
}

js_panel_window::PanelType js_panel_window::GetPanelType() const
{
    return panelType_;
}

void js_panel_window::Repaint( bool force /*= false */ )
{
    if ( force )
    {
        RedrawWindow( hWnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW );
    }
    else
    {
        InvalidateRect( hWnd_, nullptr, FALSE );
    }
}

void js_panel_window::RepaintRect( LONG x, LONG y, LONG w, LONG h, bool force /*= false */ )
{
    RECT rc;
    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;

    if ( force )
    {
        RedrawWindow( hWnd_, &rc, nullptr, RDW_INVALIDATE | RDW_UPDATENOW );
    }
    else
    {
        InvalidateRect( hWnd_, &rc, FALSE );
    }
}

void js_panel_window::RepaintBackground( LPRECT lprcUpdate /*= nullptr */ )
{
    HWND wnd_parent = GetAncestor( hWnd_, GA_PARENT );

    if ( !wnd_parent || IsIconic( core_api::get_main_window() ) || !IsWindowVisible( hWnd_ ) )
    {
        return;
    }

    HDC dc_parent = GetDC( wnd_parent );
    HDC hdc_bk = CreateCompatibleDC( dc_parent );
    POINT pt = { 0, 0 };
    RECT rect_child = { 0, 0, (LONG)width_, (LONG)height_ };
    RECT rect_parent;
    HRGN rgn_child = nullptr;

    // HACK: for Tab control
    // Find siblings
    HWND hwnd = nullptr;
    while ( hwnd = FindWindowEx( wnd_parent, hwnd, nullptr, nullptr ) )
    {
        TCHAR buff[64];
        if ( hwnd == hWnd_ )
        {
            continue;
        }
        GetClassName( hwnd, buff, _countof( buff ) );
        if ( _tcsstr( buff, _T( "SysTabControl32" ) ) )
        {
            wnd_parent = hwnd;
            break;
        }
    }

    if ( lprcUpdate )
    {
        HRGN rgn = CreateRectRgnIndirect( lprcUpdate );
        rgn_child = CreateRectRgnIndirect( &rect_child );
        CombineRgn( rgn_child, rgn_child, rgn, RGN_DIFF );
        DeleteRgn( rgn );
    }
    else
    {
        rgn_child = CreateRectRgn( 0, 0, 0, 0 );
    }

    ClientToScreen( hWnd_, &pt );
    ScreenToClient( wnd_parent, &pt );

    CopyRect( &rect_parent, &rect_child );
    ClientToScreen( hWnd_, (LPPOINT)&rect_parent );
    ClientToScreen( hWnd_, (LPPOINT)&rect_parent + 1 );
    ScreenToClient( wnd_parent, (LPPOINT)&rect_parent );
    ScreenToClient( wnd_parent, (LPPOINT)&rect_parent + 1 );

    // Force Repaint
    SetWindowRgn( hWnd_, rgn_child, FALSE );
    RedrawWindow( wnd_parent, &rect_parent, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW );

    // Background bitmap
    HBITMAP old_bmp = SelectBitmap( hdc_bk, hBitmapBg_ );

    // Paint BK
    BitBlt( hdc_bk, rect_child.left, rect_child.top, rect_child.right - rect_child.left, rect_child.bottom - rect_child.top, dc_parent, pt.x, pt.y, SRCCOPY );

    SelectBitmap( hdc_bk, old_bmp );
    DeleteDC( hdc_bk );
    ReleaseDC( wnd_parent, dc_parent );
    DeleteRgn( rgn_child );
    SetWindowRgn( hWnd_, nullptr, FALSE );
    if ( smp::config::EdgeStyle::NO_EDGE != get_edge_style() )
    {
        SendMessage( hWnd_, WM_NCPAINT, 1, 0 );
    }
}

uint32_t js_panel_window::SetInterval( JS::HandleFunction func, uint32_t delay )
{
    return pJsContainer_->SetInterval( hWnd_, delay, func );
}

uint32_t js_panel_window::SetTimeout( JS::HandleFunction func, uint32_t delay )
{
    return pJsContainer_->SetTimeout( hWnd_, delay, func );
}

void js_panel_window::ClearIntervalOrTimeout( uint32_t timerId )
{
    return pJsContainer_->KillTimer( timerId );
}

bool js_panel_window::script_load()
{
    pfc::hires_timer timer;
    timer.start();

    const auto extstyle = [&]() {
        DWORD extstyle = GetWindowLongPtr( hWnd_, GWL_EXSTYLE );
        extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
        extstyle |= edge_style_from_config( get_edge_style() );

        return extstyle;
    }();

    SetWindowLongPtr( hWnd_, GWL_EXSTYLE, extstyle );
    SetWindowPos( hWnd_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

    maxSize_ = { INT_MAX, INT_MAX };
    minSize_ = { 0, 0 };
    PostMessage( hWnd_, static_cast<UINT>( InternalMessage::size_limit_changed ), 0, uie::size_limit_all );

    if ( !pJsContainer_->Initialize() )
    { // error reporting handled inside
        return false;
    }

    if ( !pJsContainer_->ExecuteScript( get_script_code().c_str() ) )
    { // error reporting handled inside
        return false;
    }

    // HACK: Script update will not call on_size, so invoke it explicitly
    SendMessage( hWnd_, static_cast<UINT>( InternalMessage::update_size ), 0, 0 );

    FB2K_console_formatter() << SMP_NAME_WITH_VERSION " (" << ScriptInfo().build_info_string() << "): initialized in " << ( uint32_t )( timer.query() * 1000 ) << " ms";
    return true;
}

void js_panel_window::script_unload()
{
    pJsContainer_->InvokeJsCallback( "on_script_unload" );

    ScriptInfo().clear();
    selectionHolder_.release();
    pJsContainer_->Finalize();
}

void js_panel_window::create_context()
{
    delete_context();

    hBitmap_ = CreateCompatibleBitmap( hDc_, width_, height_ );

    if ( get_pseudo_transparent() )
    {
        hBitmapBg_ = CreateCompatibleBitmap( hDc_, width_, height_ );
    }
}

void js_panel_window::delete_context()
{
    if ( hBitmap_ )
    {
        DeleteBitmap( hBitmap_ );
        hBitmap_ = nullptr;
    }

    if ( hBitmapBg_ )
    {
        DeleteBitmap( hBitmapBg_ );
        hBitmapBg_ = nullptr;
    }
}

void js_panel_window::on_context_menu( int x, int y )
{
    const int base_id = 0;
    HMENU hMenu = CreatePopupMenu();
    mozjs::scope::final_action autoMenu( [hMenu] {
        DestroyMenu( hMenu );
    } );

    build_context_menu( hMenu, x, y, base_id );
    int ret = TrackPopupMenu( hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, hWnd_, 0 );
    execute_context_menu_command( ret, base_id );
}

void js_panel_window::on_erase_background()
{
    if ( get_pseudo_transparent() )
    {
        PostMessage( hWnd_, static_cast<UINT>( InternalMessage::refresh_bg ), 0, 0 );
    }
}

void js_panel_window::on_panel_create( HWND hWnd )
{
    RECT rect;
    hWnd_ = hWnd;
    hDc_ = GetDC( hWnd_ );
    GetClientRect( hWnd_, &rect );
    width_ = rect.right - rect.left;
    height_ = rect.bottom - rect.top;
    create_context();
    // Interfaces
    // TODO: check
    // m_gr_wrap.Attach( new com_object_impl_t<GdiGraphics>(), false );
    panel_manager::instance().add_window( hWnd_ );

    // TODO: add error handling
    pJsContainer_ = std::make_shared<mozjs::JsContainer>( *this );
    script_load();
}

void js_panel_window::on_panel_destroy()
{
    script_unload();
    pJsContainer_.reset();

    panel_manager::instance().remove_window( hWnd_ );

    // TODO: check
    //if ( m_gr_wrap )
    //{
    //m_gr_wrap.Release();
    //}
    delete_context();
    ReleaseDC( hWnd_, hDc_ );
}

void js_panel_window::on_script_error()
{
    auto& tooltip_param = GetPanelTooltipParam();
    if ( tooltip_param.hTooltip )
    {
        SendMessage( tooltip_param.hTooltip, TTM_ACTIVATE, FALSE, 0 );
    }

    Repaint();
    script_unload();
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
    auto actionParams = reinterpret_cast<mozjs::DropActionMessageParams*>( lp );
    pJsContainer_->InvokeOnDragAction( "on_drag_drop",
                                       actionParams->pt,
                                       actionParams->keyState,
                                       actionParams->actionParams );
}

void js_panel_window::on_drag_enter( LPARAM lp )
{
    auto actionParams = reinterpret_cast<mozjs::DropActionMessageParams*>( lp );
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
    auto actionParams = reinterpret_cast<mozjs::DropActionMessageParams*>( lp );
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

void js_panel_window::on_get_album_art_done( LPARAM lp )
{
    // Destroyed by task runner, no need to keep track
    auto param = reinterpret_cast<mozjs::art::AsyncArtTaskResult*>( lp );
    auto autoRet = pJsContainer_->InvokeJsCallback( "on_get_album_art_done",
                                                    static_cast<metadb_handle_ptr>( param->handle ),
                                                    static_cast<uint32_t>( param->artId ),
                                                    std::move( param->bitmap ),
                                                    static_cast<pfc::string8_fast>( param->imagePath ) );
}

void js_panel_window::on_item_focus_change( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<t_size, t_size, t_size>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_item_focus_change",
                                     static_cast<int32_t>( std::get<0>( data ) ),
                                     static_cast<int32_t>( std::get<1>( data ) ),
                                     static_cast<int32_t>( std::get<2>( data ) ) );
}

void js_panel_window::on_item_played( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_handle_ptr>*>( pData );
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

void js_panel_window::on_load_image_done( LPARAM lp )
{
    // Destroyed by task runner, no need to keep track
    auto param = reinterpret_cast<mozjs::image::AsyncImageTaskResult*>( lp );
    auto autoRet = pJsContainer_->InvokeJsCallback( "on_load_image_done",
                                                    param->taskId,
                                                    std::move( param->bitmap ),
                                                    static_cast<pfc::string8_fast>( param->imagePath ) );
}

void js_panel_window::on_library_items_added( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_callback_data>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_library_items_added",
                                     std::get<0>( data ).m_items );
}

void js_panel_window::on_library_items_changed( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_callback_data>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_library_items_changed",
                                     std::get<0>( data ).m_items );
}

void js_panel_window::on_library_items_removed( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_callback_data>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_library_items_removed",
                                     std::get<0>( data ).m_items );
}

void js_panel_window::on_main_menu( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_main_menu",
                                     static_cast<uint32_t>( wp ) );
}

void js_panel_window::on_metadb_changed( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_callback_data>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_metadb_changed",
                                     std::get<0>( data ).m_items,
                                     std::get<0>( data ).m_fromhook );
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
    }
}

void js_panel_window::on_mouse_button_down( UINT msg, WPARAM wp, LPARAM lp )
{
    if ( get_grab_focus() )
    {
        SetFocus( hWnd_ );
    }

    SetCapture( hWnd_ );

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
        _variant_t result;

        // Bypass the user code.
        if ( IsKeyPressed( VK_LSHIFT ) && IsKeyPressed( VK_LWIN ) )
        {
            break;
        }

        auto autoRet = pJsContainer_->InvokeJsCallback<bool>( "on_mouse_rbtn_up",
                                                              static_cast<int32_t>( GET_X_LPARAM( lp ) ),
                                                              static_cast<int32_t>( GET_Y_LPARAM( lp ) ),
                                                              static_cast<uint32_t>( wp ) );
        if ( autoRet )
        {
            ret = autoRet.value();
        }
        break;
    }
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
        TRACKMOUSEEVENT tme;

        tme.cbSize = sizeof( tme );
        tme.hwndTrack = hWnd_;
        tme.dwFlags = TME_LEAVE;
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

void js_panel_window::on_paint( HDC dc, LPRECT lpUpdateRect )
{
    if ( !dc || !lpUpdateRect || !hBitmap_ )
    {
        return;
    }

    HDC memdc = CreateCompatibleDC( dc );
    auto autoMemDc = mozjs::gdi::CreateUniquePtr( memdc );

    HBITMAP oldbmp = SelectBitmap( memdc, hBitmap_ );
    mozjs::scope::final_action autoBmp( [memdc, oldbmp] {
        SelectBitmap( memdc, oldbmp );
    } );

    if ( mozjs::JsContainer::JsStatus::EngineFailed == pJsContainer_->GetStatus()
         || mozjs::JsContainer::JsStatus::Failed == pJsContainer_->GetStatus() )
    {
        on_paint_error( memdc );
    }
    else
    {
        if ( get_pseudo_transparent() )
        {
            HDC bkdc = CreateCompatibleDC( dc );
            auto autoBkDc = mozjs::gdi::CreateUniquePtr( bkdc );

            HBITMAP bkoldbmp = SelectBitmap( bkdc, hBitmapBg_ );
            mozjs::scope::final_action autoBkBmp( [bkdc, bkoldbmp] {
                SelectBitmap( bkdc, bkoldbmp );
            } );

            BitBlt( memdc,
                    lpUpdateRect->left,
                    lpUpdateRect->top,
                    lpUpdateRect->right - lpUpdateRect->left,
                    lpUpdateRect->bottom - lpUpdateRect->top,
                    bkdc,
                    lpUpdateRect->left,
                    lpUpdateRect->top,
                    SRCCOPY );
        }
        else
        {
            RECT rc = { 0, 0, (LONG)width_, (LONG)height_ };
            FillRect( memdc, &rc, ( HBRUSH )( COLOR_WINDOW + 1 ) );
        }

        on_paint_user( memdc, lpUpdateRect );
    }

    BitBlt( dc, 0, 0, width_, height_, memdc, 0, 0, SRCCOPY );
}

void js_panel_window::on_paint_error( HDC memdc )
{
    HFONT newfont = CreateFont(
        20,
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
        _T( "Tahoma" ) );
    auto autoFont = mozjs::gdi::CreateUniquePtr( newfont );

    HFONT oldfont = (HFONT)SelectObject( memdc, newfont );
    mozjs::scope::final_action autoFontSelect( [memdc, oldfont]() {
        SelectObject( memdc, oldfont );
    } );

    LOGBRUSH lbBack = { BS_SOLID, RGB( 225, 60, 45 ), 0 };
    HBRUSH hBack = CreateBrushIndirect( &lbBack );
    auto autoHBack = mozjs::gdi::CreateUniquePtr( hBack );

    RECT rc = { 0, 0, (LONG)width_, (LONG)height_ };
    FillRect( memdc, &rc, hBack );
    SetBkMode( memdc, TRANSPARENT );

    SetTextColor( memdc, RGB( 255, 255, 255 ) );
    DrawText( memdc, L"Aw, crashed :(", -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );
}

void js_panel_window::on_paint_user( HDC memdc, LPRECT lpUpdateRect )
{
    // Prepare graphics object to the script.
    Gdiplus::Graphics gr( memdc );
    Gdiplus::Rect rect( lpUpdateRect->left, lpUpdateRect->top, lpUpdateRect->right - lpUpdateRect->left, lpUpdateRect->bottom - lpUpdateRect->top );

    // SetClip() may improve performance slightly
    gr.SetClip( rect );

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

void js_panel_window::on_playback_edited( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_handle_ptr>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_playback_edited",
                                     std::get<0>( data ) );
}

void js_panel_window::on_playback_follow_cursor_changed( WPARAM wp )
{
    pJsContainer_->InvokeJsCallback( "on_playback_follow_cursor_changed",
                                     static_cast<bool>( wp ) );
}

void js_panel_window::on_playback_new_track( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<metadb_handle_ptr>*>( pData );
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

void js_panel_window::on_playback_seek( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<double>*>( pData );
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

void js_panel_window::on_playback_time( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<double>*>( pData );
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

    delete_context();
    create_context();

    pJsContainer_->InvokeJsCallback( "on_size",
                                     static_cast<uint32_t>( w ),
                                     static_cast<uint32_t>( h ) );
}

void js_panel_window::on_volume_change( void* pData )
{
    auto& data = *reinterpret_cast<std::tuple<float>*>( pData );
    pJsContainer_->InvokeJsCallback( "on_volume_change",
                                     std::get<0>( data ) );
}
