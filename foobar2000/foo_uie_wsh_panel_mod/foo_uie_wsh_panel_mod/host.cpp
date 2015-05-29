#include "stdafx.h"
#include "host.h"
#include "resource.h"
#include "helpers.h"
#include "panel_manager.h"
#include "global_cfg.h"
#include "popup_msg.h"
#include "dbgtrace.h"
#include "obsolete.h"


HostComm::HostComm() 
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
}

HostComm::~HostComm()
{
}

void HostComm::Redraw()
{
	m_paint_pending = false;
	RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void HostComm::Repaint(bool force /*= false*/)
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

void HostComm::RepaintRect(UINT x, UINT y, UINT w, UINT h, bool force /*= false*/)
{
	RECT rc = {x, y, x + w, y + h};

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

void HostComm::RefreshBackground(LPRECT lprcUpdate /*= NULL*/)
{
	HWND wnd_parent = GetAncestor(m_hwnd, GA_PARENT);

	if (!wnd_parent || IsIconic(core_api::get_main_window()) || !IsWindowVisible(m_hwnd))
		return;

	HDC dc_parent = GetDC(wnd_parent);
	HDC hdc_bk = CreateCompatibleDC(dc_parent);
	POINT pt = {0, 0};
	RECT rect_child = {0, 0, m_width, m_height};
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
	BitBlt(hdc_bk, rect_child.left, rect_child.top, rect_child.right - rect_child.left, rect_child.bottom - rect_child.top, 
		dc_parent, pt.x, pt.y, SRCCOPY);

	SelectBitmap(hdc_bk, old_bmp);
	DeleteDC(hdc_bk);
	ReleaseDC(wnd_parent, dc_parent);
	DeleteRgn(rgn_child);
	SetWindowRgn(m_hwnd, NULL, FALSE);
	m_suppress_drawing = false;
	SendMessage(m_hwnd, UWM_REFRESHBKDONE, 0, 0);
	if (get_edge_style()) SendMessage(m_hwnd, WM_NCPAINT, 1, 0);
	Repaint(true);
}

unsigned HostComm::SetTimeout(IDispatch * func, INT delay)
{
	return m_host_timer_dispatcher.setTimeout(delay, func);
}

unsigned HostComm::SetInterval(IDispatch * func, INT delay)
{
	return m_host_timer_dispatcher.setInterval(delay, func);
}

void HostComm::ClearIntervalOrTimeout(UINT timerId)
{
	m_host_timer_dispatcher.kill(timerId);
}

IGdiBitmap * HostComm::GetBackgroundImage()
{
	Gdiplus::Bitmap * bitmap = NULL;
	IGdiBitmap * ret = NULL;

	if (get_pseudo_transparent())
	{
		bitmap = Gdiplus::Bitmap::FromHBITMAP(m_gr_bmp_bk, NULL);

		if (!helpers::ensure_gdiplus_object(bitmap))
		{
			if (bitmap) delete bitmap;
			bitmap = NULL;
		}
	}

	if (bitmap) ret = new com_object_impl_t<GdiBitmap>(bitmap);

	return ret;
}

void CALLBACK HostComm::g_timer_proc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HWND wnd = reinterpret_cast<HWND>(dwUser);

	SendMessage(wnd, UWM_TIMER, uTimerID, 0);
}


STDMETHODIMP FbWindow::get_ID(UINT* p)
{
	TRACK_FUNCTION();

	if (!p ) return E_POINTER;

	*p = (UINT)m_host->GetHWND();
	return S_OK;
}

STDMETHODIMP FbWindow::get_Width(INT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetWidth();
	return S_OK;
}

STDMETHODIMP FbWindow::get_Height(INT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetHeight();
	return S_OK;
}

STDMETHODIMP FbWindow::get_InstanceType(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	*p = m_host->GetInstanceType();
	return S_OK;
}

STDMETHODIMP FbWindow::get_MaxWidth(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->MaxSize().x;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MaxWidth(UINT width)
{
	TRACK_FUNCTION();

	m_host->MaxSize().x = width;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_maximum_width);
	return S_OK;
}

STDMETHODIMP FbWindow::get_MaxHeight(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->MaxSize().y;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MaxHeight(UINT height)
{
	TRACK_FUNCTION();

	m_host->MaxSize().y = height;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_maximum_height);
	return S_OK;
}

STDMETHODIMP FbWindow::get_MinWidth(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->MinSize().x;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MinWidth(UINT width)
{
	TRACK_FUNCTION();

	m_host->MinSize().x = width;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_minimum_width);
	return S_OK;
}

STDMETHODIMP FbWindow::get_MinHeight(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->MinSize().y;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MinHeight(UINT height)
{
	TRACK_FUNCTION();

	m_host->MinSize().y = height;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_minimum_height);
	return S_OK;
}

STDMETHODIMP FbWindow::get_DlgCode(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->DlgCode();
	return S_OK;
}

STDMETHODIMP FbWindow::put_DlgCode(UINT code)
{
	TRACK_FUNCTION();

	m_host->DlgCode() = code;
	return S_OK;
}

STDMETHODIMP FbWindow::get_IsTransparent(VARIANT_BOOL* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(m_host->get_pseudo_transparent());
	return S_OK;
}

STDMETHODIMP FbWindow::get_IsVisible(VARIANT_BOOL* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(IsWindowVisible(m_host->GetHWND()));
	return S_OK;
}

STDMETHODIMP FbWindow::Repaint(VARIANT_BOOL force)
{
	TRACK_FUNCTION();

	m_host->Repaint(force != FALSE);
	return S_OK;
}

STDMETHODIMP FbWindow::RepaintRect(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL force)
{
	TRACK_FUNCTION();

	m_host->RepaintRect(x, y, w, h, force != FALSE);
	return S_OK;
}

STDMETHODIMP FbWindow::CreatePopupMenu(IMenuObj ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = new com_object_impl_t<MenuObj>(m_host->GetHWND());
	return S_OK;
}

STDMETHODIMP FbWindow::SetInterval(IDispatch * func, INT delay, UINT * outIntervalID)
{
	TRACK_FUNCTION();
	if (!outIntervalID) return E_POINTER;
	(*outIntervalID) = m_host->SetInterval(func, delay);
	return S_OK;
}

STDMETHODIMP FbWindow::ClearInterval(UINT intervalID)
{
	TRACK_FUNCTION();
	m_host->ClearIntervalOrTimeout(intervalID);
	return S_OK;
}

STDMETHODIMP FbWindow::SetTimeout(IDispatch * func, INT delay, UINT * outTimeoutID)
{
	TRACK_FUNCTION();
	(*outTimeoutID) = m_host->SetTimeout(func, delay);
	return S_OK;
}

STDMETHODIMP FbWindow::ClearTimeout(UINT timeoutID)
{
	TRACK_FUNCTION();
	m_host->ClearIntervalOrTimeout(timeoutID);
	return S_OK;
}

STDMETHODIMP FbWindow::NotifyOthers(BSTR name, VARIANT info)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (info.vt & VT_BYREF) return E_INVALIDARG;

	HRESULT hr = S_OK;
	_variant_t var;

	hr = VariantCopy(&var, &info);

	if (FAILED(hr)) return hr;

	simple_callback_data_2<_bstr_t, _variant_t> * notify_data 
		= new simple_callback_data_2<_bstr_t, _variant_t>(name, NULL);

	notify_data->m_item2.Attach(var.Detach());

	panel_manager::instance().send_msg_to_others_pointer(m_host->GetHWND(), 
		CALLBACK_UWM_NOTIFY_DATA, notify_data);

	return S_OK;
}

