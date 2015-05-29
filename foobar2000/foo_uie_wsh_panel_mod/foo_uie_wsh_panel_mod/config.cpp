#include "stdafx.h"
#include "config.h"
#include "resource.h"


void wsh_panel_vars::get_default_script_code(pfc::string_base & out)
{
	out.reset();
	puResource pures = uLoadResource(core_api::get_my_instance(), uMAKEINTRESOURCE(IDR_SCRIPT), "SCRIPT");

	if (pures)
		out.set_string(reinterpret_cast<const char *>(pures->GetPointer()), pures->GetSize());
}

void wsh_panel_vars::reset_config()
{
	m_script_engine_str = "JScript";
	get_default_script_code(m_script_code);
	m_pseudo_transparent = false;
	m_wndpl.length = 0;
	m_grab_focus = true;
	m_disabled_before = false;
	m_delay_load = false;
	m_edge_style = NO_EDGE;
	CoCreateGuid(&m_config_guid);
}

void wsh_panel_vars::load_config(stream_reader * reader, t_size size, abort_callback & abort)
{
	reset_config();

	if (size > sizeof(unsigned))
	{
		bool have_read_config = false;
		unsigned ver = 0;
		
		try
		{
			// Read version
			reader->read_object_t(ver, abort);

			// fall-thru
			switch (ver)
			{
			case VERSION_0x81:
				reader->read_object_t(m_delay_load, abort);

			case VERSION_0x80:
				// Due to default value of delay load had been changed, skip
				if (ver < VERSION_0x81)
				{
					reader->skip_object(sizeof(m_delay_load), abort);
				}

			case VERSION_0x79:
				reader->read_object_t(m_config_guid, abort);

			case VERSION_0x78:
				reader->read_object(&m_edge_style, sizeof(m_edge_style), abort);

			case VERSION_0x77:
				m_config_prop.load(reader, abort);

			case VERSION_0x76:
				reader->read_object_t(m_disabled_before, abort);

			case VERSION_0x75:
				reader->read_object_t(m_grab_focus, abort);

			case VERSION_0x74:
				reader->read_object(&m_wndpl, sizeof(m_wndpl), abort);

			case VERSION_0x73:
				reader->read_string(m_script_engine_str, abort);
				reader->read_string(m_script_code, abort);
				reader->read_object_t(m_pseudo_transparent, abort);
				have_read_config = true;
				break;
			}
		}
		catch (std::exception &)
		{
			have_read_config = false;
			reset_config();
		}

		if (!have_read_config)
		{
			// Configuration corruputed or config version dismatch.
			console::complain(WSPM_NAME, 
				"Error: Configuration needs a newer component or is corrupted");
		}
	}
}

void wsh_panel_vars::save_config(stream_writer * writer, abort_callback & abort) const
{
	unsigned const VERSION_CURRENT = CONFIG_VERSION_CURRENT;

	try
	{
		// Write version
		writer->write_object_t(VERSION_CURRENT, abort);
		//
		writer->write_object_t(m_delay_load, abort);
		writer->write_object_t(m_config_guid, abort);
		writer->write_object(&m_edge_style, sizeof(m_edge_style), abort);
		m_config_prop.save(writer, abort);
		writer->write_object_t(m_disabled_before, abort);
		writer->write_object_t(m_grab_focus, abort);
		writer->write_object(&m_wndpl, sizeof(m_wndpl), abort);
		writer->write_string(m_script_engine_str, abort);
		writer->write_string(m_script_code, abort);
		writer->write_object_t(m_pseudo_transparent, abort);
	}
	catch (std::exception &)
	{
	}
}

bool prop_kv_config::g_is_allowed_type(VARTYPE p_vt)
{
	switch (p_vt)
	{
	case VT_UI1:
	case VT_I1:
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_I8:
	case VT_UI8:
	case VT_R8:
	case VT_CY:
	case VT_DATE:
	case VT_BSTR:
		return true;
	}

	return false;
}

bool prop_kv_config::get_config_item(const char * p_key, VARIANT & p_out)
{
	t_val val;

	if (m_map.query(p_key, val))
	{
		if (g_is_allowed_type(val.vt) && SUCCEEDED(VariantCopy(&p_out, &val)))
		{
			return true;
		}
		else
		{
			m_map.remove(p_key);
		}
	}

	return false;
}

