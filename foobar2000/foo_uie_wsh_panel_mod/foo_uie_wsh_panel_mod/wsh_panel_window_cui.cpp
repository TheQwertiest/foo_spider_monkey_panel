#include "stdafx.h"
#include "wsh_panel_window.h"
#include "wsh_panel_window_cui.h"
#include "popup_msg.h"


// CUI panel instance
static uie::window_factory<wsh_panel_window_cui> g_wsh_panel_wndow_cui;


const GUID& wsh_panel_window_cui::get_extension_guid() const
{
	return g_wsh_panel_window_extension_guid;
}

void wsh_panel_window_cui::get_name(pfc::string_base& out) const
{
	out = WSPM_NAME;
}

void wsh_panel_window_cui::get_category(pfc::string_base& out) const
{
	out = "Panels";
}

unsigned wsh_panel_window_cui::get_type() const
{
	return uie::type_toolbar | uie::type_panel;
}

void wsh_panel_window_cui::set_config(stream_reader * reader, t_size size, abort_callback & abort)
{
	load_config(reader, size, abort);
}

void wsh_panel_window_cui::get_config(stream_writer * writer, abort_callback & abort) const
{
	save_config(writer, abort);
}

bool wsh_panel_window_cui::have_config_popup() const
{
	return true;
}

bool wsh_panel_window_cui::show_config_popup(HWND parent)
{
	return show_configure_popup(parent);
}

LRESULT wsh_panel_window_cui::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		try
		{
			static_api_ptr_t<columns_ui::fonts::manager>()->register_common_callback(this);
			static_api_ptr_t<columns_ui::colours::manager>()->register_common_callback(this);
		}
		catch (exception_service_not_found &)
		{
			// Using in Default UI and dockable panels without Columns UI installed?
			static bool g_reported = false;
			const char warning[] = "Warning: At least one " WSPM_NAME " instance is running in CUI containers "
				"(dockable panels, Func UI, etc) without some services provided by the "
				"Columns UI component (have not been installed or have a very old "
				"version installed?).\n"
				"Please download and install the latest version of Columns UI:\n"
				"http://yuo.be/columns.php";

			if (!g_cfg_cui_warning_reported)
			{
				popup_msg::g_show(pfc::string_formatter(warning) << "\n\n[This popup message will be shown only once]", 
					WSPM_NAME);

				g_cfg_cui_warning_reported = true;
			}
			else if (!g_reported)
			{
				console::formatter() << "\n" WSPM_NAME ": " << warning << "\n\n";
				g_reported = true;
			}
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

HWND wsh_panel_window_cui::create_or_transfer_window(HWND parent, const uie::window_host_ptr & host, const ui_helpers::window_position_t & p_position)
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

void wsh_panel_window_cui::notify_size_limit_changed_(LPARAM lp)
{
	get_host()->on_size_limit_change(m_hwnd, lp);
}

void wsh_panel_window_cui::on_font_changed(t_size mask) const
{
	PostMessage(m_hwnd, CALLBACK_UWM_FONT_CHANGED, 0, 0);
}

void wsh_panel_window_cui::on_colour_changed(t_size mask) const
{
	PostMessage(m_hwnd, CALLBACK_UWM_COLORS_CHANGED, 0, 0);
}

void wsh_panel_window_cui::on_bool_changed(t_size mask) const
{
	// TODO: may be implemented one day
}

DWORD wsh_panel_window_cui::GetColorCUI(unsigned type, const GUID & guid)
{
	if (type <= columns_ui::colours::colour_active_item_frame)
	{
		columns_ui::colours::helper helper(guid);

		return helpers::convert_colorref_to_argb(
			helper.get_colour((columns_ui::colours::colour_identifier_t)type));
	}

	return 0;
}

HFONT wsh_panel_window_cui::GetFontCUI(unsigned type, const GUID & guid)
{
	if (guid == pfc::guid_null)
	{
		if (type <= columns_ui::fonts::font_type_labels)
		{
			try
			{
				return static_api_ptr_t<columns_ui::fonts::manager>()->get_font((columns_ui::fonts::font_type_t)type);
			}
			catch (exception_service_not_found &)
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
