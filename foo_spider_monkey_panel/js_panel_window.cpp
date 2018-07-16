#include "stdafx.h"
#include "js_panel_window.h"
#include "ui_conf.h"
#include "ui_property.h"
#include "panel_manager.h"
#include "popup_msg.h"

#include <js_engine/js_engine.h>
#include <js_engine/native_to_js_invoker.h>
#include <js_objects/fb_tooltip.h>
#include <js_objects/gdi_graphics.h>
#include <js_utils/art_helper.h>
#include <js_utils/image_helper.h>


js_panel_window::js_panel_window( PanelType instanceType )
    : panelType_( instanceType )
    , m_script_info( get_config_guid() )
{
}

js_panel_window::~js_panel_window()
{
}

void js_panel_window::update_script( const char* code )
{
    if ( code )
    {
        get_script_code() = code;
    }

    script_unload();
    script_load();
}

void js_panel_window::JsEngineFail( const pfc::string8_fast& errorText )
{
    popup_msg::g_show( errorText, JSP_NAME );
    MessageBeep( MB_ICONASTERISK );

    SendMessage( hWnd_, UWM_SCRIPT_ERROR, 0, 0 );
}

LRESULT js_panel_window::on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
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
        if ( isPaintSuppressed_ )
        {
            break;
        }

        if ( get_pseudo_transparent() && !isPaintPending_ )
        {
            RECT rc;

            GetUpdateRect( hWnd_, &rc, FALSE );
            RefreshBackground( &rc );
            return 0;
        }

        PAINTSTRUCT ps;
        HDC dc = BeginPaint( hWnd_, &ps );
        on_paint( dc, &ps.rcPaint );
        EndPaint( hWnd_, &ps );
        isPaintPending_ = false;

        return 0;
    }
    case WM_SIZE:
    {
        RECT rect;
        GetClientRect( hWnd_, &rect );
        on_size( rect.right - rect.left, rect.bottom - rect.top );
        if ( get_pseudo_transparent() )
        {
            PostMessage( hWnd_, UWM_REFRESHBK, 0, 0 );
        }
        else
        {
            Repaint();
        }
        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO pmmi = reinterpret_cast<LPMINMAXINFO>(lp);
        memcpy( &pmmi->ptMaxTrackSize, &MaxSize(), sizeof( POINT ) );
        memcpy( &pmmi->ptMinTrackSize, &MinSize(), sizeof( POINT ) );
    }
    return 0;

    case WM_GETDLGCODE:
    {
        return DlgCode();
    }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        on_mouse_button_down( msg, wp, lp );
        break;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    {
        if ( on_mouse_button_up( msg, wp, lp ) )
        {
            return 0;
        }
        break;
    }
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    {
        on_mouse_button_dblclk( msg, wp, lp );
        break;
    }
    case WM_CONTEXTMENU:
    {
        on_context_menu( GET_X_LPARAM( lp ), GET_Y_LPARAM( lp ) );
        return 1;
    }
    case WM_MOUSEMOVE:
    {
        on_mouse_move( wp, lp );
        break;
    }
    case WM_MOUSELEAVE:
    {
        on_mouse_leave();
        break;
    }
    case WM_MOUSEWHEEL:
    {
        on_mouse_wheel( wp );
        break;
    }
    case WM_MOUSEHWHEEL:
    {
        on_mouse_wheel_h( wp );
        break;
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
        on_focus_changed( true );
        break;
    }
    case WM_KILLFOCUS:
    {
        on_focus_changed( false );
        break;
    }
    case CALLBACK_UWM_ON_ALWAYS_ON_TOP_CHANGED:
    {
        on_always_on_top_changed( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_COLOURS_CHANGED:
    {
        on_colours_changed();
        return 0;
    }
    case CALLBACK_UWM_ON_CURSOR_FOLLOW_PLAYBACK_CHANGED:
    {
        on_cursor_follow_playback_changed( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_DSP_PRESET_CHANGED:
    {
        on_dsp_preset_changed();
        return 0;
    }
    case CALLBACK_UWM_ON_FONT_CHANGED:
    {
        on_font_changed();
        return 0;
    }
    case CALLBACK_UWM_ON_GET_ALBUM_ART_DONE:
    {
        on_get_album_art_done( lp );
        return 0;
    }
    case CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE:
    {
        on_item_focus_change( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_ITEM_PLAYED:
    {
        on_item_played( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_LIBRARY_ITEMS_ADDED:
    {
        on_library_items_added( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_LIBRARY_ITEMS_CHANGED:
    {
        on_library_items_changed( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_LIBRARY_ITEMS_REMOVED:
    {
        on_library_items_removed( wp );
        return 0;
    }
    case CALLBACK_UWM_ON_LOAD_IMAGE_DONE:
    {
        on_load_image_done( lp );
        return 0;
    }
    case CALLBACK_UWM_ON_MAIN_MENU:
        on_main_menu( wp );
        return 0;

    case CALLBACK_UWM_ON_METADB_CHANGED:
        on_metadb_changed( wp );
        return 0;

    case CALLBACK_UWM_ON_NOTIFY_DATA:
        on_notify_data( wp );
        return 0;

    case CALLBACK_UWM_ON_OUTPUT_DEVICE_CHANGED:
        on_output_device_changed();
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO:
        on_playback_dynamic_info();
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK:
        on_playback_dynamic_info_track();
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_EDITED:
        on_playback_edited( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_FOLLOW_CURSOR_CHANGED:
        on_playback_follow_cursor_changed( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK:
        on_playback_new_track( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED:
        on_playback_order_changed( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_PAUSE:
        on_playback_pause( wp != 0 );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_QUEUE_CHANGED:
        on_playback_queue_changed( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_SEEK:
        on_playback_seek( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_STARTING:
        on_playback_starting( (playback_control::t_track_command)wp, lp != 0 );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_STOP:
        on_playback_stop( (playback_control::t_stop_reason)wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_TIME:
        on_playback_time( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_ITEM_ENSURE_VISIBLE:
        on_playlist_item_ensure_visible( wp, lp );
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_ITEMS_ADDED:
        on_playlist_items_added( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_ITEMS_REMOVED:
        on_playlist_items_removed( wp, lp );
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_ITEMS_REORDERED:
        on_playlist_items_reordered( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_ITEMS_SELECTION_CHANGE:
        on_playlist_items_selection_change();
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_STOP_AFTER_CURRENT_CHANGED:
        on_playlist_stop_after_current_changed( wp );
        return 0;

    case CALLBACK_UWM_ON_PLAYLIST_SWITCH:
        on_playlist_switch();
        return 0;

    case CALLBACK_UWM_ON_PLAYLISTS_CHANGED:
        on_playlists_changed();
        return 0;

    case CALLBACK_UWM_ON_REPLAYGAIN_MODE_CHANGED:
        on_replaygain_mode_changed( wp );
        return 0;

    case CALLBACK_UWM_ON_SELECTION_CHANGED:
        on_selection_changed();
        return 0;

    case CALLBACK_UWM_ON_VOLUME_CHANGE:
        on_volume_change( wp );
        return 0;

    case UWM_REFRESHBK:
        Repaint(true);
        return 0;

    case UWM_RELOAD:
        update_script();
        return 0;

    case UWM_SCRIPT_ERROR:
    {
        on_script_error();
        return 0;
    }

    case UWM_SCRIPT_TERM:
        script_unload();
        return 0;

    case UWM_SHOW_CONFIGURE:
        show_configure_popup( hWnd_ );
        return 0;

    case UWM_SHOW_PROPERTIES:
        show_property_popup( hWnd_ );
        return 0;

    case UWM_SIZE:
        on_size( width_, height_ );
        if ( get_pseudo_transparent() )
        {
            PostMessage( hWnd_, UWM_REFRESHBK, 0, 0 );
        }
        else
        {
            Repaint();
        }
        return 0;

    case UWM_TIMER:
        jsContainer_.InvokeTimerFunction( static_cast<uint32_t>( wp ) );
        return 0;
    }

    return uDefWindowProc( hwnd, msg, wp, lp );
}

bool js_panel_window::show_configure_popup( HWND parent )
{
    modal_dialog_scope scope;
    if ( !scope.can_create() ) return false;
    scope.initialize( parent );

    CDialogConf dlg( this );
    return ( dlg.DoModal( parent ) == IDOK );
}

bool js_panel_window::show_property_popup( HWND parent )
{
    modal_dialog_scope scope;
    if ( !scope.can_create() ) return false;
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

t_script_info& js_panel_window::ScriptInfo()
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
        isPaintPending_ = false;
        RedrawWindow( hWnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW );
    }
    else
    {
        isPaintPending_ = true;
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
        isPaintPending_ = false;
        RedrawWindow( hWnd_, &rc, nullptr, RDW_INVALIDATE | RDW_UPDATENOW );
    }
    else
    {
        isPaintPending_ = true;
        InvalidateRect( hWnd_, &rc, FALSE );
    }
}

void js_panel_window::RefreshBackground( LPRECT lprcUpdate /*= nullptr */ )
{
    HWND wnd_parent = GetAncestor( hWnd_, GA_PARENT );

    if ( !wnd_parent || IsIconic( core_api::get_main_window() ) || !IsWindowVisible( hWnd_ ) )
        return;

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
    isPaintSuppressed_ = true;
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
    isPaintSuppressed_ = false;
    if ( get_edge_style() )
    {
        SendMessage( hWnd_, WM_NCPAINT, 1, 0 );
    }
    Repaint( true );
}

uint32_t js_panel_window::SetInterval( JS::HandleFunction func, uint32_t delay )
{
    return jsContainer_.SetInterval( hWnd_, delay, func );
}

uint32_t js_panel_window::SetTimeout( JS::HandleFunction func, uint32_t delay )
{
    return jsContainer_.SetTimeout( hWnd_, delay, func );
}

void js_panel_window::ClearIntervalOrTimeout( uint32_t timerId )
{
    return jsContainer_.KillTimer( timerId );
}

bool js_panel_window::script_load()
{
    pfc::hires_timer timer;
    timer.start();

    DWORD extstyle = GetWindowLongPtr( hWnd_, GWL_EXSTYLE );
    extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
    extstyle |= edge_style_from_config( get_edge_style() );
    SetWindowLongPtr( hWnd_, GWL_EXSTYLE, extstyle );
    SetWindowPos( hWnd_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

    maxSize_ = { INT_MAX, INT_MAX };
    minSize_ = { 0, 0 };
    PostMessage( hWnd_, UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_all );

    if ( !jsContainer_.Initialize() )
    {
        return false;
    }
    
    if ( !jsContainer_.ExecuteScript( get_script_code().c_str() ) )
    {
        return false;
    }

    // TODO: check
    //HRESULT hr = m_script_host->Initialize();

    if ( ScriptInfo().feature_mask & t_script_info::kFeatureDragDrop )
    {
        m_drop_target.Attach( new com_object_impl_t<HostDropTarget>( hWnd_, &jsContainer_ ) );
        m_drop_target->RegisterDragDrop();
        isDropTargetRegistered_ = true;
    }

    // HACK: Script update will not call on_size, so invoke it explicitly
    SendMessage( hWnd_, UWM_SIZE, 0, 0 );

    FB2K_console_formatter() << JSP_NAME " v" JSP_VERSION " (" << ScriptInfo().build_info_string() << "): initialized in " << (uint32_t)( timer.query() * 1000 ) << " ms";
    return true;
}

void js_panel_window::script_unload()
{
    // TODO: check
    //m_script_host->Finalize();

    if ( isDropTargetRegistered_ )
    {
        m_drop_target->RevokeDragDrop();
        isDropTargetRegistered_ = false;
    }

    selectionHolder_.release();

    jsContainer_.Finalize();
}

ui_helpers::container_window::class_data& js_panel_window::get_class_data() const
{
    static class_data my_class_data =
    {
        _T( JSP_WINDOW_CLASS_NAME ),
        _T( "" ),
        0,
        false,
        false,
        0,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        edge_style_from_config( get_edge_style() ),
        CS_DBLCLKS,
        true, true, true, IDC_ARROW
    };

    return my_class_data;
}

void js_panel_window::create_context()
{
    if ( hBitmap_ || hBitmapBg_ )
    {
        delete_context();
    }

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
    HMENU menu = CreatePopupMenu();
    int ret = 0;

    build_context_menu( menu, x, y, base_id );
    ret = TrackPopupMenu( menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, hWnd_, 0 );
    execute_context_menu_command( ret, base_id );
    DestroyMenu( menu );
}

void js_panel_window::on_erase_background()
{
    if ( get_pseudo_transparent() )
    {
        PostMessage( hWnd_, UWM_REFRESHBK, 0, 0 );
    }
}

void js_panel_window::on_panel_create(HWND hWnd)
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

    mozjs::JsEngine::GetInstance().RegisterPanel( *this, jsContainer_ );
    script_load();
}

void js_panel_window::on_panel_destroy()
{
    script_unload();
    mozjs::JsEngine::GetInstance().UnregisterPanel( *this );

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
    jsContainer_.InvokeJsCallback( "on_always_on_top_changed",
                                   static_cast<bool>(wp) );
}

void js_panel_window::on_char( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_char",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_colours_changed()
{
    jsContainer_.InvokeJsCallback( "on_colours_changed" );
}

void js_panel_window::on_cursor_follow_playback_changed( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_cursor_follow_playback_changed",
                                   static_cast<bool>(wp) );
}

void js_panel_window::on_dsp_preset_changed()
{
    jsContainer_.InvokeJsCallback( "on_dsp_preset_changed" );
}

void js_panel_window::on_focus_changed( bool isFocused )
{
    if ( isFocused )
    {
        selectionHolder_ = ui_selection_manager::get()->acquire();
    }
    else
    {
        selectionHolder_.release();
    }
    jsContainer_.InvokeJsCallback( "on_focus_changed",
                                   static_cast<bool>(isFocused) );
}

void js_panel_window::on_font_changed()
{
    jsContainer_.InvokeJsCallback( "on_font_changed" );
}

void js_panel_window::on_get_album_art_done( LPARAM lp )
{
    // Destroyed by task runner, no need to keep track
    auto param = reinterpret_cast<mozjs::art::AsyncArtTaskResult*>(lp);
    auto autoRet = jsContainer_.InvokeJsCallback( "on_get_album_art_done",
                                                  static_cast<metadb_handle_ptr>(param->handle),
                                                  static_cast<uint32_t>(param->artId),
                                                  std::move(param->bitmap),
                                                  static_cast<pfc::string8_fast>(param->imagePath) );
}

void js_panel_window::on_item_focus_change( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data_3<t_size, t_size, t_size>> data( wp );
    jsContainer_.InvokeJsCallback( "on_item_focus_change",
                                   static_cast<int32_t>(data->m_item1),
                                   static_cast<int32_t>(data->m_item2),
                                   static_cast<int32_t>(data->m_item3) );
}

void js_panel_window::on_item_played( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr>> data( wp );
    jsContainer_.InvokeJsCallback( "on_item_played",
                                   static_cast<metadb_handle_ptr>(data->m_item) );
}

void js_panel_window::on_key_down( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_key_down",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_key_up( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_key_up",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_load_image_done( LPARAM lp )
{
    // Destroyed by task runner, no need to keep track
    auto param = reinterpret_cast<mozjs::image::AsyncImageTaskResult*>(lp);
    auto autoRet = jsContainer_.InvokeJsCallback( "on_load_image_done",
                                                  std::move( param->bitmap ),
                                                  static_cast<pfc::string8_fast>(param->imagePath) );
}

void js_panel_window::on_library_items_added( WPARAM wp )
{
    simple_callback_data_scope_releaser<t_on_data> data( wp );
    jsContainer_.InvokeJsCallback( "on_library_items_added",
                                   static_cast<metadb_handle_list>(data->m_items) );
}

void js_panel_window::on_library_items_changed( WPARAM wp )
{
    simple_callback_data_scope_releaser<t_on_data> data( wp );
    jsContainer_.InvokeJsCallback( "on_library_items_changed",
                                   static_cast<metadb_handle_list>(data->m_items) );
}

void js_panel_window::on_library_items_removed( WPARAM wp )
{
    simple_callback_data_scope_releaser<t_on_data> data( wp );
    jsContainer_.InvokeJsCallback( "on_library_items_removed",
                                   static_cast<metadb_handle_list>(data->m_items) );
}

void js_panel_window::on_main_menu( WPARAM wp )
{
    simple_callback_data_scope_releaser<t_on_data> data( wp );
    jsContainer_.InvokeJsCallback( "on_main_menu",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_metadb_changed( WPARAM wp )
{
    simple_callback_data_scope_releaser<t_on_data> data( wp );
    jsContainer_.InvokeJsCallback( "on_metadb_changed",
                                   static_cast<metadb_handle_list>(data->m_items),
                                   static_cast<bool>(data->m_fromhook) );
}

void js_panel_window::on_mouse_button_dblclk( UINT msg, WPARAM wp, LPARAM lp )
{
    switch ( msg )
    {
    case WM_LBUTTONDBLCLK:
    {
        jsContainer_.InvokeJsCallback( "on_mouse_lbtn_dblclk",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
        break;
    }

    case WM_MBUTTONDBLCLK:
    {
        jsContainer_.InvokeJsCallback( "on_mouse_mbtn_dblclk",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
        break;
    }

    case WM_RBUTTONDBLCLK:
    {
        jsContainer_.InvokeJsCallback( "on_mouse_rbtn_dblclk",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
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
        jsContainer_.InvokeJsCallback( "on_mouse_lbtn_down",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
        break;
    }
    case WM_MBUTTONDOWN:
    {
        jsContainer_.InvokeJsCallback( "on_mouse_mbtn_down",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
        break;
    }
    case WM_RBUTTONDOWN:
    {
        jsContainer_.InvokeJsCallback( "on_mouse_rbtn_down",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
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
        jsContainer_.InvokeJsCallback( "on_mouse_lbtn_up",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
        break;
    }
    case WM_MBUTTONUP:
    {
        jsContainer_.InvokeJsCallback( "on_mouse_mbtn_up",
                                       static_cast<int32_t>(GET_X_LPARAM( lp )),
                                       static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                       static_cast<uint32_t>(wp) );
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

        auto autoRet = jsContainer_.InvokeJsCallback<bool>( "on_mouse_rbtn_up",
                                                            static_cast<int32_t>(GET_X_LPARAM( lp )),
                                                            static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                                            static_cast<uint32_t>(wp) );
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

    jsContainer_.InvokeJsCallback( "on_mouse_leave" );

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

    jsContainer_.InvokeJsCallback( "on_mouse_move",
                                   static_cast<int32_t>(GET_X_LPARAM( lp )),
                                   static_cast<int32_t>(GET_Y_LPARAM( lp )),
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_mouse_wheel( WPARAM wp )
{// TODO: missing param doc
    jsContainer_.InvokeJsCallback( "on_mouse_wheel",
                                   static_cast<int8_t>(GET_WHEEL_DELTA_WPARAM( wp ) > 0 ? 1 : -1),
                                   static_cast<int32_t>(GET_WHEEL_DELTA_WPARAM( wp )),
                                   static_cast<int32_t>(WHEEL_DELTA) );
}

void js_panel_window::on_mouse_wheel_h( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_mouse_wheel_h",
                                   static_cast<int8_t>(GET_WHEEL_DELTA_WPARAM( wp ) > 0 ? 1 : -1) );
}

void js_panel_window::on_notify_data( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data_2<std::wstring, std::wstring>> data( wp );
    jsContainer_.InvokeOnNotifyCallback( data->m_item1, data->m_item2 );
}

void js_panel_window::on_output_device_changed()
{
    jsContainer_.InvokeJsCallback( "on_output_device_changed" );
}

void js_panel_window::on_paint( HDC dc, LPRECT lpUpdateRect )
{
    if ( !dc || !lpUpdateRect || !hBitmap_ ) return;

    HDC memdc = CreateCompatibleDC( dc );
    HBITMAP oldbmp = SelectBitmap( memdc, hBitmap_ );

    if ( mozjs::JsContainer::JsStatus::Failed == jsContainer_.GetStatus() )
    {
        on_paint_error( memdc );
    }
    else
    {
        if ( get_pseudo_transparent() )
        {
            HDC bkdc = CreateCompatibleDC( dc );
            HBITMAP bkoldbmp = SelectBitmap( bkdc, hBitmapBg_ );

            BitBlt(
                memdc,
                lpUpdateRect->left,
                lpUpdateRect->top,
                lpUpdateRect->right - lpUpdateRect->left,
                lpUpdateRect->bottom - lpUpdateRect->top,
                bkdc,
                lpUpdateRect->left,
                lpUpdateRect->top,
                SRCCOPY );

            SelectBitmap( bkdc, bkoldbmp );
            DeleteDC( bkdc );
        }
        else
        {
            RECT rc = { 0, 0, (LONG)width_, (LONG)height_ };

            FillRect( memdc, &rc, (HBRUSH)( COLOR_WINDOW + 1 ) );
        }

        on_paint_user( memdc, lpUpdateRect );
    }

    BitBlt( dc, 0, 0, width_, height_, memdc, 0, 0, SRCCOPY );
    SelectBitmap( memdc, oldbmp );
    DeleteDC( memdc );
}

void js_panel_window::on_paint_error( HDC memdc )
{
    const std::wstring errmsg( L"Aw, crashed :(" );
    RECT rc = { 0, 0, (LONG)width_, (LONG)height_ };
    SIZE sz = { 0 };

    // Font chosing
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

    HFONT oldfont = (HFONT)SelectObject( memdc, newfont );

    // Font drawing
    {
        LOGBRUSH lbBack = { BS_SOLID, RGB( 225, 60, 45 ), 0 };
        HBRUSH hBack = CreateBrushIndirect( &lbBack );

        FillRect( memdc, &rc, hBack );
        SetBkMode( memdc, TRANSPARENT );

        SetTextColor( memdc, RGB( 255, 255, 255 ) );
        DrawText( memdc, errmsg.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );

        DeleteObject( hBack );
    }

    SelectObject( memdc, oldfont );
}

void js_panel_window::on_paint_user( HDC memdc, LPRECT lpUpdateRect )
{
    if ( mozjs::JsContainer::JsStatus::Ready != jsContainer_.GetStatus() )
    {
        return;
    }

    // Prepare graphics object to the script.
    Gdiplus::Graphics gr( memdc );
    Gdiplus::Rect rect( lpUpdateRect->left, lpUpdateRect->top, lpUpdateRect->right - lpUpdateRect->left, lpUpdateRect->bottom - lpUpdateRect->top );

    // SetClip() may improve performance slightly
    gr.SetClip( rect );

    jsContainer_.InvokeOnPaintCallback( gr );
}

void js_panel_window::on_playback_dynamic_info()
{
    jsContainer_.InvokeJsCallback( "on_playback_dynamic_info" );
}

void js_panel_window::on_playback_dynamic_info_track()
{
    jsContainer_.InvokeJsCallback( "on_playback_dynamic_info_track" );
}

void js_panel_window::on_playback_edited( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr>> data( wp );
    jsContainer_.InvokeJsCallback( "on_playback_edited",
                                   static_cast<metadb_handle_ptr&>(data->m_item) );
}

void js_panel_window::on_playback_follow_cursor_changed( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_playback_follow_cursor_changed",
                                   static_cast<bool>(wp) );
}

void js_panel_window::on_playback_new_track( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr>> data( wp );
    jsContainer_.InvokeJsCallback( "on_playback_new_track",
                                   static_cast<metadb_handle_ptr&>(data->m_item) );
}

void js_panel_window::on_playback_order_changed( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_playback_order_changed",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_playback_pause( bool state )
{
    jsContainer_.InvokeJsCallback( "on_playback_pause",
                                   static_cast<bool>(state) );
}

void js_panel_window::on_playback_queue_changed( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_playback_queue_changed",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_playback_seek( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data<double>> data( wp );
    jsContainer_.InvokeJsCallback( "on_playback_seek",
                                   static_cast<double>(data->m_item) );
}

void js_panel_window::on_playback_starting( play_control::t_track_command cmd, bool paused )
{
    jsContainer_.InvokeJsCallback( "on_playback_starting",
                                   static_cast<uint32_t>(cmd) ,
                                   static_cast<bool>(paused) );
}

void js_panel_window::on_playback_stop( play_control::t_stop_reason reason )
{
    jsContainer_.InvokeJsCallback( "on_playback_stop",
                                   static_cast<uint32_t>(reason));
}

void js_panel_window::on_playback_time( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data<double>> data( wp );
    jsContainer_.InvokeJsCallback( "on_playback_time",
                                   static_cast<double>(data->m_item) );
}

void js_panel_window::on_playlist_item_ensure_visible( WPARAM wp, LPARAM lp )
{
    jsContainer_.InvokeJsCallback( "on_playlist_item_ensure_visible",
                                   static_cast<uint32_t>(wp),
                                   static_cast<uint32_t>(lp) );
}

void js_panel_window::on_playlist_items_added( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_playlist_items_added",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_playlist_items_removed( WPARAM wp, LPARAM lp )
{
    jsContainer_.InvokeJsCallback( "on_playlist_items_removed",
                                   static_cast<uint32_t>(wp),
                                   static_cast<uint32_t>(lp) );
}

void js_panel_window::on_playlist_items_reordered( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_playlist_items_reordered",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_playlist_items_selection_change()
{
    jsContainer_.InvokeJsCallback( "on_playlist_items_selection_change" );
}

void js_panel_window::on_playlist_stop_after_current_changed( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_playlist_stop_after_current_changed",
                                   static_cast<bool>(wp) );
}

void js_panel_window::on_playlist_switch()
{
    jsContainer_.InvokeJsCallback( "on_playlist_switch" );
}

void js_panel_window::on_playlists_changed()
{
    jsContainer_.InvokeJsCallback( "on_playlists_changed" );
}

void js_panel_window::on_replaygain_mode_changed( WPARAM wp )
{
    jsContainer_.InvokeJsCallback( "on_replaygain_mode_changed",
                                   static_cast<uint32_t>(wp) );
}

void js_panel_window::on_selection_changed()
{
    jsContainer_.InvokeJsCallback( "on_selection_changed" );
}

void js_panel_window::on_size( uint32_t w, uint32_t h )
{
    width_ = w;
    height_ = h;

    delete_context();
    create_context();

    jsContainer_.InvokeJsCallback( "on_size",
                                   static_cast<uint32_t>(w),
                                   static_cast<uint32_t>(h) );
}

void js_panel_window::on_volume_change( WPARAM wp )
{
    simple_callback_data_scope_releaser<simple_callback_data<float>> data( wp );
    jsContainer_.InvokeJsCallback( "on_volume_change",
                                   static_cast<float>(data->m_item) );
}
