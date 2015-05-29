#include "stdafx.h"
#include "wsh_panel_window.h"
#include "host.h"
#include "host_droptarget.h"
#include "ui_conf.h"
#include "ui_property.h"
#include "panel_manager.h"
#include "popup_msg.h"


wsh_panel_window::wsh_panel_window() :
	m_script_host(new ScriptHost(this)), 
	m_is_mouse_tracked(false),
	m_is_droptarget_registered(false)
{

}

wsh_panel_window::~wsh_panel_window()
{
	m_script_host->Release();
}

void wsh_panel_window::update_script(const char * name /*= NULL*/, const char * code /*= NULL*/)
{
	if (name && code)
	{
		get_script_engine() = name;
		get_script_code() = code;
	}

	script_unload();
	script_load();
}

bool wsh_panel_window::script_load()
{
	TRACK_FUNCTION();

	m_host_timer_dispatcher.setWindow(m_hwnd);

	helpers::mm_timer timer;
	bool result = true;
	timer.start();

	// Set window edge
	{
		DWORD extstyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);

		// Exclude all edge style
		extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
		extstyle |= edge_style_from_config(get_edge_style());
		SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, extstyle);
		SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	// Set something to default
	m_max_size.x = INT_MAX;
	m_max_size.y = INT_MAX;
	m_min_size.x = 0;
	m_min_size.x = 0;
	PostMessage(m_hwnd, UWM_SIZELIMITECHANGED, 0, uie::size_limit_all);

	if (get_disabled_before())
	{
		PostMessage(m_hwnd, UWM_SCRIPT_DISABLED_BEFORE, 0, 0);
		return false;
	}

	HRESULT hr = m_script_host->Initialize();

	if (FAILED(hr))
	{
		result = false;
	}
	else
	{
		if (ScriptInfo().feature_mask & t_script_info::kFeatureDragDrop)
		{
			// Ole Drag and Drop support
			m_drop_target.Attach(new com_object_impl_t<HostDropTarget>(this));
			m_drop_target->RegisterDragDrop();
			m_is_droptarget_registered = true;
		}

		// HACK: Script update will not call on_size, so invoke it explicitly
		SendMessage(m_hwnd, UWM_SIZE, 0, 0);

		// Show init message
		console::formatter() << WSPM_NAME " (" 
			<< ScriptInfo().build_info_string()
			<< "): initialized in "
			<< (int)(timer.query() * 1000)
			<< " ms";
	}

	return result;
}

void wsh_panel_window::script_unload()
{
	m_script_host->Finalize();

	if (m_is_droptarget_registered)
	{
		m_drop_target->RevokeDragDrop();
		m_is_droptarget_registered = false;
	}

	m_host_timer_dispatcher.reset();
	m_watched_handle.release();
	m_selection_holder.release();
}

void wsh_panel_window::create_context()
{
	if (m_gr_bmp || m_gr_bmp_bk)
		delete_context();

	m_gr_bmp = CreateCompatibleBitmap(m_hdc, m_width, m_height);

	if (get_pseudo_transparent())
	{
		m_gr_bmp_bk = CreateCompatibleBitmap(m_hdc, m_width, m_height);
	}
}

void wsh_panel_window::delete_context()
{
	if (m_gr_bmp)
	{
		DeleteBitmap(m_gr_bmp);
		m_gr_bmp = NULL;
	}

	if (m_gr_bmp_bk)
	{
		DeleteBitmap(m_gr_bmp_bk);
		m_gr_bmp_bk = NULL;
	}
}

ui_helpers::container_window::class_data & wsh_panel_window::get_class_data() const
{
	static ui_helpers::container_window::class_data my_class_data =
	{
		_T(WSPM_WINDOW_CLASS_NAME),
		_T(""), 
		0, 
		false, 
		false, 
		0, 
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
		edge_style_from_config(get_edge_style()),
		CS_DBLCLKS,
		true, true, true, IDC_ARROW
	};

	return my_class_data;
}

bool wsh_panel_window::show_configure_popup(HWND parent)
{
	modal_dialog_scope scope;
	if (!scope.can_create()) return false;
	scope.initialize(parent);

	CDialogConf dlg(this);
	return (dlg.DoModal(parent) == IDOK);
}

