#include <stdafx.h>
#include "backend.h"

#include "panel_manager.h"
#include "popup_msg.h"

PanelBackend::PanelBackend()
	: m_hwnd(NULL)
	, m_hdc(NULL)
	, m_width(0)
	, m_height(0)
	, m_gr_bmp(NULL)
	, m_suppress_drawing(false)
	, m_paint_pending(false)
	, m_accuracy(0)
	, m_instance_type(KInstanceTypeCUI)
	, m_dlg_code(0)
	, m_script_info(get_config_guid())
	, m_panel_tooltip_param_ptr(new panel_tooltip_param)
{
	m_max_size.x = INT_MAX;
	m_max_size.y = INT_MAX;

	m_min_size.x = 0;
	m_min_size.y = 0;

    jsEngineFailed_ = false;
}

PanelBackend::~PanelBackend()
{
}

void PanelBackend::JsEngineFail( std::string_view errorText )
{
    jsEngineFailed_ = true;

    popup_msg::g_show( errorText.data(), JSP_NAME );
    MessageBeep( MB_ICONASTERISK );

    SendMessage( m_hwnd, UWM_SCRIPT_ERROR, 0, 0 );

}

GUID PanelBackend::GetGUID()
{
	return get_config_guid();
}

HDC PanelBackend::GetHDC()
{
	return m_hdc;
}

HWND PanelBackend::GetHWND()
{
	return m_hwnd;
}

INT PanelBackend::GetHeight()
{
	return m_height;
}

INT PanelBackend::GetWidth()
{
	return m_width;
}

POINT& PanelBackend::MaxSize()
{
	return m_max_size;
}

POINT& PanelBackend::MinSize()
{
	return m_min_size;
}

UINT& PanelBackend::DlgCode()
{
	return m_dlg_code;
}

UINT PanelBackend::GetInstanceType()
{
	return m_instance_type;
}

panel_tooltip_param_ptr& PanelBackend::PanelTooltipParam()
{
	return m_panel_tooltip_param_ptr;
}

t_script_info& PanelBackend::ScriptInfo()
{
	return m_script_info;
}

unsigned PanelBackend::SetInterval(IDispatch* func, uint32_t delay)
{
	return HostTimerDispatcher::Get().setInterval(m_hwnd, delay, func);
}

unsigned PanelBackend::SetTimeout(IDispatch* func, uint32_t delay)
{
	return HostTimerDispatcher::Get().setTimeout(m_hwnd, delay, func);
}

void PanelBackend::ClearIntervalOrTimeout( uint32_t timerId)
{
	HostTimerDispatcher::Get().killTimer(timerId);
}

