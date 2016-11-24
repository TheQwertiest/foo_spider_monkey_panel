#pragma once

#include "script_preprocessor.h"
#include "script_interface_impl.h"
#include "script_interface_playlist_impl.h"
#include "script_interface_tooltip_impl.h"
#include "config.h"
#include "user_message.h"
#include "global_cfg.h"
#include "IDropSourceImpl.h"
#include "host_droptarget.h"
#include "host_timer_dispatcher.h"
#include "script_callback_invoker.h"


// Smart pointers for Active Scripting
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, IID_IActiveScriptParse);
_COM_SMARTPTR_TYPEDEF(IProcessDebugManager, IID_IProcessDebugManager);
_COM_SMARTPTR_TYPEDEF(IDebugDocumentHelper, IID_IDebugDocumentHelper);
_COM_SMARTPTR_TYPEDEF(IDebugApplication, IID_IDebugApplication);


class HostComm : public js_panel_vars
{
protected:
	HWND m_hwnd;
	INT m_width;
	INT m_height;
	POINT m_max_size;
	POINT m_min_size;
	UINT m_dlg_code;
	HDC m_hdc;
	HBITMAP m_gr_bmp;
	HBITMAP m_gr_bmp_bk;
	UINT m_accuracy;
	t_script_info m_script_info;
	ui_selection_holder::ptr m_selection_holder;
	int m_instance_type;
	bool m_suppress_drawing;
	bool m_paint_pending;
	HostTimerDispatcher m_host_timer_dispatcher;
	panel_tooltip_param_ptr m_panel_tooltip_param_ptr;

	HostComm();
	virtual ~HostComm();

public:
	enum
	{
		KInstanceTypeCUI = 0,
		KInstanceTypeDUI,
	};

	GUID GetGUID() { return get_config_guid(); }
	IGdiBitmap* GetBackgroundImage();
	HDC GetHDC() { return m_hdc; }
	HWND GetHWND() { return m_hwnd; }
	INT GetHeight() { return m_height; }
	INT GetWidth() { return m_width; }
	POINT& MaxSize() { return m_max_size; }
	POINT& MinSize() { return m_min_size; }
	UINT& DlgCode() { return m_dlg_code; }
	UINT GetInstanceType() { return m_instance_type; }
	panel_tooltip_param_ptr& PanelTooltipParam() { return m_panel_tooltip_param_ptr; }
	t_script_info& ScriptInfo() { return m_script_info; }
	void PreserveSelection() { m_selection_holder = static_api_ptr_t<ui_selection_manager>()->acquire(); }
	unsigned SetInterval(IDispatch* func, INT delay);
	unsigned SetTimeout(IDispatch* func, INT delay);
	virtual DWORD GetColorCUI(unsigned type, const GUID& guid) = 0;
	virtual DWORD GetColorDUI(unsigned type) = 0;
	virtual HFONT GetFontCUI(unsigned type, const GUID& guid) = 0;
	virtual HFONT GetFontDUI(unsigned type) = 0;
	void ClearIntervalOrTimeout(UINT timerId);
	void Redraw();
	void RefreshBackground(LPRECT lprcUpdate = NULL);
	void Repaint(bool force = false);
	void RepaintRect(UINT x, UINT y, UINT w, UINT h, bool force = false);
};

class FbWindow : public IDispatchImpl3<IFbWindow>
{
protected:
	FbWindow(HostComm* p) : m_host(p)
	{
	}

