#include "stdafx.h"
#include "js_panel_window.h"
#include "js_panel_window_dui.h"

// Just because I don't want to include the helpers
template <typename TImpl> class my_ui_element_impl : public ui_element
{
public:
	GUID get_guid()
	{
		return TImpl::g_get_guid();
	}
	GUID get_subclass()
	{
		return TImpl::g_get_subclass();
	}
	void get_name(pfc::string_base& out)
	{
		TImpl::g_get_name(out);
	}
	ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback)
	{
		PFC_ASSERT(cfg->get_guid() == get_guid());
		service_nnptr_t<ui_element_instance_impl_helper> item = new service_impl_t<ui_element_instance_impl_helper>(cfg, callback);
		item->initialize_window(parent);
		return item;
	}
	ui_element_config::ptr get_default_configuration()
	{
		return TImpl::g_get_default_configuration();
	}
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg)
	{
		return NULL;
	}
	bool get_description(pfc::string_base& out)
	{
		out = TImpl::g_get_description();
		return true;
	}

private:
	class ui_element_instance_impl_helper : public TImpl
	{
	public:
		ui_element_instance_impl_helper(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) : TImpl(cfg, callback)
		{
		}
	};
};

// DUI panel instance
static service_factory_t<my_ui_element_impl<js_panel_window_dui>> g_js_panel_wndow_dui;

js_panel_window_dui::js_panel_window_dui(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) : m_callback(callback)
{
	m_instance_type = KInstanceTypeDUI;
	m_is_edit_mode = m_callback->is_edit_mode_enabled();
	set_configuration(cfg);
}

js_panel_window_dui::~js_panel_window_dui()
{
	t_parent::destroy();
}

GUID js_panel_window_dui::g_get_guid()
{
	return g_js_panel_window_dui_guid;
}

GUID js_panel_window_dui::g_get_subclass()
{
	return ui_element_subclass_utility;
}

pfc::string8 js_panel_window_dui::g_get_description()
{
	return "Customisable panel with JScript scripting support.";
}

ui_element_config::ptr js_panel_window_dui::g_get_default_configuration()
{
	ui_element_config_builder builder;
	abort_callback_dummy abort;
	js_panel_vars vars;

	vars.reset_config();
	vars.save_config(&builder.m_stream, abort);
	return builder.finish(g_get_guid());
}

void js_panel_window_dui::g_get_name(pfc::string_base& out)
{
	out = JSP_NAME;
}

DWORD js_panel_window_dui::GetColourCUI(unsigned type, const GUID& guid)
{
	return 0;
}

DWORD js_panel_window_dui::GetColourDUI(unsigned type)
{
	const GUID* guids[] = {
		&ui_color_text,
		&ui_color_background,
		&ui_color_highlight,
		&ui_color_selection,
	};

	if (type < _countof(guids))
	{
		return helpers::convert_colorref_to_argb(m_callback->query_std_color(*guids[type]));
	}

	return 0;
}

GUID js_panel_window_dui::get_guid()
{
	return g_get_guid();
}

GUID js_panel_window_dui::get_subclass()
{
	return g_get_subclass();
}

HFONT js_panel_window_dui::GetFontCUI(unsigned type, const GUID& guid)
{
	return NULL;
}

HFONT js_panel_window_dui::GetFontDUI(unsigned type)
{
	const GUID* guids[] = {
		&ui_font_default,
		&ui_font_tabs,
		&ui_font_lists,
		&ui_font_playlists,
		&ui_font_statusbar,
		&ui_font_console,
	};

	if (type < _countof(guids))
	{
		return m_callback->query_font_ex(*guids[type]);
	}

	return NULL;
}

HWND js_panel_window_dui::get_wnd()
{
	return t_parent::get_wnd();
}

LRESULT js_panel_window_dui::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_CONTEXTMENU:
		if (m_is_edit_mode)
			return DefWindowProc(hwnd, msg, wp, lp);
		break;

	case UWM_SIZELIMITECHANGED:
		notify_size_limit_changed_(lp);
		return 0;
	}

	return t_parent::on_message(hwnd, msg, wp, lp);
}

bool js_panel_window_dui::edit_mode_context_menu_get_description(unsigned p_id, unsigned p_id_base, pfc::string_base& p_out)
{
	return false;
}

bool js_panel_window_dui::edit_mode_context_menu_test(const POINT& p_point, bool p_fromkeyboard)
{
	return true;
}

ui_element_config::ptr js_panel_window_dui::get_configuration()
{
	ui_element_config_builder builder;
	abort_callback_dummy abort;

	save_config(&builder.m_stream, abort);
	return builder.finish(g_get_guid());
}

void js_panel_window_dui::edit_mode_context_menu_build(const POINT& p_point, bool p_fromkeyboard, HMENU p_menu, unsigned p_id_base)
{
	build_context_menu(p_menu, p_point.x, p_point.y, p_id_base);
}

void js_panel_window_dui::edit_mode_context_menu_command(const POINT& p_point, bool p_fromkeyboard, unsigned p_id, unsigned p_id_base)
{
	execute_context_menu_command(p_id, p_id_base);
}

void js_panel_window_dui::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size)
{
	if (p_what == ui_element_notify_edit_mode_changed)
	{
		notify_is_edit_mode_changed_(m_callback->is_edit_mode_enabled());
	}
	else if (p_what == ui_element_notify_font_changed)
	{
		PostMessage(m_hwnd, CALLBACK_UWM_FONT_CHANGED, 0, 0);
	}
	else if (p_what == ui_element_notify_colors_changed)
	{
		PostMessage(m_hwnd, CALLBACK_UWM_COLOURS_CHANGED, 0, 0);
	}
}

void js_panel_window_dui::set_configuration(ui_element_config::ptr data)
{
	ui_element_config_parser parser(data);
	abort_callback_dummy abort;

	load_config(&parser.m_stream, parser.get_remaining(), abort);

	// FIX: If window already created, DUI won't destroy it and create it again.
	if (m_hwnd)
	{
		update_script();
	}
}

void js_panel_window_dui::initialize_window(HWND parent)
{
	create(parent);
}

void js_panel_window_dui::notify_size_limit_changed_(LPARAM lp)
{
	m_callback->on_min_max_info_change();
}

void js_panel_window_dui::notify_is_edit_mode_changed_(bool enabled)
{
	m_is_edit_mode = enabled;
}