STDMETHODIMP FbWindow::CreateTooltip(IFbTooltip ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	auto no_background = (m_host->ScriptInfo().tooltip_mask & t_script_info::kTooltipCustomPaintNoBackground) != 0;
	const auto& tooltip_param = m_host->PanelTooltipParam();
	(*pp) = new com_object_impl_t<FbTooltip>(m_host->GetHWND(), no_background, tooltip_param);
	return S_OK;
}

STDMETHODIMP FbWindow::ShowConfigure()
{
	TRACK_FUNCTION();

	PostMessage(m_host->GetHWND(), UWM_SHOWCONFIGURE, 0, 0);
	return S_OK;
}

STDMETHODIMP FbWindow::ShowProperties()
{
	TRACK_FUNCTION();

	PostMessage(m_host->GetHWND(), UWM_SHOWPROPERTIES, 0, 0);
	return S_OK;
}


STDMETHODIMP FbWindow::GetProperty(BSTR name, VARIANT defaultval, VARIANT * p)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!p) return E_POINTER;

	HRESULT hr;
	_variant_t var;
	pfc::stringcvt::string_utf8_from_wide uname(name);

	if (m_host->get_config_prop().get_config_item(uname, var))
	{
		hr = VariantCopy(p, &var);
	}
	else
	{
		m_host->get_config_prop().set_config_item(uname, defaultval);
		hr = VariantCopy(p, &defaultval);
	}

	if (FAILED(hr))
		p = NULL;

	return S_OK;
}