bool wsh_panel_window::show_property_popup(HWND parent)
{
	modal_dialog_scope scope;
	if (!scope.can_create()) return false;
	scope.initialize(parent);

	CDialogProperty dlg(this);
	return (dlg.DoModal(parent) == IDOK);
}

LRESULT wsh_panel_window::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	//TRACK_CALL_TEXT_FORMAT("uie_win::on_message(hwnd=0x%x,msg=%u,wp=%d,lp=%d)", m_hwnd, msg, wp, lp);

	switch (msg)
	{
	case WM_CREATE:
		{
			RECT rect;
			m_hwnd = hwnd;
			m_hdc = GetDC(m_hwnd);
			GetClientRect(m_hwnd, &rect);
			m_width  = rect.right - rect.left;
			m_height = rect.bottom - rect.top;
			create_context();
			// Interfaces
			m_gr_wrap.Attach(new com_object_impl_t<GdiGraphics>(), false);
			panel_manager::instance().add_window(m_hwnd);
			if (get_delay_load())
				delay_loader::g_enqueue(new delay_script_init_action(m_hwnd));
			else
				script_load();
		}
		return 0;

	case WM_DESTROY:
		script_unload();
		panel_manager::instance().remove_window(m_hwnd);
		if (m_gr_wrap)
			m_gr_wrap.Release();
		delete_context();
		ReleaseDC(m_hwnd, m_hdc);
		return 0;

	case UWM_SCRIPT_INIT:
		script_load();
		return 0;

	case UWM_SCRIPT_TERM:
		script_unload();
		return 0;

	case UWM_RELOAD:
	case WM_DISPLAYCHANGE:
	case WM_THEMECHANGED:
		update_script();
		return 0;

	case WM_ERASEBKGND:
		if (get_pseudo_transparent())
			PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
		return 1;

	case UWM_REFRESHBK:
		Redraw();
		return 0;

	case UWM_REFRESHBKDONE:
		on_refresh_background_done();
		return 0;

	case WM_PAINT:
		{
			if (m_suppress_drawing)
				break;

			if (get_pseudo_transparent() && !m_paint_pending)
			{
				RECT rc;

				GetUpdateRect(m_hwnd, &rc, FALSE);
				RefreshBackground(&rc);
				return 0;
			}

			PAINTSTRUCT ps;
			HDC dc = BeginPaint(m_hwnd, &ps);
			on_paint(dc, &ps.rcPaint);
			EndPaint(m_hwnd, &ps);
			m_paint_pending = false;
		}
		return 0;

	case UWM_SIZE:
		on_size(m_width, m_height);
		if (get_pseudo_transparent())
			PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
		else
			Repaint();
		return 0;

	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(m_hwnd, &rect);
			on_size(rect.right - rect.left, rect.bottom - rect.top);
			if (get_pseudo_transparent())
				PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
			else
				Repaint();
		}
		return 0;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pmmi = reinterpret_cast<LPMINMAXINFO>(lp);
			memcpy(&pmmi->ptMaxTrackSize, &MaxSize(), sizeof(POINT));
			memcpy(&pmmi->ptMinTrackSize, &MinSize(), sizeof(POINT));
		}
		return 0;

	case WM_GETDLGCODE:
		return DlgCode();

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		on_mouse_button_down(msg, wp, lp);
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		if (on_mouse_button_up(msg, wp, lp))
			return 0;
		break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		on_mouse_button_dblclk(msg, wp, lp);
		break;

	case WM_CONTEXTMENU:
		on_context_menu(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
		return 1;

	case WM_MOUSEMOVE:
		on_mouse_move(wp, lp);
		break;

	case WM_MOUSELEAVE:
		on_mouse_leave();
		break;

	case WM_MOUSEWHEEL:
		on_mouse_wheel(wp);
		break;

	case WM_SETCURSOR:
		return 1;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		{
			VARIANTARG args[1];
			_variant_t result;

			args[0].vt = VT_UI4;
			args[0].ulVal = (ULONG)wp;

			if (SUCCEEDED(script_invoke_v(CallbackIds::on_key_down, args, _countof(args), &result)))
			{
				result.ChangeType(VT_BOOL);
				if (result.boolVal != VARIANT_FALSE) 
				{
					// If user return true in the callback, bypass keyboard shortcut processing.
					return 0;
				}
			}

			static_api_ptr_t<keyboard_shortcut_manager_v2> ksm;

			if (ksm->process_keydown_simple(wp)) 
			{
				return 0;
			}
		}
		return 0;

	case WM_KEYUP:
		{
			VARIANTARG args[1];

			args[0].vt = VT_UI4;
			args[0].ulVal = (ULONG)wp;

			script_invoke_v(CallbackIds::on_key_up, args, _countof(args));
		}
		return 0;

	case WM_CHAR:
		{
			VARIANTARG args[1];

			args[0].vt = VT_UI4;
			args[0].ulVal = (ULONG)wp;
			script_invoke_v(CallbackIds::on_char, args, _countof(args));
		}
		return 0;

	case WM_SETFOCUS:
		{
			PreserveSelection();

			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = VARIANT_TRUE;
			script_invoke_v(CallbackIds::on_focus, args, _countof(args));
		}
		break;

	case WM_KILLFOCUS:
		{
			m_selection_holder.release();

			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = VARIANT_FALSE;
			script_invoke_v(CallbackIds::on_focus, args, _countof(args));
		}
		break;

	case WM_NOTIFY:
		{
			LRESULT result = 0;
			if (on_notify(lp, &result)) return result;
		}
		break;

	case UWM_SCRIPT_ERROR:
		Repaint();
		m_script_host->Stop();
		script_unload();
		return 0;

	case UWM_SCRIPT_DISABLED_BEFORE:
		// Show error message
		popup_msg::g_show(pfc::string_formatter() 
			<< "Panel ("
			<< ScriptInfo().build_info_string()
			<< "): Refuse to load script due to critical error last run,"
			<< " please check your script and apply it again.",
			WSPM_NAME, 
			popup_message::icon_error);
		return 0;

	case UWM_TIMER:
		on_timer(wp);
		return 0;

	case UWM_TIMER_NEW:
		m_host_timer_dispatcher.invoke(wp);
		return 0;

	case UWM_SHOWCONFIGURE:
		show_configure_popup(m_hwnd);
		return 0;

	case UWM_SHOWPROPERTIES:
		show_property_popup(m_hwnd);
		return 0;

	case CALLBACK_UWM_PLAYLIST_STOP_AFTER_CURRENT:
		on_playlist_stop_after_current_changed(wp);
		return 0;

	case CALLBACK_UWM_CURSOR_FOLLOW_PLAYBACK:
		on_cursor_follow_playback_changed(wp);
		return 0;

	case CALLBACK_UWM_PLAYBACK_FOLLOW_CURSOR:
		on_playback_follow_cursor_changed(wp);
		return 0;

	case CALLBACK_UWM_NOTIFY_DATA:
		on_notify_data(wp);
		return 0;

	case CALLBACK_UWM_GETALBUMARTASYNCDONE:
		on_get_album_art_done(lp);
		return 0;

	case CALLBACK_UWM_LOADIMAGEASYNCDONE:
		on_load_image_done(lp);
		return 0;

	case CALLBACK_UWM_FONT_CHANGED:
		on_font_changed();
		return 0;

	case CALLBACK_UWM_COLORS_CHANGED:
		on_colors_changed();
		return 0;

	case CALLBACK_UWM_ON_ITEM_PLAYED:
		on_item_played(wp);
		return 0;

	case CALLBACK_UWM_ON_CHANGED_SORTED:
		on_changed_sorted(wp);
		return 0;

	case CALLBACK_UWM_ON_SELECTION_CHANGED:
		on_selection_changed(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_STARTING:
		on_playback_starting((playback_control::t_track_command)wp, lp != 0);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK:
		on_playback_new_track(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_STOP:
		on_playback_stop((playback_control::t_stop_reason)wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_SEEK:
		on_playback_seek(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_PAUSE:
		on_playback_pause(wp != 0);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_EDITED:
		on_playback_edited(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO:
		on_playback_dynamic_info();
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK:
		on_playback_dynamic_info_track();
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_TIME:
		on_playback_time(wp);
		return 0;

	case CALLBACK_UWM_ON_VOLUME_CHANGE:
		on_volume_change(wp);
		return 0;

	case CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE:
		on_item_focus_change(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED:
		on_playback_order_changed((t_size)wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_SWITCH:
		on_playlist_switch();
		return 0;

	case CALLBACK_UWM_ON_PLAYLISTS_CHANGED:
		on_playlists_changed();
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_ADDED:
		on_playlist_items_added(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_REORDERED:
		on_playlist_items_reordered(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_REMOVED:
		on_playlist_items_removed(wp, lp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_SELECTION_CHANGE:
		on_playlist_items_selection_change();
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEM_ENSURE_VISIBLE:
		on_playlist_item_ensure_visible(wp, lp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_QUEUE_CHANGED:
		on_playback_queue_changed(wp);
		return 0;

	case CALLBACK_UWM_ON_LIBRARY_ITEMS_ADDED:
		on_library_items_added();
		return 0;

	case CALLBACK_UWM_ON_LIBRARY_ITEMS_REMOVED:
		on_library_items_removed();
		return 0;

	case CALLBACK_UWM_ON_LIBRARY_ITEMS_CHANGED:
		on_library_items_changed();
		return 0;
	}

	return uDefWindowProc(hwnd, msg, wp, lp);
}

void wsh_panel_window::on_size(int w, int h)
{
	TRACK_FUNCTION();

	m_width = w;
	m_height = h;

	delete_context();
	create_context();

	script_invoke_v(CallbackIds::on_size);
}

void wsh_panel_window::on_paint(HDC dc, LPRECT lpUpdateRect)
{
	TRACK_FUNCTION();

	if (!dc || !lpUpdateRect || !m_gr_bmp || !m_gr_wrap)
		return;

	HDC memdc = CreateCompatibleDC(dc);
	HBITMAP oldbmp = SelectBitmap(memdc, m_gr_bmp);

	if (m_script_host->HasError())
	{
		on_paint_error(memdc);
	}
	else
	{
		if (get_pseudo_transparent())
		{
			HDC bkdc = CreateCompatibleDC(dc);
			HBITMAP bkoldbmp = SelectBitmap(bkdc, m_gr_bmp_bk);

			BitBlt(memdc, lpUpdateRect->left, lpUpdateRect->top, 
				lpUpdateRect->right - lpUpdateRect->left, 
				lpUpdateRect->bottom - lpUpdateRect->top, bkdc, lpUpdateRect->left, lpUpdateRect->top, SRCCOPY);

			SelectBitmap(bkdc, bkoldbmp);
			DeleteDC(bkdc);
		}
		else
		{
			RECT rc = {0, 0, m_width, m_height};

			FillRect(memdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
		}

		on_paint_user(memdc, lpUpdateRect);
	}

	BitBlt(dc, 0, 0, m_width, m_height, memdc, 0, 0, SRCCOPY);
	SelectBitmap(memdc, oldbmp);
	DeleteDC(memdc);
}

void wsh_panel_window::on_paint_user(HDC memdc, LPRECT lpUpdateRect)
{
	if (m_script_host->Ready())
	{
		// Prepare graphics object to the script.
		Gdiplus::Graphics gr(memdc);
		Gdiplus::Rect rect(lpUpdateRect->left, lpUpdateRect->top,
			lpUpdateRect->right - lpUpdateRect->left,
			lpUpdateRect->bottom - lpUpdateRect->top);

		// SetClip() may improve performance slightly
		gr.SetClip(rect);

		m_gr_wrap->put__ptr(&gr);

		{
			VARIANTARG args[1];

			args[0].vt = VT_DISPATCH;
			args[0].pdispVal = m_gr_wrap;
			script_invoke_v(CallbackIds::on_paint, args, _countof(args));
		}

		m_gr_wrap->put__ptr(NULL);
	}
}

void wsh_panel_window::on_paint_error(HDC memdc)
{
	const TCHAR errmsg[] = _T("Aw, crashed :(");
	RECT rc = {0, 0, m_width, m_height};
	SIZE sz = {0};

	// Font chosing
	HFONT newfont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, _T("Tahoma"));
	HFONT oldfont = (HFONT)SelectObject(memdc, newfont);

	// Font drawing
	{
		LOGBRUSH lbBack = {BS_SOLID, RGB(35, 48, 64), 0};
		HBRUSH hBack = CreateBrushIndirect(&lbBack);

		FillRect(memdc, &rc, hBack);
		SetBkMode(memdc, TRANSPARENT);

		SetTextColor(memdc, RGB(255, 255, 255));
		DrawText(memdc, errmsg, -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);

		DeleteObject(hBack);
	}

	SelectObject(memdc, oldfont);
}

void wsh_panel_window::on_timer(UINT timer_id)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_UI4;
	args[0].ulVal = timer_id;
	script_invoke_v(CallbackIds::on_timer, args, _countof(args));
}

void wsh_panel_window::on_context_menu(int x, int y)
{
	const int base_id = 0;
	HMENU menu = CreatePopupMenu();
	int ret = 0;

	build_context_menu(menu, x, y, base_id);
	ret = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, m_hwnd, 0);
	execute_context_menu_command(ret, base_id);
	DestroyMenu(menu);
}

void wsh_panel_window::on_mouse_wheel(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
	script_invoke_v(CallbackIds::on_mouse_wheel, args, _countof(args));
}

void wsh_panel_window::on_mouse_leave()
{
	TRACK_FUNCTION();

	m_is_mouse_tracked = false;

	script_invoke_v(CallbackIds::on_mouse_leave);
	// Restore default cursor
	SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
}

void wsh_panel_window::on_mouse_move(WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	if (!m_is_mouse_tracked)
	{
		TRACKMOUSEEVENT tme;

		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hwnd;
		tme.dwFlags = TME_LEAVE;
		TrackMouseEvent(&tme);
		m_is_mouse_tracked = true;

		// Restore default cursor
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
	}

	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);	
	script_invoke_v(CallbackIds::on_mouse_move, args, _countof(args));
}

void wsh_panel_window::on_mouse_button_dblclk(UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);

	switch (msg)
	{
	case WM_LBUTTONDBLCLK:
		script_invoke_v(CallbackIds::on_mouse_lbtn_dblclk, args, _countof(args));
		break;

	case WM_MBUTTONDBLCLK:
		script_invoke_v(CallbackIds::on_mouse_mbtn_dblclk, args, _countof(args));
		break;

	case WM_RBUTTONDBLCLK:
		script_invoke_v(CallbackIds::on_mouse_rbtn_dblclk, args, _countof(args));
		break;
	}
}

bool wsh_panel_window::on_mouse_button_up(UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	bool ret = false;
	VARIANTARG args[3];
	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);	

	switch (msg)
	{
	case WM_LBUTTONUP:
		script_invoke_v(CallbackIds::on_mouse_lbtn_up, args, _countof(args));
		break;

	case WM_MBUTTONUP:
		script_invoke_v(CallbackIds::on_mouse_mbtn_up, args, _countof(args));
		break;

	case WM_RBUTTONUP:
		{
			_variant_t result;

			// Bypass the user code.
			if (IsKeyPressed(VK_LSHIFT) && IsKeyPressed(VK_LWIN))
			{
				break;
			}

			if (SUCCEEDED(script_invoke_v(CallbackIds::on_mouse_rbtn_up, args, _countof(args), &result)))
			{
				result.ChangeType(VT_BOOL);
				if (result.boolVal != VARIANT_FALSE)
					ret = true;
			}
		}
		break;
	}

	ReleaseCapture();
	return ret;
}

void wsh_panel_window::on_mouse_button_down(UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	if (get_grab_focus())
		SetFocus(m_hwnd);

	SetCapture(m_hwnd);

	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		script_invoke_v(CallbackIds::on_mouse_lbtn_down, args, _countof(args));
		break;

	case WM_MBUTTONDOWN:
		script_invoke_v(CallbackIds::on_mouse_mbtn_down, args, _countof(args));
		break;

	case WM_RBUTTONDOWN:
		script_invoke_v(CallbackIds::on_mouse_rbtn_down, args, _countof(args));
		break;
	}
}

bool wsh_panel_window::on_notify(LPARAM lp, LRESULT *pResult)
{
	const auto& tooltip_param = PanelTooltipParam();

	if (!tooltip_param)
		return false;

	LPNMHDR lpnmhdr = (LPNMHDR)lp;

	if (lpnmhdr->code == NM_CUSTOMDRAW) 
	{
		LPNMTTCUSTOMDRAW lpnmcd = (LPNMTTCUSTOMDRAW)lpnmhdr;

		// Check feature
		t_uint32 mask = ScriptInfo().tooltip_mask;
		if ((mask & t_script_info::kTooltipCustomPaint) == 0 &&
			(mask & t_script_info::kTooltipCustomPaintNoBackground) == 0)
			return false;

		// Check tooltip
		if (!tooltip_param->tooltip_hwnd)
			return false;
		if (lpnmhdr->hwndFrom != tooltip_param->tooltip_hwnd)
			return false;
		return on_tooltip_custom_draw(lpnmcd, mask, pResult);
	}

	return false;
}

bool wsh_panel_window::on_tooltip_custom_draw(LPNMTTCUSTOMDRAW lpnmcd, t_uint32 mask, LRESULT * &pResult)
{
	const auto& tooltip_param = PanelTooltipParam();

	if (!tooltip_param) 
		return false;

	// From emule source code:
	// For each tooltip which is to be shown Windows invokes the draw function at least 2 times.
	//	1st invokation: to get the drawing rectangle
	//	2nd invokation: to draw the actual tooltip window contents
	// 
	// 'DrawText' flags for the 1st and 2nd/3rd call
	// ---------------------------------------------
	// NMTTCUSTOMDRAW 00000e50	DT_NOPREFIX | DT_CALCRECT | DT_EXTERNALLEADING | DT_EXPANDTABS | DT_WORDBREAK
	// TTN_SHOW
	// NMTTCUSTOMDRAW 00000a50	DT_NOPREFIX |               DT_EXTERNALLEADING | DT_EXPANDTABS | DT_WORDBREAK
	// --an additional NMTTCUSTOMDRAW may follow which is identical to the 2nd--
	// NMTTCUSTOMDRAW 00000a50	DT_NOPREFIX |               DT_EXTERNALLEADING | DT_EXPANDTABS | DT_WORDBREAK

	if (lpnmcd->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		if (lpnmcd->uDrawFlags & DT_CALCRECT)
		{
			SIZE sz;
			memcpy(&sz, &tooltip_param->tooltip_size, sizeof(SIZE));

			if (sz.cx > 0 || sz.cy > 0) 
			{
				lpnmcd->nmcd.rc.right = lpnmcd->nmcd.rc.left + sz.cx;
				lpnmcd->nmcd.rc.bottom = lpnmcd->nmcd.rc.top + sz.cy;
			}

			return true;
		}

		if (mask & t_script_info::kTooltipCustomPaintNoBackground) 
		{
			return on_tooltip_custom_paint(lpnmcd, pResult);
		}
		else
		{
			*pResult = CDRF_NOTIFYPOSTPAINT;
			return true;
		}
	}
	else if (lpnmcd->nmcd.dwDrawStage == CDDS_POSTPAINT)
	{
		if ((lpnmcd->uDrawFlags & DT_CALCRECT) == 0)
			return on_tooltip_custom_paint(lpnmcd, pResult);
	}

	return false;
}

bool wsh_panel_window::on_tooltip_custom_paint(LPNMTTCUSTOMDRAW lpnmcd, LRESULT * &pResult)
{
	Gdiplus::Graphics gr(lpnmcd->nmcd.hdc);
	Gdiplus::Rect rect(lpnmcd->nmcd.rc.left, lpnmcd->nmcd.rc.top,
		lpnmcd->nmcd.rc.right - lpnmcd->nmcd.rc.left,
		lpnmcd->nmcd.rc.bottom -lpnmcd->nmcd.rc.top);

	gr.SetClip(rect);
	m_gr_wrap->put__ptr(&gr);

	bool succeeded = false;
	VARIANTARG args[1];
	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = m_gr_wrap;
	if (SUCCEEDED(script_invoke_v(CallbackIds::on_tooltip_custom_paint, args, _countof(args))))
		succeeded = true;

	m_gr_wrap->put__ptr(NULL);

	if (!succeeded)
		return false;
	*pResult = CDRF_SKIPDEFAULT;
	return true;
}

void wsh_panel_window::on_refresh_background_done()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_refresh_background_done);
}

void wsh_panel_window::build_context_menu(HMENU menu, int x, int y, int id_base)
{
	::AppendMenu(menu, MF_STRING, id_base + 3, _T("&Reload"));
	::AppendMenu(menu, MF_SEPARATOR, 0, 0);
	::AppendMenu(menu, MF_STRING, id_base + 1, _T("&Properties"));
	::AppendMenu(menu, MF_STRING, id_base + 2, _T("&Configure..."));
}

void wsh_panel_window::execute_context_menu_command(int id, int id_base)
{
	switch (id - id_base)
	{
	case 1:
		show_property_popup(m_hwnd);
		break;
	case 2:
		show_configure_popup(m_hwnd);			
		break;
	case 3:
		update_script();
		break;
	}
}

void wsh_panel_window::on_item_played(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);

	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);
	VARIANTARG args[1];

	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;
	script_invoke_v(CallbackIds::on_item_played, args, _countof(args));

	if (handle)
		handle->Release();
}

