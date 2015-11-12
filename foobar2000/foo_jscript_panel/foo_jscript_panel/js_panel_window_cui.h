#pragma once

class wsh_panel_window_cui : public wsh_panel_window, public uie::window, 
	public columns_ui::fonts::common_callback, public columns_ui::colours::common_callback
{
protected:
	// ui_extension
	virtual const GUID & get_extension_guid() const;
	virtual void get_name(pfc::string_base& out) const;
	virtual void get_category(pfc::string_base& out) const;
	virtual unsigned get_type() const;
	virtual void set_config(stream_reader * reader, t_size size, abort_callback & abort);
	virtual void get_config(stream_writer * writer, abort_callback & abort) const;
	virtual bool have_config_popup() const;
	virtual bool show_config_popup(HWND parent);
	virtual LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	virtual bool is_available(const uie::window_host_ptr & p) const {return true;}
	virtual const uie::window_host_ptr & get_host() const {return m_host;}
	virtual HWND create_or_transfer_window(HWND parent, const uie::window_host_ptr & host, const ui_helpers::window_position_t & p_position);
	virtual void destroy_window() {destroy(); m_host.release();}
	virtual HWND get_wnd() const { return t_parent::get_wnd(); }

	// columns_ui::fonts::common_callback
	virtual void on_font_changed(t_size mask) const;

	// columns_ui::colours::common_callback
	virtual void on_colour_changed(t_size mask) const;
	virtual void on_bool_changed(t_size mask) const;

	// HostComm
	virtual DWORD GetColorCUI(unsigned type, const GUID & guid);
	virtual HFONT GetFontCUI(unsigned type, const GUID & guid);
	virtual DWORD GetColorDUI(unsigned type) { return 0; }
	virtual HFONT GetFontDUI(unsigned type) { return NULL; }

private:
	virtual void notify_size_limit_changed_(LPARAM lp);

private:
	typedef wsh_panel_window t_parent;
	uie::window_host_ptr m_host;
};