void prop_kv_config::set_config_item(const char * p_key, const VARIANT & p_val)
{
	if (!g_is_allowed_type(p_val.vt))
	{
		m_map.remove(p_key);
	}
	else
	{
		m_map[p_key] = p_val;
	}
}

void prop_kv_config::load(stream_reader * reader, abort_callback & abort)
{
	g_load(m_map, reader, abort);
}

void prop_kv_config::save(stream_writer * writer, abort_callback & abort) const
{
	g_save(m_map, writer, abort);
}

void prop_kv_config::g_load(t_map & data, stream_reader * reader, abort_callback & abort) throw()
{
	t_size count;

	data.remove_all();

	try
	{
		// Get count
		reader->read_lendian_t(count, abort);

		for (t_size i = 0; i < count; ++i)
		{
			pfc::string8_fast key;
			t_val val;
			VARTYPE vt;
			int cbRead = 0;

			// read key
			reader->read_string(key, abort);
			// read vtype
			reader->read_lendian_t(vt, abort);

			switch (vt)
			{
			case VT_UI1:
			case VT_I1:
				cbRead = sizeof(BYTE);
				break;

			case VT_I2:
			case VT_UI2:
			case VT_BOOL:
				cbRead = sizeof(short);
				break;

			case VT_I4:
			case VT_UI4:
			case VT_R4:
			case VT_INT:
			case VT_UINT:
				cbRead = sizeof(long);
				break;

			case VT_I8:
			case VT_UI8:
				cbRead = sizeof(LONGLONG);
				break;

			case VT_R8:
			case VT_CY:
			case VT_DATE:
				cbRead = sizeof(double);
				break;
			}

			val.vt = vt;

			if (cbRead != 0)
			{
				reader->read(&val.bVal, cbRead, abort);
			}
			else
			{
				// Read to bstr
				pfc::string8_fast str;

				reader->read_string(str, abort);
				val.bstrVal = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(str));
			}

			data[key] = val;
		}
	}
	catch (std::exception &)
	{

	}
}

void prop_kv_config::g_save(const t_map & data, stream_writer * writer, abort_callback & abort) throw()
{
	try
	{
		// Write count
		writer->write_lendian_t(data.get_count(), abort);

		for (t_map::const_iterator iter = data.first(); iter.is_valid(); ++iter)
		{
			// Write key
			writer->write_string(iter->m_key, abort);
			// Write vt
			writer->write_lendian_t(iter->m_value.vt, abort);
			// Write value
			int cbWrite = 0;

			switch (iter->m_value.vt)
			{
			case VT_UI1:
			case VT_I1:
				cbWrite = sizeof(BYTE);
				break;

			case VT_I2:
			case VT_UI2:
			case VT_BOOL:
				cbWrite = sizeof(short);
				break;

			case VT_I4:
			case VT_UI4:
			case VT_R4:
			case VT_INT:
			case VT_UINT:
				cbWrite = sizeof(long);
				break;

			case VT_I8:
			case VT_UI8:
				cbWrite = sizeof(LONGLONG);
				break;

			case VT_R8:
			case VT_CY:
			case VT_DATE:
				cbWrite = sizeof(double);
				break;
			}

			if (cbWrite != 0)
			{
				writer->write(&iter->m_value.bVal, cbWrite, abort);
			}
			else if (iter->m_value.vt == VT_BSTR)
			{
				pfc::stringcvt::string_utf8_from_wide conv = iter->m_value.bstrVal;
				writer->write_string(conv, abort);
			}
		}
	}
	catch (std::exception &)
	{

	}
}

//void panel_config::g_import(t_map & data, stream_reader * reader, abort_callback & abort) throw()
//{
//	GUID guid;
//
//	try
//	{
//		reader->read_lendian_t(guid, abort);
//
//		if (guid != m_guid)
//			return;
//	}
//	catch (std::exception &)
//	{
//		return;
//	}
//
//	g_load(data, reader, abort);
//}
//
//void panel_config::g_export(const t_map & data, stream_writer * writer, abort_callback & abort) throw()
//{
//	try
//	{
//		writer->write_lendian_t(m_guid, abort);
//	}
//	catch (std::exception &)
//	{
//		return;
//	}
//
//	g_save(data, writer, abort);
//}
