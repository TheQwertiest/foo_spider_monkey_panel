#include "stdafx.h"
#include "config.h"
#include "script_preprocessor.h"
#include "helpers.h"


HRESULT script_preprocessor::process_import(const t_script_info & info, t_script_list & scripts)
{
	HRESULT hr = S_OK;

	if (!m_is_ok) return hr;

	for (t_size i = 0; i < m_directive_value_list.get_count(); ++i)
	{
		t_directive_value & val = m_directive_value_list[i];

		if (wcscmp(val.directive.get_ptr(), L"import") == 0)
		{
			// Try parse
			expand_var(val.value);

			pfc::array_t<wchar_t> code;
			bool success = helpers::read_file_wide(CP_ACP, val.value.get_ptr(), code);
			pfc::string_formatter msg;

			if (!success)
			{
				msg << "Error: ";
			}

			msg << WSPM_NAME " (" << info.build_info_string() << "): "
				<< "Parsing file \"" << pfc::stringcvt::string_utf8_from_wide(val.value.get_ptr())
				<< "\"";

			if (!success)
			{
				msg << ": Failed to load";
			}

			console::formatter() << msg;

			if (success)
			{
				t_script_code script;
				script.path = val.value;
				script.code = code;

				scripts.add_item(script);
			}
		}
	}

	return hr;
}

bool script_preprocessor::process_script_info(t_script_info & info)
{
	bool ret = false;

	info.clear();
	if (!m_is_ok) return ret;

	for (t_size i = 0; i < m_directive_value_list.get_count(); ++i)
	{
		t_directive_value & v = m_directive_value_list[i];
		expand_var(v.value);
		pfc::string_simple value = pfc::stringcvt::string_utf8_from_wide(v.value.get_ptr());

		if (wcscmp(v.directive.get_ptr(), L"name") == 0)
		{
			ret = true;
			info.name = value;
		}
		else if (wcscmp(v.directive.get_ptr(), L"version") == 0)
		{
			ret = true;
			info.version = value;
		}
		else if (wcscmp(v.directive.get_ptr(), L"author") == 0)
		{
			ret = true;
			info.author = value;
		}
		else if (wcscmp(v.directive.get_ptr(), L"feature") == 0)
		{
			parse_directive_feature(value, info);
		}
		else if (wcscmp(v.directive.get_ptr(), L"tooltip") == 0)
		{
			if (strcmp(value.get_ptr(), "custom-paint") == 0)
			{
				info.tooltip_mask |= t_script_info::kTooltipCustomPaint;
			}
			else if (strcmp(value.get_ptr(), "custom-paint-no-background") == 0)
			{
				info.tooltip_mask |= t_script_info::kTooltipCustomPaintNoBackground;
			}
		}
	}

	return ret;
}

bool script_preprocessor::preprocess(const wchar_t * script)
{
	// Haven't introduce a FSM, so yes, these codes below looks really UGLY.
	int block_begin = 0;
	int block_end = 0;

	if (!extract_preprocessor_block(script, block_begin, block_end))
		return false;

	const wchar_t * p = script + block_begin;
	const wchar_t * pend = script + block_end;

	while (*p && p < pend)
	{
		while (*p == ' ' || *p == '\t' || *p == '\'')
			++p;

		if (wcsncmp(p, L"//", 2) == 0)
		{
			p += 2;

			// Skip blank chars
			while (*p == ' ' || *p == '\t')
				++p;

			if (scan_directive_and_value(p, pend))
			{
				// Put in the directive_value list
				m_directive_value_list.add_item(t_directive_value(m_directive_buffer, m_value_buffer));
			}
		}

		// Jump to the next line
		while (*p && *p != '\n')
			++p;

		if (*p == '\n')
			++p;
	}

	return true;
}

bool script_preprocessor::scan_directive_and_value(const wchar_t *& p, const wchar_t * pend)
{
	m_directive_buffer.force_reset();

	if (*p == '@')
	{
		++p;

		const wchar_t * pdirective_begin = 0;
		const wchar_t * pdirective_end = 0;

		if (isalpha(*p))
		{
			pdirective_begin = p;
			++p;

			while (isalnum(*p) || *p == '_' || *p == '-' || *p == '.')
				++p;

			// We may get a directive now
			pdirective_end = p;

			// Scan value
			if (scan_value(p, pend))
			{
				// We may get a value now, set the directive
				m_directive_buffer.set_size(pdirective_end - pdirective_begin + 1);
				pfc::__unsafe__memcpy_t(m_directive_buffer.get_ptr(), pdirective_begin, pdirective_end - pdirective_begin);
				m_directive_buffer[pdirective_end - pdirective_begin] = 0;
			}
		}
	}

	return (m_directive_buffer.get_size() > 0 && m_value_buffer.get_size() > 0);
}