STDMETHODIMP FbWindow::SetProperty(BSTR name, VARIANT val)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;

	m_host->get_config_prop().set_config_item(pfc::stringcvt::string_utf8_from_wide(name), val);
	return S_OK;
}

STDMETHODIMP FbWindow::GetBackgroundImage(IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = m_host->GetBackgroundImage();
	return S_OK;
}

STDMETHODIMP FbWindow::SetCursor(UINT id)
{
	TRACK_FUNCTION();

	::SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(id)));
	return S_OK;
}

STDMETHODIMP FbWindow::GetColorCUI(UINT type, BSTR guidstr, int * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (!guidstr) return E_INVALIDARG;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeCUI) return E_NOTIMPL;

	GUID guid;

	if (!*guidstr)
	{
		memcpy(&guid, &pfc::guid_null, sizeof(guid));
	}
	else
	{
		if (CLSIDFromString(guidstr, &guid) != NOERROR)
		{
			return E_INVALIDARG;
		}
	}

	*p = m_host->GetColorCUI(type, guid);
	return S_OK;
}

STDMETHODIMP FbWindow::GetFontCUI(UINT type, BSTR guidstr, IGdiFont ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!guidstr) return E_INVALIDARG;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeCUI) return E_NOTIMPL;

	GUID guid;

	if (!*guidstr)
	{
		memcpy(&guid, &pfc::guid_null, sizeof(guid));
	}
	else
	{
		if (CLSIDFromString(guidstr, &guid) != NOERROR)
		{
			return E_INVALIDARG;
		}
	}

	HFONT hFont = m_host->GetFontCUI(type, guid);

	*pp = NULL;

	if (hFont)
	{
		Gdiplus::Font * font = new Gdiplus::Font(m_host->GetHDC(), hFont);

		if (!helpers::ensure_gdiplus_object(font))
		{
			if (font) delete font;
			(*pp) = NULL;
			return S_OK;
		}

		*pp = new com_object_impl_t<GdiFont>(font, hFont);
	}

	return S_OK;
}

STDMETHODIMP FbWindow::GetColorDUI(UINT type, int * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeDUI) return E_NOTIMPL;

	*p = m_host->GetColorDUI(type);
	return S_OK;
}

STDMETHODIMP FbWindow::GetFontDUI(UINT type, IGdiFont ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeDUI) return E_NOTIMPL;

	HFONT hFont = m_host->GetFontDUI(type);
	*pp = NULL;

	if (hFont)
	{
		Gdiplus::Font * font = new Gdiplus::Font(m_host->GetHDC(), hFont);

		if (!helpers::ensure_gdiplus_object(font))
		{
			if (font) delete font;
			(*pp) = NULL;
			return S_OK;
		}

		*pp = new com_object_impl_t<GdiFont>(font, hFont, false);
	}

	return S_OK;
}

STDMETHODIMP FbWindow::CreateThemeManager(BSTR classid, IThemeManager ** pp)
{
	TRACK_FUNCTION();

	if (!classid) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	IThemeManager * ptheme = NULL;

	try
	{
		ptheme = new com_object_impl_t<ThemeManager>(m_host->GetHWND(), classid);
	}
	catch (pfc::exception_invalid_params &)
	{
		if (ptheme)
		{
			ptheme->Dispose();
			delete ptheme;
			ptheme = NULL;
		}
	}

	*pp = ptheme;
	return S_OK;
}

