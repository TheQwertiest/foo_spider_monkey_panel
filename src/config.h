#pragma once

enum t_version_info
{
	JSP_VERSION_100 = 123, // must start with 123 so we don't break component upgrades
	CONFIG_VERSION_CURRENT = JSP_VERSION_100
};

enum t_edge_style : char
{
	NO_EDGE = 0,
	SUNKEN_EDGE,
	GREY_EDGE,
};

inline LONG edge_style_from_config(t_edge_style edge_style)
{
	switch (edge_style)
	{
	case SUNKEN_EDGE:
		return WS_EX_CLIENTEDGE;

	case GREY_EDGE:
		return WS_EX_STATICEDGE;

	default:
		return 0;
	}
}

class prop_kv_config
{
public:
	typedef prop_kv_config t_self;
	typedef pfc::string_simple t_key;
	typedef _variant_t t_val;
	typedef pfc::map_t<t_key, t_val, pfc::comparator_stricmp_ascii> t_map;

	t_map& get_val()
	{
		return m_map;
	}

	bool get_config_item(const char* p_key, VARIANT& p_out);
	static bool g_is_allowed_type(VARTYPE p_vt);
	static void g_load(t_map& data, stream_reader* reader, abort_callback& abort) throw();
	static void g_save(const t_map& data, stream_writer* writer, abort_callback& abort) throw();
	void load(stream_reader* reader, abort_callback& abort) throw();
	void save(stream_writer* writer, abort_callback& abort) const throw();
	void set_config_item(const char* p_key, const VARIANT& p_val);

private:
	t_map m_map;
};

class js_panel_vars
{
public:
	js_panel_vars()
	{
		reset_config();
	}

	GUID& get_config_guid();
	WINDOWPLACEMENT& get_windowplacement();
	bool& get_disabled_before();
	bool& get_grab_focus();
	bool& get_pseudo_transparent();
	const bool& get_pseudo_transparent() const;
	const t_edge_style& get_edge_style() const;
	pfc::string_base& get_script_code();
	pfc::string_base& get_script_engine();
	prop_kv_config& get_config_prop();
	static void get_default_script_code(pfc::string_base& out);
	t_edge_style& get_edge_style();
	void load_config(stream_reader* reader, t_size size, abort_callback& abort);
	void reset_config();
	void save_config(stream_writer* writer, abort_callback& abort) const;

private:
	GUID m_config_guid;
	WINDOWPLACEMENT m_wndpl;
	prop_kv_config m_config_prop;
	pfc::string8 m_script_engine_str;
	pfc::string8 m_script_code;
	t_edge_style m_edge_style;
	bool m_disabled_before;
	bool m_grab_focus;
	bool m_pseudo_transparent;
};
