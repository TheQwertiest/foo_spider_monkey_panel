#pragma once

class js_panel_window_dui 
    : public js_panel_window
    , public ui_element_instance
{
public:
	js_panel_window_dui(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback);
	virtual ~js_panel_window_dui();

	static GUID g_get_guid();
	static GUID g_get_subclass();
	static pfc::string8 g_get_description();
	static ui_element_config::ptr g_get_default_configuration();
	static void g_get_name(pfc::string_base& out);
	virtual DWORD GetColourCUI(unsigned type, const GUID& guid) override;
	virtual DWORD GetColourDUI(unsigned type) override;
	virtual GUID get_guid();
	virtual GUID get_subclass();
	virtual HFONT GetFontCUI(unsigned type, const GUID& guid) override;
	virtual HFONT GetFontDUI(unsigned type) override;
	virtual HWND get_wnd();
	virtual LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	virtual bool edit_mode_context_menu_get_description(unsigned p_id, unsigned p_id_base, pfc::string_base& p_out);
	virtual bool edit_mode_context_menu_test(const POINT& p_point, bool p_fromkeyboard);
	virtual ui_element_config::ptr get_configuration();
	virtual void edit_mode_context_menu_build(const POINT& p_point, bool p_fromkeyboard, HMENU p_menu, unsigned p_id_base);
	virtual void edit_mode_context_menu_command(const POINT& p_point, bool p_fromkeyboard, unsigned p_id, unsigned p_id_base);
	virtual void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);
	virtual void set_configuration(ui_element_config::ptr data);
	void initialize_window(HWND parent);

private:
    virtual void notify_size_limit_changed( LPARAM lp ) override;
    void notify_is_edit_mode_changed( bool enabled );

    using t_parent = js_panel_window;
    bool m_is_edit_mode;   
    ui_element_instance_callback::ptr m_callback;
};