void wsh_panel_window::on_get_album_art_done(LPARAM lp)
{
	TRACK_FUNCTION();

	using namespace helpers;
	album_art_async::t_param * param = reinterpret_cast<album_art_async::t_param *>(lp);
	VARIANTARG args[4];

	args[0].vt = VT_BSTR;
	args[0].bstrVal = SysAllocString(param->image_path);
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = param->bitmap;
	args[2].vt = VT_I4;
	args[2].lVal = param->art_id;
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = param->handle;
	script_invoke_v(CallbackIds::on_get_album_art_done, args, _countof(args));
}

void wsh_panel_window::on_load_image_done(LPARAM lp)
{
	TRACK_FUNCTION();

	using namespace helpers;
	load_image_async::t_param * param = reinterpret_cast<load_image_async::t_param *>(lp);
	VARIANTARG args[3];

	args[0].vt = VT_BSTR;
	args[0].bstrVal = param->path;
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = param->bitmap;
	args[2].vt = VT_I4;
	args[2].lVal = param->cookie;
	script_invoke_v(CallbackIds::on_load_image_done, args, _countof(args));
}

void wsh_panel_window::on_playlist_stop_after_current_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(wp);
	script_invoke_v(CallbackIds::on_playlist_stop_after_current_changed, args, _countof(args));
}

