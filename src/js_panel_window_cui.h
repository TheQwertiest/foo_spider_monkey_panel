#pragma once

class js_panel_window_cui : public js_panel_window, public uie::window, public columns_ui::fonts::common_callback, public columns_ui::colours::common_callback
{
protected:
	virtual DWORD GetColourCUI(unsigned type, const GUID& guid);
	virtual DWORD GetColourDUI(unsigned type);
	virtual HFONT GetFontCUI(unsigned type, const GUID& guid);
	virtual HFONT GetFontDUI(unsigned type);
	virtual HWND create_or_transfer_window(HWND parent, const uie::window_host_ptr& host, const ui_helpers::window_position_t& p_position);
	virtual HWND get_wnd() const;
	virtual LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	virtual bool have_config_popup() const;
	virtual bool is_available(const uie::window_host_ptr& p) const;
	virtual bool show_config_popup(HWND parent);
	virtual const GUID& get_extension_guid() const;
	virtual const uie::window_host_ptr& get_host() const;
	virtual unsigned get_type() const;
	virtual void destroy_window();
	virtual void get_category(pfc::string_base& out) const;
	virtual void get_config(stream_writer* writer, abort_callback& abort) const;
	virtual void get_name(pfc::string_base& out) const;
	virtual void on_bool_changed(t_size mask) const;
	virtual void on_colour_changed(t_size mask) const;
	virtual void on_font_changed(t_size mask) const;
	virtual void set_config(stream_reader* reader, t_size size, abort_callback& abort);

private:
	typedef js_panel_window t_parent;
	uie::window_host_ptr m_host;
	virtual void notify_size_limit_changed_(LPARAM lp);
};