bool script_preprocessor::scan_value(const wchar_t *& p, const wchar_t * pend)
{
	const t_size delta = 32;

	m_value_buffer.force_reset();

	// Skip leading spaces
	while (*p == ' ' || *p == '\t')
		++p;

	if (*p == '"')
	{
		++p;

		while (*p && (p < pend) && (*p != '\r') && (*p != '\n'))
		{
			const wchar_t * p2 = p + 1;

			// Escape (")
			if (*p == '"')
			{
				if (*p2 == '"')
				{
					m_value_buffer.append_single(*p2);
					p = p2 + 1;
					continue;
				}
				else
				{
					// Add trailing 'zero'
					m_value_buffer.append_single(0);
					return true;
				}
			}
			else
			{
				m_value_buffer.append_single(*p);
				++p;
			}
		}
	}

	m_value_buffer.force_reset();
	return false;
}

bool script_preprocessor::expand_var(pfc::array_t<wchar_t> & out)
{
	typedef pfc::string8_fast (*t_func)();

	enum {
		KStateInNormal,
		KStateInPercent,
	};

	struct {
		const wchar_t * which;
		t_func func;
	} expand_table[] = {
		{ L"fb2k_path", helpers::get_fb2k_path },
		{ L"fb2k_component_path", helpers::get_fb2k_component_path },
		{ L"fb2k_profile_path", helpers::get_profile_path },
	};

	pfc::array_t<wchar_t> buffer;

	wchar_t * pscan = out.get_ptr();
	const wchar_t * pready = NULL;
	const t_size delta = 32;

	int state = KStateInNormal;

	while (*pscan)
	{
		switch (state)
		{
		case KStateInNormal:
			if (*pscan == '%')
			{
				pready = pscan;
				state = KStateInPercent;
			}
			else
			{
				buffer.append_single(*pscan);
			}
			break;

		case KStateInPercent:
			if (*pscan == '%')
			{
				unsigned count = pscan - pready - 1;

				if (!count)
				{
					buffer.append_single('%');
				}
				else
				{
					bool found = false;

					for (t_size i = 0; i < _countof(expand_table); ++i)
					{
						t_size expand_which_size = wcslen(expand_table[i].which);

						if (wcsncmp(pready + 1, expand_table[i].which, max(count, expand_which_size)) == 0)
						{
							pfc::stringcvt::string_wide_from_utf8_fast expanded(expand_table[i].func());
							t_size expanded_count = expanded.length();

							buffer.append_fromptr(expanded.get_ptr(), expanded_count);
							found = true;
							break;
						}
					}

					if (!found)
					{
						buffer.append_fromptr(pready, count);
					}
				}

				state = KStateInNormal;
			}
			break;
		}

		++pscan;
	}

	if (state == KStateInPercent)
	{
		buffer.append_fromptr(pscan, wcslen(pscan));
	}

	// trailing 'zero'
	buffer.append_single(0);
	// Copy
	out = buffer;
	return true;
}

bool script_preprocessor::extract_preprocessor_block(const wchar_t * script, int & block_begin, int & block_end)
{
	block_begin = 0;
	block_end = 0;

	if (!script) return false;

	const wchar_t preprocessor_begin[] = L"==PREPROCESSOR==";
	const wchar_t preprocessor_end[] = L"==/PREPROCESSOR==";

	const wchar_t * pblock_begin = wcsstr(script, preprocessor_begin);

	if (!pblock_begin) return false;

	pblock_begin += _countof(preprocessor_begin) - 1;

	// to next line
	while (*pblock_begin && (*pblock_begin != '\n'))
		++pblock_begin;

	const wchar_t * pblock_end = wcsstr(pblock_begin, preprocessor_end);

	if (!pblock_end) return false;

	// to prev line
	while ((pblock_end > script) && (*pblock_end != '\n'))
		--pblock_end;

	if (*pblock_end == '\r')
		--pblock_end;

	if (pblock_end <= pblock_begin)
		return false;

	block_begin = pblock_begin - script;
	block_end = pblock_end - script;
	return true;
}

void script_preprocessor::parse_directive_feature(pfc::string_simple &value, t_script_info &info)
{
	const char * str = value.get_ptr();
	const char first_char = *str;

	if (*str == 'v')
	{
		++str;
		int pos = strchr(str, '.') - str;

		if (pos > 0)
		{
			t_uint8 major = pfc::atoui_ex(str, pos) & 0xff;
			t_uint8 minor = atoi(str + pos + 1) & 0xff;

			switch (major)
			{
			case 1:
				switch (minor)
				{
				case 4:
					info.feature_mask |= t_script_info::kFeatureMetadbHandleList0;
					info.feature_mask |= t_script_info::kFeatureNoWatchMetadb;
					break;

				default:
					break;
				}
				break;

			default:
				info.feature_mask = 0;
				break;
			}
		}
	}
	else if (strcmp(str, "watch-metadb") == 0)
	{
		info.feature_mask &= ~t_script_info::kFeatureNoWatchMetadb;
	}
	else if (strcmp(str, "dragdrop") == 0)
	{
		info.feature_mask |= t_script_info::kFeatureDragDrop;
	}
}