void wsh_panel_window::on_cursor_follow_playback_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(wp);
	script_invoke_v(CallbackIds::on_cursor_follow_playback_changed, args, _countof(args));
}

void wsh_panel_window::on_playback_follow_cursor_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(wp);
	script_invoke_v(CallbackIds::on_playback_follow_cursor_changed, args, _countof(args));
}

void wsh_panel_window::on_notify_data(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];
	simple_callback_data_scope_releaser<simple_callback_data_2<_bstr_t, _variant_t> > data(wp);

	args[0] = data->m_item2;
	args[1].vt = VT_BSTR;
	args[1].bstrVal = data->m_item1;
	script_invoke_v(CallbackIds::on_notify_data, args, _countof(args));
}

void wsh_panel_window::on_font_changed()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_font_changed);
}

void wsh_panel_window::on_colors_changed()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_colors_changed);
}

void wsh_panel_window::on_playback_starting(play_control::t_track_command cmd, bool paused)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(paused);
	args[1].vt = VT_I4;
	args[1].lVal = cmd;
	script_invoke_v(CallbackIds::on_playback_starting, args, _countof(args));
}

void wsh_panel_window::on_playback_new_track(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);
	VARIANTARG args[1];
	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);

	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;
	script_invoke_v(CallbackIds::on_playback_new_track, args, _countof(args));

	if (handle)
		handle->Release();
}