STDMETHODIMP FbWindow::Reload()
{
	TRACK_FUNCTION();

	PostMessage(m_host->GetHWND(), UWM_RELOAD, 0, 0);
	return S_OK;
}

ScriptHost::ScriptHost(HostComm * host) 
	: m_host(host)
	, m_window(new com_object_impl_t<FbWindow, false>(host))
	, m_gdi(com_object_singleton_t<GdiUtils>::instance())
	, m_fb2k(com_object_singleton_t<FbUtils>::instance())
	, m_utils(com_object_singleton_t<WSHUtils>::instance())
	, m_playlistman(com_object_singleton_t<FbPlaylistManager>::instance())
	, m_dwStartTime(0)
	, m_dwRef(1)
	, m_engine_inited(false)
	, m_has_error(false)
	, m_lastSourceContext(0)
{
}

ScriptHost::~ScriptHost()
{
}

STDMETHODIMP_(ULONG) ScriptHost::AddRef()
{
	return InterlockedIncrement(&m_dwRef);
}

STDMETHODIMP_(ULONG) ScriptHost::Release()
{
	ULONG n = InterlockedDecrement(&m_dwRef); 

	if (n == 0)
	{
		delete this;
	}

	return n;
}

STDMETHODIMP ScriptHost::GetLCID(LCID* plcid)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** ppunk, ITypeInfo** ppti)
{
	if (ppti) *ppti = NULL;

	if (ppunk) *ppunk = NULL;

	if (mask & SCRIPTINFO_IUNKNOWN)
	{
		if (!name) return E_INVALIDARG;
		if (!ppunk) return E_POINTER;

		if (wcscmp(name, L"window") == 0)
		{
			(*ppunk) = m_window;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"gdi") == 0)
		{
			(*ppunk) = m_gdi;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"fb") == 0)
		{
			(*ppunk) = m_fb2k;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"utils") == 0)
		{
			(*ppunk) = m_utils;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"plman") == 0)
		{
			(*ppunk) = m_playlistman;
			(*ppunk)->AddRef();
			return S_OK;
		}
	}

	return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP ScriptHost::GetDocVersionString(BSTR* pstr)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::OnScriptTerminate(const VARIANT* result, const EXCEPINFO* excep)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::OnStateChange(SCRIPTSTATE state)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::OnScriptError(IActiveScriptError* err)
{
	m_has_error = true;

	if (!err) return E_POINTER;

	ReportError(err);
	return S_OK;
}

STDMETHODIMP ScriptHost::OnEnterScript()
{
	m_dwStartTime = GetTickCount();
	return S_OK;
}

STDMETHODIMP ScriptHost::OnLeaveScript()
{
	return S_OK;
}

STDMETHODIMP ScriptHost::GetWindow(HWND *phwnd)
{
	*phwnd = m_host->GetHWND();

	return S_OK;
}

STDMETHODIMP ScriptHost::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

