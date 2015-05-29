#include "stdafx.h"
#include "wsh_panel_window.h"
#include "wsh_panel_window_dui.h"


// Just because I don't want to include the helpers
template<typename TImpl>
class my_ui_element_impl : public ui_element 
{
public:
	GUID get_guid() { return TImpl::g_get_guid();}
	GUID get_subclass() { return TImpl::g_get_subclass();}
	void get_name(pfc::string_base & out) { TImpl::g_get_name(out); }

	ui_element_instance::ptr instantiate(HWND parent,ui_element_config::ptr cfg,ui_element_instance_callback::ptr callback) 
	{
		PFC_ASSERT( cfg->get_guid() == get_guid() );
		service_nnptr_t<ui_element_instance_impl_helper> item = new service_impl_t<ui_element_instance_impl_helper>(cfg, callback);
		item->initialize_window(parent);
		return item;
	}

	ui_element_config::ptr get_default_configuration() { return TImpl::g_get_default_configuration(); }
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) {return NULL;}
	bool get_description(pfc::string_base & out) {out = TImpl::g_get_description(); return true;}

private:
	class ui_element_instance_impl_helper : public TImpl 
	{
	public:
		ui_element_instance_impl_helper(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback)
			: TImpl(cfg, callback) {}
	};
};

// DUI panel instance
static service_factory_t<my_ui_element_impl<wsh_panel_window_dui> > g_wsh_panel_wndow_dui;

void wsh_panel_window_dui::initialize_window(HWND parent)
{
	t_parent::create(parent);
}

HWND wsh_panel_window_dui::get_wnd()
{
	return t_parent::get_wnd();
}

void wsh_panel_window_dui::set_configuration(ui_element_config::ptr data)
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

ui_element_config::ptr wsh_panel_window_dui::g_get_default_configuration()
{
	ui_element_config_builder builder;
	abort_callback_dummy abort;
	wsh_panel_vars vars;

	vars.reset_config();
	vars.save_config(&builder.m_stream, abort);
	return builder.finish(g_get_guid());
}

ui_element_config::ptr wsh_panel_window_dui::get_configuration()
{
	ui_element_config_builder builder;
	abort_callback_dummy abort;

	save_config(&builder.m_stream, abort);
	return builder.finish(g_get_guid());
}

void wsh_panel_window_dui::g_get_name(pfc::string_base & out)
{
	out = WSPM_NAME;
}

pfc::string8 wsh_panel_window_dui::g_get_description()
{
	return "Customizable panel with VBScript and JScript scripting support.";
}

GUID wsh_panel_window_dui::g_get_guid()
{
	return g_wsh_panel_window_dui_guid;
}

GUID wsh_panel_window_dui::get_guid()
{
	return g_get_guid();
}

GUID wsh_panel_window_dui::g_get_subclass()
{
	return ui_element_subclass_utility;
}

GUID wsh_panel_window_dui::get_subclass()
{
	return g_get_subclass();
}

void wsh_panel_window_dui::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size)
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
		PostMessage(m_hwnd, CALLBACK_UWM_COLORS_CHANGED, 0, 0);
	}
}

LRESULT wsh_panel_window_dui::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
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

void wsh_panel_window_dui::notify_size_limit_changed_(LPARAM lp)
{
	m_callback->on_min_max_info_change();
}

DWORD wsh_panel_window_dui::GetColorDUI(unsigned type)
{
	const GUID * guids[] = {
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

HFONT wsh_panel_window_dui::GetFontDUI(unsigned type)
{
	const GUID * guids[] = {
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
