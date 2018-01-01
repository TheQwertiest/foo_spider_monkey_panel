#include "stdafx.h"
#include "js_panel_window.h"
#include "js_panel_window_cui.h"

// CUI panel instance
static uie::window_factory<js_panel_window_cui> g_js_panel_wndow_cui;

DWORD js_panel_window_cui::GetColourCUI(unsigned type, const GUID& guid)
{
	if (type <= columns_ui::colours::colour_active_item_frame)
	{
		columns_ui::colours::helper helper(guid);

		return helpers::convert_colorref_to_argb(helper.get_colour((columns_ui::colours::colour_identifier_t)type));
	}

	return 0;
}

DWORD js_panel_window_cui::GetColourDUI(unsigned type)
{
	return 0;
}

HFONT js_panel_window_cui::GetFontCUI(unsigned type, const GUID& guid)
{
	if (guid == pfc::guid_null)
	{
		if (type <= columns_ui::fonts::font_type_labels)
		{
			try
			{
				return static_api_ptr_t<columns_ui::fonts::manager>()->get_font((columns_ui::fonts::font_type_t)type);
			}
			catch (exception_service_not_found&)
			{
				return uCreateIconFont();
			}
		}
	}
	else
	{
		columns_ui::fonts::helper helper(guid);
		return helper.get_font();
	}

	return NULL;
}

HFONT js_panel_window_cui::GetFontDUI(unsigned type)
{
	return NULL;
}

HWND js_panel_window_cui::create_or_transfer_window(HWND parent, const uie::window_host_ptr& host, const ui_helpers::window_position_t& p_position)
{
	if (m_host.is_valid())
	{
		ShowWindow(m_hwnd, SW_HIDE);
		SetParent(m_hwnd, parent);
		m_host->relinquish_ownership(m_hwnd);
		m_host = host;

		SetWindowPos(m_hwnd, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
	}
	else
	{
		m_host = host; //store interface to host
		create(parent, this, p_position);
	}

	return get_wnd();
}

HWND js_panel_window_cui::get_wnd() const
{
	return t_parent::get_wnd();
}

LRESULT js_panel_window_cui::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		if (uie::window::g_process_keydown_keyboard_shortcuts(wp))
			return 0;
		break;

	case WM_CREATE:
		try
		{
			static_api_ptr_t<columns_ui::fonts::manager>()->register_common_callback(this);
			static_api_ptr_t<columns_ui::colours::manager>()->register_common_callback(this);
		}
		catch (...)
		{
		}
		break;

	case WM_DESTROY:
		try
		{
			static_api_ptr_t<columns_ui::fonts::manager>()->deregister_common_callback(this);
			static_api_ptr_t<columns_ui::colours::manager>()->deregister_common_callback(this);
		}
		catch (...)
		{
		}
		break;

	case UWM_SIZELIMITECHANGED:
		notify_size_limit_changed_(lp);
		return 0;
	}

	return t_parent::on_message(hwnd, msg, wp, lp);
}

bool js_panel_window_cui::have_config_popup() const
{
	return true;
}

bool js_panel_window_cui::is_available(const uie::window_host_ptr& p) const
{
	return true;
}

bool js_panel_window_cui::show_config_popup(HWND parent)
{
	return show_configure_popup(parent);
}

const GUID& js_panel_window_cui::get_extension_guid() const
{
	return g_js_panel_window_cui_guid;
}

const uie::window_host_ptr& js_panel_window_cui::get_host() const
{
	return m_host;
}

unsigned js_panel_window_cui::get_type() const
{
	return uie::type_toolbar | uie::type_panel;
}

void js_panel_window_cui::destroy_window()
{
	destroy();
	m_host.release();
}

void js_panel_window_cui::get_category(pfc::string_base& out) const
{
	out = "Panels";
}

void js_panel_window_cui::get_config(stream_writer* writer, abort_callback& abort) const
{
	save_config(writer, abort);
}

void js_panel_window_cui::get_name(pfc::string_base& out) const
{
	out = JSP_NAME;
}

void js_panel_window_cui::on_bool_changed(t_size mask) const
{
	// TODO: may be implemented one day
}

void js_panel_window_cui::on_colour_changed(t_size mask) const
{
	PostMessage(m_hwnd, CALLBACK_UWM_COLOURS_CHANGED, 0, 0);
}

void js_panel_window_cui::on_font_changed(t_size mask) const
{
	PostMessage(m_hwnd, CALLBACK_UWM_FONT_CHANGED, 0, 0);
}

void js_panel_window_cui::set_config(stream_reader* reader, t_size size, abort_callback& abort)
{
	load_config(reader, size, abort);
}

void js_panel_window_cui::notify_size_limit_changed_(LPARAM lp)
{
	get_host()->on_size_limit_change(m_hwnd, lp);
}