void wsh_panel_window::on_playback_stop(play_control::t_stop_reason reason)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = reason;
	script_invoke_v(CallbackIds::on_playback_stop, args, _countof(args));
}

void wsh_panel_window::on_playback_seek(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<double> > data(wp);

	VARIANTARG args[1];

	args[0].vt = VT_R8;
	args[0].dblVal = data->m_item;
	script_invoke_v(CallbackIds::on_playback_seek, args, _countof(args));
}

void wsh_panel_window::on_playback_pause(bool state)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(state);
	script_invoke_v(CallbackIds::on_playback_pause, args, _countof(args));
}

void wsh_panel_window::on_playback_edited(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);
	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);
	VARIANTARG args[1];

	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;

	script_invoke_v(CallbackIds::on_playback_edited);

	if (handle)
	{
		handle->Release();
	}
}

void wsh_panel_window::on_playback_dynamic_info()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_playback_dynamic_info);
}

void wsh_panel_window::on_playback_dynamic_info_track()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_playback_dynamic_info_track);
}

void wsh_panel_window::on_playback_time(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<double> > data(wp);

	VARIANTARG args[1];

	args[0].vt = VT_R8;
	args[0].dblVal = data->m_item;
	script_invoke_v(CallbackIds::on_playback_time, args, _countof(args));
}

