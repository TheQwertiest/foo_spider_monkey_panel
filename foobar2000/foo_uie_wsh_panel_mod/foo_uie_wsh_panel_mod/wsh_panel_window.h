#pragma once

#include "host.h"
#include "delay_loader.h"
#include "panel_tooltip_param.h"


class ScriptHost;
class CDialogConf;
class CDialogProperty;


class wsh_panel_window : public HostComm, public ui_helpers::container_window
{
private:
	class delay_script_init_action : public delay_loader_action
	{
	public:
		delay_script_init_action(HWND wnd) : wnd_(wnd) {}

		virtual void execute()
		{
			SendMessage(wnd_, UWM_SCRIPT_INIT, 0, 0);
		}

	private:
		HWND wnd_;
	};

	CComPtr<IDropTargetImpl>   m_drop_target;
	// Scripting
	IGdiGraphicsPtr  m_gr_wrap;
	ScriptHost      *m_script_host;
	bool             m_is_mouse_tracked;
	bool	         m_is_droptarget_registered;

public:
	wsh_panel_window();
	virtual ~wsh_panel_window();
	void update_script(const char * name = NULL, const char * code = NULL);
	inline HRESULT script_invoke_v(int callbackId, VARIANTARG * argv = NULL, UINT argc = 0, VARIANT * ret = NULL)
	{
		return m_script_host->InvokeCallback(callbackId, argv, argc, ret);
	}

private:
	bool script_load();
	void script_unload();

	void create_context();
	void delete_context();

	virtual ui_helpers::container_window::class_data & get_class_data() const;

protected:
	bool show_configure_popup(HWND parent);
	bool show_property_popup(HWND parent);
	LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
	void on_size(int w, int h);
	void on_paint(HDC dc, LPRECT lpUpdateRect);
	void on_paint_user( HDC memdc, LPRECT lpUpdateRect);
	void on_paint_error(HDC memdc);
	void on_timer(UINT timer_id);
	void on_context_menu(int x, int y);
	void on_mouse_wheel(WPARAM wp);
	void on_mouse_leave();
	void on_mouse_move(WPARAM wp, LPARAM lp);
	void on_mouse_button_dblclk(UINT msg, WPARAM wp, LPARAM lp);
	bool on_mouse_button_up(UINT msg, WPARAM wp, LPARAM lp);
	void on_mouse_button_down(UINT msg, WPARAM wp, LPARAM lp);
	void on_refresh_background_done();

	bool on_notify(LPARAM lp, LRESULT *pResult);
	bool on_tooltip_custom_draw(LPNMTTCUSTOMDRAW lpnmcd, t_uint32 mask, LRESULT * &pResult);
	bool on_tooltip_custom_paint(LPNMTTCUSTOMDRAW lpnmcd, LRESULT * &pResult);

protected:
	static void build_context_menu(HMENU menu, int x, int y, int id_base);
	void execute_context_menu_command(int id, int id_base);

private:
	// callbacks
	void on_get_album_art_done(LPARAM lp);
	void on_load_image_done(LPARAM lp);
	void on_item_played(WPARAM wp);
	void on_playlist_stop_after_current_changed(WPARAM wp);
	void on_cursor_follow_playback_changed(WPARAM wp);
	void on_playback_follow_cursor_changed(WPARAM wp);
	void on_notify_data(WPARAM wp);

	void on_font_changed();
	void on_colors_changed();

	// play_callback
	void on_playback_starting(play_control::t_track_command cmd, bool paused);
	void on_playback_new_track(WPARAM wp);
	void on_playback_stop(play_control::t_stop_reason reason);
	void on_playback_seek(WPARAM wp);
	void on_playback_pause(bool state);
	void on_playback_edited(WPARAM wp);
	void on_playback_dynamic_info();
	void on_playback_dynamic_info_track();
	void on_playback_time(WPARAM wp);
	void on_volume_change(WPARAM wp);

	// playlist_callback
	void on_playlist_items_added(WPARAM wp);
	void on_playlist_items_reordered(WPARAM wp);
	void on_playlist_items_removed(WPARAM wp, LPARAM lp);
	void on_playlist_items_selection_change();
	void on_playlist_item_ensure_visible(WPARAM wp, LPARAM lp);
	void on_item_focus_change(WPARAM wp);
	void on_playback_order_changed(t_size p_new_index);
	void on_playlist_switch();
	void on_playlists_changed();
	
	// library callback
	void on_library_items_added();
	void on_library_items_removed();
	void on_library_items_changed();

	// metadb_io_callback_dynamic
	void on_changed_sorted(WPARAM wp);

	// ui_selection_callback
	void on_selection_changed(WPARAM wp);

	// playback queue callback
	void on_playback_queue_changed(WPARAM wp);

protected:
	// override me
	virtual void notify_size_limit_changed_(LPARAM lp) = 0;
};