void PanelBackend::Redraw()
{
	m_paint_pending = false;
	RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void PanelBackend::RefreshBackground(LPRECT lprcUpdate)
{
	HWND wnd_parent = GetAncestor(m_hwnd, GA_PARENT);

	if (!wnd_parent || IsIconic(core_api::get_main_window()) || !IsWindowVisible(m_hwnd))
		return;

	HDC dc_parent = GetDC(wnd_parent);
	HDC hdc_bk = CreateCompatibleDC(dc_parent);
	POINT pt = { 0, 0 };
	RECT rect_child = { 0, 0, m_width, m_height };
	RECT rect_parent;
	HRGN rgn_child = NULL;

	// HACK: for Tab control
	// Find siblings
	HWND hwnd = NULL;
	while (hwnd = FindWindowEx(wnd_parent, hwnd, NULL, NULL))
	{
		TCHAR buff[64];
		if (hwnd == m_hwnd) continue;
		GetClassName(hwnd, buff, _countof(buff));
		if (_tcsstr(buff, _T("SysTabControl32")))
		{
			wnd_parent = hwnd;
			break;
		}
	}

	if (lprcUpdate)
	{
		HRGN rgn = CreateRectRgnIndirect(lprcUpdate);
		rgn_child = CreateRectRgnIndirect(&rect_child);
		CombineRgn(rgn_child, rgn_child, rgn, RGN_DIFF);
		DeleteRgn(rgn);
	}
	else
	{
		rgn_child = CreateRectRgn(0, 0, 0, 0);
	}

	ClientToScreen(m_hwnd, &pt);
	ScreenToClient(wnd_parent, &pt);

	CopyRect(&rect_parent, &rect_child);
	ClientToScreen(m_hwnd, (LPPOINT)&rect_parent);
	ClientToScreen(m_hwnd, (LPPOINT)&rect_parent + 1);
	ScreenToClient(wnd_parent, (LPPOINT)&rect_parent);
	ScreenToClient(wnd_parent, (LPPOINT)&rect_parent + 1);

	// Force Repaint
	m_suppress_drawing = true;
	SetWindowRgn(m_hwnd, rgn_child, FALSE);
	RedrawWindow(wnd_parent, &rect_parent, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW);

	// Background bitmap
	HBITMAP old_bmp = SelectBitmap(hdc_bk, m_gr_bmp_bk);

	// Paint BK
	BitBlt(hdc_bk, rect_child.left, rect_child.top, rect_child.right - rect_child.left, rect_child.bottom - rect_child.top, dc_parent, pt.x, pt.y, SRCCOPY);

	SelectBitmap(hdc_bk, old_bmp);
	DeleteDC(hdc_bk);
	ReleaseDC(wnd_parent, dc_parent);
	DeleteRgn(rgn_child);
	SetWindowRgn(m_hwnd, NULL, FALSE);
	m_suppress_drawing = false;
	if (get_edge_style()) SendMessage(m_hwnd, WM_NCPAINT, 1, 0);
	Repaint(true);
}

void PanelBackend::Repaint(bool force)
{
	m_paint_pending = true;

	if (force)
	{
		RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	else
	{
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void PanelBackend::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force)
{
	RECT rc;
	rc.left = x;
	rc.top = y;
	rc.right = x + w;
	rc.bottom = y + h;

	m_paint_pending = true;

	if (force)
	{
		RedrawWindow(m_hwnd, &rc, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	else
	{
		InvalidateRect(m_hwnd, &rc, FALSE);
	}
}

/*

bool ScriptHost::HasError()
{
	return m_has_error;
}

bool ScriptHost::Ready()
{
	return m_engine_inited && m_script_engine;
}

HRESULT ScriptHost::GenerateSourceContext(const wchar_t* path, const wchar_t* code, DWORD& source_context)
{
	pfc::stringcvt::string_wide_from_utf8_fast name, guidString;
	HRESULT hr = S_OK;
	t_size len = wcslen(code);

	if (!path)
	{
		if (m_host->ScriptInfo().name.is_empty())
			name.convert(pfc::print_guid(m_host->GetGUID()));
		else
			name.convert(m_host->ScriptInfo().name);

		guidString.convert(pfc::print_guid(m_host->GetGUID()));
	}

	source_context = m_lastSourceContext++;
	return hr;
}


HRESULT ScriptHost::Initialize()
{
	HRESULT hr = S_OK;

	Finalize();

	m_has_error = false;

	IActiveScriptParsePtr parser;
	pfc::stringcvt::string_wide_from_utf8_fast wname(m_host->get_script_engine());
	pfc::stringcvt::string_wide_from_utf8_fast wcode(m_host->get_script_code());
	script_preprocessor preprocessor(wcode.get_ptr());
	preprocessor.process_script_info(m_host->ScriptInfo());

	hr = InitScriptEngineByName(wname);

	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptSite(this);
	if (SUCCEEDED(hr)) hr = m_script_engine->QueryInterface(&parser);
	if (SUCCEEDED(hr)) hr = parser->InitNew();

	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"window", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"gdi", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"fb", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"utils", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"wsh_utils", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"plman", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"console", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (SUCCEEDED(hr)) hr = m_script_engine->GetScriptDispatch(NULL, &m_script_root);
	if (SUCCEEDED(hr)) hr = ProcessImportedScripts(preprocessor, parser);

	DWORD source_context = 0;
	if (SUCCEEDED(hr)) hr = GenerateSourceContext(NULL, wcode, source_context);
	m_contextToPathMap[source_context] = "<main>";

	if (SUCCEEDED(hr)) hr = parser->ParseScriptText(wcode.get_ptr(), NULL, NULL, NULL, source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);

	if (SUCCEEDED(hr))
	{
		m_engine_inited = true;
	}
	else
	{
		m_engine_inited = false;
		m_has_error = true;
	}

	m_callback_invoker.init(m_script_root);
	return hr;
}

HRESULT ScriptHost::InvokeCallback(int callbackId, VARIANTARG* argv, UINT argc, VARIANT* ret)
{
	if (HasError()) return E_FAIL;
	if (!Ready()) return E_FAIL;

	HRESULT hr = E_FAIL;

	try
	{
		hr = m_callback_invoker.invoke(callbackId, argv, argc, ret);
	}
	catch (std::exception& e)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(JSP_NAME " (%s): Unhandled C++ Exception: \"%s\", will crash now...", m_host->ScriptInfo().build_info_string().get_ptr(), e.what());
	}
	catch (_com_error& e)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(JSP_NAME " (%s): Unhandled COM Error: \"%s\", will crash now...", m_host->ScriptInfo().build_info_string().get_ptr(), pfc::stringcvt::string_utf8_from_wide(e.ErrorMessage()).get_ptr());
	}
	catch (...)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(JSP_NAME " (%s): Unhandled Unknown Exception, will crash now...", m_host->ScriptInfo().build_info_string().get_ptr());
	}

	return hr;
}

HRESULT ScriptHost::ProcessImportedScripts(script_preprocessor& preprocessor, IActiveScriptParsePtr& parser)
{
	// processing "@import"
	script_preprocessor::t_script_list scripts;
	HRESULT hr = preprocessor.process_import(m_host->ScriptInfo(), scripts);

	for (t_size i = 0; i < scripts.get_count(); ++i)
	{
		DWORD source_context;

		if (SUCCEEDED(hr)) hr = GenerateSourceContext(scripts[i].path.get_ptr(), scripts[i].code.get_ptr(), source_context);
		if (FAILED(hr)) break;

		m_contextToPathMap[source_context] = pfc::stringcvt::string_utf8_from_wide(scripts[i].path.get_ptr());
		hr = parser->ParseScriptText(scripts[i].code.get_ptr(), NULL, NULL, NULL, source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);
	}

	return hr;
}

void ScriptHost::Finalize()
{
    InvokeCallback( CallbackIds::on_script_unload );

    if ( Ready() )
    {
        // Call GC explicitly
    }
}

*/
