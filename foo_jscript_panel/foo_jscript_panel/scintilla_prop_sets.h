#pragma once


enum t_sci_editor_style_flag 
{
	ESF_NONE = 0,
	ESF_FONT = 1 << 0,
	ESF_SIZE = 1 << 1,
	ESF_FORE = 1 << 2,
	ESF_BACK = 1 << 3,
	ESF_BOLD = 1 << 4,
	ESF_ITALICS = 1 << 5,
	ESF_UNDERLINED = 1 << 6,
	ESF_CASEFORCE = 1 << 7,
};

struct t_sci_editor_style 
{
	t_sci_editor_style()
	{
		flags = 0;
	}

	unsigned flags;
	bool italics, bold, underlined;
	pfc::string_simple font;
	unsigned size;
	DWORD fore, back;
	int case_force;
};

struct t_sci_prop_set 
{
	pfc::string_simple key, defaultval, val;
};

struct t_prop_set_init_table 
{
	const char * key;
	const char * defaultval;
};

typedef pfc::list_t<t_sci_prop_set> t_sci_prop_set_list;
typedef pfc::map_t<pfc::string_simple, pfc::string_simple, pfc::comparator_stricmp_ascii> t_str_to_str_map;

class cfg_sci_prop_sets : public cfg_var
{
private:
	t_sci_prop_set_list m_data;

	void init_data(const t_prop_set_init_table * p_default);
	void merge_data(const t_str_to_str_map & data_map);

public:
	explicit inline cfg_sci_prop_sets(const GUID & p_guid, const t_prop_set_init_table * p_default) : cfg_var(p_guid)
	{
		init_data(p_default);
	}

	inline t_sci_prop_set_list & val() { return m_data; }
	inline const t_sci_prop_set_list & val() const { return m_data; }

	void get_data_raw(stream_writer * p_stream, abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream, t_size p_sizehint, abort_callback & p_abort);

	void reset();
	void export_to_file(const char * filename);
	void import_from_file(const char * filename);
};

extern cfg_sci_prop_sets g_sci_prop_sets;
