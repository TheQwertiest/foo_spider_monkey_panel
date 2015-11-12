#pragma once

class wsh_panel_window_dui : public wsh_panel_window, public ui_element_instance
{
public:
	wsh_panel_window_dui(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) : m_callback(callback)
	{
		m_instance_type = KInstanceTypeDUI;
		m_is_edit_mode = m_callback->is_edit_mode_enabled();
		set_configuration(cfg);
	}

	virtual ~wsh_panel_window_dui() { t_parent::destroy(); }

	void initialize_window(HWND parent);

	virtual HWND get_wnd();

	virtual void set_configuration(ui_element_config::ptr data);
	static ui_element_config::ptr g_get_default_configuration();
	virtual ui_element_config::ptr get_configuration();

	static void g_get_name(pfc::string_base & out);
	static pfc::string8 g_get_description();

	static GUID g_get_guid();
	virtual GUID get_guid();

	static GUID g_get_subclass();
	virtual GUID get_subclass();

	virtual void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);

	virtual bool edit_mode_context_menu_test(const POINT & p_point,bool p_fromkeyboard) {return true;}
	virtual void edit_mode_context_menu_build(const POINT & p_point,bool p_fromkeyboard,HMENU p_menu,unsigned p_id_base) { build_context_menu(p_menu, p_point.x, p_point.y, p_id_base); }
	virtual void edit_mode_context_menu_command(const POINT & p_point,bool p_fromkeyboard,unsigned p_id,unsigned p_id_base) { execute_context_menu_command(p_id, p_id_base); }
	//virtual bool edit_mode_context_menu_get_focus_point(POINT & p_point) {return true;}
	virtual bool edit_mode_context_menu_get_description(unsigned p_id,unsigned p_id_base,pfc::string_base & p_out) {return false;}

	virtual LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	// HostComm
	virtual DWORD GetColorCUI(unsigned type, const GUID & guid) { return 0; }
	virtual HFONT GetFontCUI(unsigned type, const GUID & guid) { return NULL; }
	virtual DWORD GetColorDUI(unsigned type);
	virtual HFONT GetFontDUI(unsigned type);

private:
	void notify_is_edit_mode_changed_(bool enabled) { m_is_edit_mode = enabled; }
	virtual void notify_size_limit_changed_(LPARAM lp);

private:
	typedef wsh_panel_window t_parent;
	ui_element_instance_callback::ptr m_callback;
	bool m_is_edit_mode;
};
