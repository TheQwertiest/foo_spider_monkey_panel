#pragma once


enum t_version_info
{
	VERSION_0x73 = 0x73,
	VERSION_0x74,
	VERSION_0x75,
	VERSION_0x76,
	VERSION_0x77,
	VERSION_0x78,
	VERSION_0x79,
	VERSION_0x80,
	VERSION_0x81,
	CONFIG_VERSION_CURRENT = VERSION_0x81,
};

enum t_edge_style : char
{
	NO_EDGE = 0,
	SUNKEN_EDGE,
	GREY_EDGE,
};

inline DWORD edge_style_from_config(t_edge_style edge_style)
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

public:
	static bool g_is_allowed_type(VARTYPE p_vt);

	inline t_map & get_val() { return m_map; }
	// p_out should be inited or cleared.
	bool get_config_item(const char * p_key, VARIANT & p_out);
	void set_config_item(const char * p_key, const VARIANT & p_val);

	void load(stream_reader * reader, abort_callback & abort) throw();
	void save(stream_writer * writer, abort_callback & abort) const throw();

	static void g_load(t_map & data, stream_reader * reader, abort_callback & abort) throw();
	static void g_save(const t_map & data, stream_writer * writer, abort_callback & abort) throw();

	//static void g_import(t_map & data, stream_reader * reader, abort_callback & abort) throw();
	//static void g_export(const t_map & data, stream_writer * writer, abort_callback & abort) throw();

private:
	t_map m_map;
};

class wsh_panel_vars
{
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
	bool m_delay_load;

public:
	wsh_panel_vars()
	{
		reset_config();
	}

	static void get_default_script_code(pfc::string_base & out);
	void reset_config();
	void load_config(stream_reader * reader, t_size size, abort_callback & abort);
	void save_config(stream_writer * writer, abort_callback & abort) const;

	inline pfc::string_base& get_script_engine()
	{
		return m_script_engine_str;
	}

	inline pfc::string_base& get_script_code()
	{
		return m_script_code;
	}

	inline bool & get_delay_load()
	{
		return m_delay_load;
	}

	inline bool & get_pseudo_transparent()
	{
		return m_pseudo_transparent;
	}

	inline const bool & get_pseudo_transparent() const
	{
		return m_pseudo_transparent;
	}

	inline bool & get_grab_focus()
	{
		return m_grab_focus;
	}

	inline WINDOWPLACEMENT & get_windowplacement()
	{
		return m_wndpl;
	}

	inline bool & get_disabled_before()
	{
		return m_disabled_before;
	}

	inline prop_kv_config & get_config_prop()
	{
		return m_config_prop;
	}

	inline t_edge_style & get_edge_style()
	{
		return m_edge_style;
	}

	inline const t_edge_style & get_edge_style() const
	{
		return m_edge_style;
	}

	inline GUID & get_config_guid()
	{
		return m_config_guid;
	}
};