HRESULT ScriptHost::Initialize()
{
	Finalize();

	m_has_error = false;

	HRESULT hr = S_OK;
	IActiveScriptParsePtr parser;
	pfc::stringcvt::string_wide_from_utf8_fast wname(m_host->get_script_engine());
	pfc::stringcvt::string_wide_from_utf8_fast wcode(m_host->get_script_code());
	// Load preprocessor module
	script_preprocessor preprocessor(wcode.get_ptr());
	preprocessor.process_script_info(m_host->ScriptInfo());

	hr = InitScriptEngineByName(wname);

	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptSite(this);
	if (SUCCEEDED(hr)) hr = m_script_engine->QueryInterface(&parser);
	if (SUCCEEDED(hr)) hr = parser->InitNew();

	EnableSafeModeToScriptEngine(m_script_engine, g_cfg_safe_mode);

	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"window", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"gdi", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"fb", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"utils", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"plman", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (SUCCEEDED(hr)) hr = m_script_engine->GetScriptDispatch(NULL, &m_script_root);
	// Parse imported scripts
	if (SUCCEEDED(hr)) hr = ProcessImportedScripts(preprocessor, parser);

	// Parse main script
	DWORD source_context = 0;
	if (SUCCEEDED(hr)) hr = GenerateSourceContext(NULL, wcode, source_context);
	m_contextToPathMap[source_context] = "<main>";

	if (SUCCEEDED(hr)) hr = parser->ParseScriptText(wcode.get_ptr(), NULL, NULL, NULL, 
		source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);

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

void ScriptHost::EnableSafeModeToScriptEngine(IActiveScript * engine, bool enable)
{
	if (!enable || !engine) return;

	_COM_SMARTPTR_TYPEDEF(IObjectSafety, IID_IObjectSafety);
	IObjectSafetyPtr psafe;

	if (SUCCEEDED(engine->QueryInterface(&psafe)))
	{
		psafe->SetInterfaceSafetyOptions(IID_IDispatch, 
			INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER);
	}
}

HRESULT ScriptHost::ProcessImportedScripts(script_preprocessor &preprocessor, IActiveScriptParsePtr &parser)
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
		hr = parser->ParseScriptText(scripts[i].code.get_ptr(), NULL, NULL, NULL, 
			source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);
	}

	return hr;
}

HRESULT ScriptHost::InitScriptEngineByName(const wchar_t * engineName)
{
	HRESULT hr = E_FAIL;
	const DWORD classContext = CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER;
	const wchar_t jscriptName[] = L"JScript";
	bool isJScript = wcsncmp(engineName, jscriptName, _countof(jscriptName) - 1) == 0;
	bool isJScript9 = wcscmp(engineName, L"JScript9") == 0;

	if (isJScript9) 
	{
		// Try using JScript9 from IE9
		// {16d51579-a30b-4c8b-a276-0ff4dc41e755}
		static const CLSID jscript9clsid = 
		{0x16d51579, 0xa30b, 0x4c8b, {0xa2, 0x76, 0x0f, 0xf4, 0xdc, 0x41, 0xe7, 0x55 } };

		if (FAILED(hr = m_script_engine.CreateInstance(jscript9clsid, NULL, classContext)))
		{
			// fallback to default JScript engine.
			engineName = L"JScript";
		}
	}

	if (FAILED(hr)) 
	{
		hr = m_script_engine.CreateInstance(engineName, NULL, classContext);
	}

	if (FAILED(hr)) 
	{
		return hr;
	}

	// In order to support new features after JScript 5.8
	if (isJScript)
	{
		IActiveScriptProperty *pActScriProp = NULL;
		
		if (SUCCEEDED(m_script_engine->QueryInterface(IID_IActiveScriptProperty, (void **)&pActScriProp)))
		{
			VARIANT scriptLangVersion;
			scriptLangVersion.vt = VT_I4;
			scriptLangVersion.lVal = SCRIPTLANGUAGEVERSION_5_8;
			pActScriProp->SetProperty(SCRIPTPROP_INVOKEVERSIONING, NULL, &scriptLangVersion);
			pActScriProp->Release();
		}
	}

	return hr;
}

void ScriptHost::Finalize()
{
	InvokeCallback(CallbackIds::on_script_unload);

	if (Ready())
	{
		// Call GC explicitly 
		IActiveScriptGarbageCollector * gc = NULL;
		if (SUCCEEDED(m_script_engine->QueryInterface(IID_IActiveScriptGarbageCollector, (void **)&gc)))
		{
			gc->CollectGarbage(SCRIPTGCTYPE_EXHAUSTIVE);
			gc->Release();
		}

		m_script_engine->SetScriptState(SCRIPTSTATE_DISCONNECTED);
		m_script_engine->SetScriptState(SCRIPTSTATE_CLOSED);
		m_script_engine->Close();
		//m_script_engine->InterruptScriptThread(SCRIPTTHREADID_ALL, NULL, 0);
		m_engine_inited = false;
	}

	m_contextToPathMap.remove_all();
	m_callback_invoker.reset();

	if (m_script_engine)
	{
		m_script_engine.Release();
	}

	if (m_script_root)
	{
		m_script_root.Release();
	}
}