void wsh_panel_window::on_volume_change(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<float> > data(wp);

	VARIANTARG args[1];

	args[0].vt = VT_R4;
	args[0].fltVal = data->m_item;
	script_invoke_v(CallbackIds::on_volume_change, args, _countof(args));
}

void wsh_panel_window::on_item_focus_change(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data_3<t_size, t_size, t_size> > data(wp);
	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = data->m_item3;
	args[1].vt = VT_I4;
	args[1].lVal = data->m_item2;
	args[2].vt = VT_I4;
	args[2].lVal = data->m_item1;
	script_invoke_v(CallbackIds::on_item_focus_change, args, _countof(args));
}

void wsh_panel_window::on_playback_order_changed(t_size p_new_index)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = p_new_index;
	script_invoke_v(CallbackIds::on_playback_order_changed, args, _countof(args));
}

void wsh_panel_window::on_playlist_switch()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_playlist_switch);
}

void wsh_panel_window::on_playlists_changed()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_playlists_changed);
}

void wsh_panel_window::on_playlist_items_added(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];
	args[0].vt = VT_UI4;
	args[0].ulVal = wp;
	script_invoke_v(CallbackIds::on_playlist_items_added, args, _countof(args));
}

void wsh_panel_window::on_playlist_items_reordered(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];
	args[0].vt = VT_UI4;
	args[0].ulVal = wp;
	script_invoke_v(CallbackIds::on_playlist_items_reordered, args, _countof(args));
}