	virtual ~FbWindow()
	{
	}

public:
	STDMETHODIMP ClearInterval(UINT intervalID);
	STDMETHODIMP ClearTimeout(UINT timeoutID);
	STDMETHODIMP CreatePopupMenu(IMenuObj** pp);
	STDMETHODIMP CreateThemeManager(BSTR classid, IThemeManager** pp);
	STDMETHODIMP CreateTooltip(BSTR name, float pxSize, INT style, IFbTooltip** pp);
	STDMETHODIMP GetBackgroundImage(IGdiBitmap** pp);
	STDMETHODIMP GetColorCUI(UINT type, BSTR guidstr, int* p);
	STDMETHODIMP GetColorDUI(UINT type, int* p);
	STDMETHODIMP GetFontCUI(UINT type, BSTR guidstr, IGdiFont** pp);
	STDMETHODIMP GetFontDUI(UINT type, IGdiFont** pp);
	STDMETHODIMP GetProperty(BSTR name, VARIANT defaultval, VARIANT* p);
	STDMETHODIMP NotifyOthers(BSTR name, VARIANT info);
	STDMETHODIMP Reload();
	STDMETHODIMP Repaint(VARIANT_BOOL force);
	STDMETHODIMP RepaintRect(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL force);
	STDMETHODIMP SetCursor(UINT id);
	STDMETHODIMP SetInterval(IDispatch* func, INT delay, UINT* outIntervalID);
	STDMETHODIMP SetProperty(BSTR name, VARIANT val);
	STDMETHODIMP SetTimeout(IDispatch* func, INT delay, UINT* outTimeoutID);
	STDMETHODIMP ShowConfigure();
	STDMETHODIMP ShowProperties();
	STDMETHODIMP get_DlgCode(UINT* p);
	STDMETHODIMP get_Height(INT* p);
	STDMETHODIMP get_ID(UINT* p);
	STDMETHODIMP get_InstanceType(UINT* p);
	STDMETHODIMP get_IsTransparent(VARIANT_BOOL* p);
	STDMETHODIMP get_IsVisible(VARIANT_BOOL* p);
	STDMETHODIMP get_MaxHeight(UINT* p);
	STDMETHODIMP get_MaxWidth(UINT* p);
	STDMETHODIMP get_MinHeight(UINT* p);
	STDMETHODIMP get_MinWidth(UINT* p);
	STDMETHODIMP get_Width(INT* p);
	STDMETHODIMP put_DlgCode(UINT code);
	STDMETHODIMP put_MaxHeight(UINT height);
	STDMETHODIMP put_MaxWidth(UINT width);
	STDMETHODIMP put_MinHeight(UINT height);
	STDMETHODIMP put_MinWidth(UINT width);

private:
	HostComm* m_host;
};

class ScriptHost :
	public IActiveScriptSite,
	public IActiveScriptSiteWindow
{
public:
	ScriptHost(HostComm* host);
	virtual ~ScriptHost();

	STDMETHODIMP EnableModeless(BOOL fEnable);
	STDMETHODIMP GetDocVersionString(BSTR* pstr);
	STDMETHODIMP GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** ppunk, ITypeInfo** ppti);
	STDMETHODIMP GetLCID(LCID* plcid);
	STDMETHODIMP GetWindow(HWND* phwnd);
	STDMETHODIMP OnEnterScript();
	STDMETHODIMP OnLeaveScript();
	STDMETHODIMP OnScriptError(IActiveScriptError* err);
	STDMETHODIMP OnScriptTerminate(const VARIANT* result, const EXCEPINFO* excep);
	STDMETHODIMP OnStateChange(SCRIPTSTATE state);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	HRESULT GenerateSourceContext(const wchar_t* path, const wchar_t* code, DWORD& source_context);
	HRESULT InitScriptEngineByName(const wchar_t* engineName);
	HRESULT Initialize();
	HRESULT InvokeCallback(int callbackId, VARIANTARG* argv = NULL, UINT argc = 0, VARIANT* ret = NULL);
	HRESULT ProcessImportedScripts(script_preprocessor& preprocessor, IActiveScriptParsePtr& parser);
	bool HasError() { return m_has_error; }
	bool Ready() { return m_engine_inited && m_script_engine; }

	void Stop()
	{
		m_engine_inited = false;
		if (m_script_engine) m_script_engine->SetScriptState(SCRIPTSTATE_DISCONNECTED);
	}

	void Finalize();
	void ReportError(IActiveScriptError* err);

private:
	volatile DWORD m_dwRef;
	HostComm* m_host;
	IFbWindowPtr m_window;
	IGdiUtilsPtr m_gdi;
	IFbUtilsPtr m_fb2k;
	IJSUtilsPtr m_utils;
	IFbPlaylistManagerPtr m_playlistman;
	pfc::tickcount_t m_dwStartTime;

	IActiveScriptPtr m_script_engine;
	IDispatchPtr m_script_root;

	bool m_engine_inited;
	bool m_has_error;

	typedef pfc::map_t<DWORD, pfc::string8> contextToPathMap;
	contextToPathMap m_contextToPathMap;
	DWORD m_lastSourceContext;

	ScriptCallbackInvoker m_callback_invoker;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSiteWindow)
	END_COM_QI_IMPL()
};