HRESULT ScriptHost::InvokeCallback(int callbackId, VARIANTARG * argv /*= NULL*/, UINT argc /*= 0*/, VARIANT * ret /*= NULL*/)
{
	if (HasError()) return E_FAIL;
	if (!Ready()) return E_FAIL;
	
	HRESULT hr = E_FAIL;

	try
	{
		hr = m_callback_invoker.invoke(callbackId, argv, argc, ret);
	}
	catch (std::exception & e)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(WSPM_NAME " (%s): Unhandled C++ Exception: \"%s\", will crash now...", 
			m_host->ScriptInfo().build_info_string().get_ptr(), e.what());
		PRINT_DISPATCH_TRACK_MESSAGE_AND_BREAK();
	}
	catch (_com_error & e)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(WSPM_NAME " (%s): Unhandled COM Error: \"%s\", will crash now...", 
			m_host->ScriptInfo().build_info_string().get_ptr(), 
			pfc::stringcvt::string_utf8_from_wide(e.ErrorMessage()).get_ptr());
		PRINT_DISPATCH_TRACK_MESSAGE_AND_BREAK();
	}
	catch (...)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(WSPM_NAME " (%s): Unhandled Unknown Exception, will crash now...", 
			m_host->ScriptInfo().build_info_string().get_ptr());
		PRINT_DISPATCH_TRACK_MESSAGE_AND_BREAK();
	}

	return hr;
}

HRESULT ScriptHost::GenerateSourceContext(const wchar_t * path, const wchar_t * code, DWORD & source_context)
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

void ScriptHost::ReportError(IActiveScriptError* err)
{
	if (!err) return;

	DWORD ctx = 0;
	ULONG line = 0;
	LONG  charpos = 0;
	EXCEPINFO excep = { 0 };
	//WCHAR buf[512] = { 0 };
	_bstr_t sourceline;
	_bstr_t name;

	if (FAILED(err->GetSourcePosition(&ctx, &line, &charpos)))
	{
		line = 0;
		charpos = 0;
	}

	if (FAILED(err->GetSourceLineText(sourceline.GetAddress())))
	{
		sourceline = L"<source text only available at compile time>";
	}

	if (FAILED(err->GetExceptionInfo(&excep)))
		return;

	// Do a deferred fill-in if necessary
	if (excep.pfnDeferredFillIn)
		(*excep.pfnDeferredFillIn)(&excep);

	using namespace pfc::stringcvt;
	pfc::string_formatter formatter;
	formatter << WSPM_NAME << " (" << m_host->ScriptInfo().build_info_string().get_ptr() << "): ";

	if (excep.bstrSource && excep.bstrDescription) 
	{
		formatter << string_utf8_from_wide(excep.bstrSource) << ":\n";
		formatter << string_utf8_from_wide(excep.bstrDescription) << "\n";
	}
	else
	{
		pfc::string8_fast errorMessage;

		if (uFormatSystemErrorMessage(errorMessage, excep.scode))
			formatter << errorMessage;
		else
			formatter << "Unknown error code: 0x" << pfc::format_hex_lowercase((unsigned)excep.scode);
	}

	if (m_contextToPathMap.exists(ctx))
	{
		formatter << "File: " << m_contextToPathMap[ctx] << "\n";
	}

	formatter << "Line: " << (t_uint32)(line + 1) << ", Col: " << (t_uint32)(charpos + 1) << "\n";
	formatter << string_utf8_from_wide(sourceline);
	if (name.length() > 0) formatter << "\nAt: " << name;

	if (excep.bstrSource)      SysFreeString(excep.bstrSource);
	if (excep.bstrDescription) SysFreeString(excep.bstrDescription);
	if (excep.bstrHelpFile)    SysFreeString(excep.bstrHelpFile);

	console::error(formatter);
	popup_msg::g_show(formatter, WSPM_NAME, popup_message::icon_error);
	MessageBeep(MB_ICONASTERISK);
	SendMessage(m_host->GetHWND(), UWM_SCRIPT_ERROR, 0, 0);
}

