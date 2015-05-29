#pragma once


// pre declaration
class HostComm;

struct t_directive_value
{
	typedef pfc::array_t<wchar_t> t_array;

	t_array directive;
	t_array value;

	t_directive_value()
	{
	}

	t_directive_value(const t_array & p_directive, const t_array & p_value) :
		directive(p_directive), 
		value(p_value)
	{
	}
};

struct t_script_info
{
	enum 
	{
		kFeatureMetadbHandleList0	=	1 << 0,
		kFeatureDragDrop			=	1 << 1,
		kFeatureNoWatchMetadb		=	1 << 7,
	};

	enum
	{
		kTooltipCustomPaint             =   1 << 0,
		kTooltipCustomPaintNoBackground =   1 << 1,
	};

	pfc::string_simple name;
	pfc::string_simple version;
	pfc::string_simple author;
	t_uint32 feature_mask;
	t_uint32 tooltip_mask;

	t_script_info(GUID & guid_ref) : m_guid_ref(guid_ref) {}

	void clear()
	{
		name = "";
		version = "";
		author = "";
		feature_mask = 0;
		tooltip_mask = 0;
	}

	pfc::string8 build_info_string() const
	{
		pfc::string8 ret;

		if (!name.is_empty())
			ret << name;
		else
			ret << "{" << pfc::print_guid(m_guid_ref) << "}";

		if (!version.is_empty()) ret << " v" << version;
		if (!author.is_empty()) ret << " by " << author;
		
		return ret;
	}

private:
	GUID & m_guid_ref;
};

class script_preprocessor
{
public:
	struct t_script_code
	{
		pfc::array_t<wchar_t> path;
		pfc::array_t<wchar_t> code;
	};

	typedef pfc::list_t<t_script_code> t_script_list;

	script_preprocessor(const wchar_t * script)
	{
		PFC_ASSERT(script != NULL);
		m_is_ok = preprocess(script);
	}

	HRESULT process_import(const t_script_info & info, t_script_list & scripts);
	bool process_script_info(t_script_info & info);
	void parse_directive_feature(pfc::string_simple &value, t_script_info &info);

private:
	bool preprocess(const wchar_t * script);
	bool scan_directive_and_value(const wchar_t *& p, const wchar_t * pend);
	bool scan_value(const wchar_t *& p, const wchar_t * pend);
	bool expand_var(pfc::array_t<wchar_t> & out);
	bool extract_preprocessor_block(const wchar_t * script, int & block_begin, int & block_end);

private:
	pfc::array_t<wchar_t> m_directive_buffer;
	pfc::array_t<wchar_t> m_value_buffer;
	pfc::list_t<t_directive_value> m_directive_value_list;
	bool m_is_ok;
};