void wsh_panel_window::on_playlist_items_removed(WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];
	args[0].vt = VT_UI4;
	args[0].ulVal = lp;
	args[1].vt = VT_UI4;
	args[1].ulVal = wp;
	script_invoke_v(CallbackIds::on_playlist_items_removed, args, _countof(args));
}

void wsh_panel_window::on_playlist_items_selection_change()
{
	TRACK_FUNCTION();

	script_invoke_v(CallbackIds::on_playlist_items_selection_change);
}

void wsh_panel_window::on_playlist_item_ensure_visible(WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];
	args[0].vt = VT_UI4;
	args[0].ulVal = lp;
	args[1].vt = VT_UI4;
	args[1].ulVal = wp;
	script_invoke_v(CallbackIds::on_playlist_item_ensure_visible, args, _countof(args));
}

void wsh_panel_window::on_changed_sorted(WPARAM wp)
{
	TRACK_FUNCTION();

	if (ScriptInfo().feature_mask & t_script_info::kFeatureNoWatchMetadb)
		return;

	simple_callback_data_scope_releaser<nonautoregister_callbacks::t_on_changed_sorted_data> data(wp);
	VARIANTARG args[2];
	IDispatch * handle = NULL;

	if (ScriptInfo().feature_mask & t_script_info::kFeatureMetadbHandleList0)
	{
		handle = new com_object_impl_t<FbMetadbHandleList>(data->m_items_sorted);
	}
	else
	{
		if (m_watched_handle.is_empty())
			return;

		if (!data->m_items_sorted.have_item(m_watched_handle))
			return;

		handle = new com_object_impl_t<FbMetadbHandle>(m_watched_handle);
	}

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(data->m_fromhook);
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = handle;
	script_invoke_v(CallbackIds::on_metadb_changed, args, _countof(args));

	if (handle)
		handle->Release();
}

void wsh_panel_window::on_selection_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	if (wp != 0)
	{
		if (ScriptInfo().feature_mask & t_script_info::kFeatureMetadbHandleList0)
		{
			script_invoke_v(CallbackIds::on_selection_changed);
		}
		else
		{
			IDispatch * handle = NULL;
			simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);
			handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);

			VARIANTARG args[1];

			args[0].vt = VT_DISPATCH;
			args[0].pdispVal = handle;
			script_invoke_v(CallbackIds::on_selection_changed, args, _countof(args));

			if (handle)
				handle->Release();
		}
	}
}

void wsh_panel_window::on_playback_queue_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];
	args[0].vt = VT_I4;
	args[0].lVal = wp;
	script_invoke_v(CallbackIds::on_playback_queue_changed, args, _countof(args));
}

void wsh_panel_window::on_library_items_added()
{
	TRACK_FUNCTION();
	
	script_invoke_v(CallbackIds::on_library_items_added);
}

void wsh_panel_window::on_library_items_removed()
{
	TRACK_FUNCTION();
	
	script_invoke_v(CallbackIds::on_library_items_removed);
}

void wsh_panel_window::on_library_items_changed()
{
	TRACK_FUNCTION();
	
	script_invoke_v(CallbackIds::on_library_items_changed);
}
