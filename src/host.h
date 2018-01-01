#pragma once

#include "script_preprocessor.h"
#include "script_interface_impl.h"
#include "config.h"
#include "user_message.h"
#include "host_drop_target.h"
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
	HostComm();
	virtual ~HostComm();

	HBITMAP m_gr_bmp;
	HBITMAP m_gr_bmp_bk;
	HDC m_hdc;
	HWND m_hwnd;
	HostTimerDispatcher m_host_timer_dispatcher;
	INT m_height;
	INT m_width;
	POINT m_max_size;
	POINT m_min_size;
	UINT m_accuracy;
	UINT m_dlg_code;
	bool m_paint_pending;
	bool m_suppress_drawing;
	int m_instance_type;
	panel_tooltip_param_ptr m_panel_tooltip_param_ptr;
	t_script_info m_script_info;
	ui_selection_holder::ptr m_selection_holder;

public:
	enum
	{
		KInstanceTypeCUI = 0,
		KInstanceTypeDUI,
	};

	GUID GetGUID();
	HDC GetHDC();
	HWND GetHWND();
	INT GetHeight();
	INT GetWidth();
	POINT& MaxSize();
	POINT& MinSize();
	UINT& DlgCode();
	UINT GetInstanceType();
	panel_tooltip_param_ptr& PanelTooltipParam();
	t_script_info& ScriptInfo();
	unsigned SetInterval(IDispatch* func, INT delay);
	unsigned SetTimeout(IDispatch* func, INT delay);
	virtual DWORD GetColourCUI(unsigned type, const GUID& guid) = 0;
	virtual DWORD GetColourDUI(unsigned type) = 0;
	virtual HFONT GetFontCUI(unsigned type, const GUID& guid) = 0;
	virtual HFONT GetFontDUI(unsigned type) = 0;
	void ClearIntervalOrTimeout(UINT timerId);
	void PreserveSelection();
	void Redraw();
	void RefreshBackground(LPRECT lprcUpdate = NULL);
	void Repaint(bool force = false);
	void RepaintRect(LONG x, LONG y, LONG w, LONG h, bool force = false);
};

class ScriptHost : public IActiveScriptSite, public IActiveScriptSiteWindow
{
public:
	ScriptHost(HostComm* host);
	virtual ~ScriptHost();

	bool HasError();
	bool Ready();

	HRESULT GenerateSourceContext(const wchar_t* path, const wchar_t* code, DWORD& source_context);
	HRESULT InitScriptEngineByName(const wchar_t* engineName);
	HRESULT Initialize();
	HRESULT InvokeCallback(int callbackId, VARIANTARG* argv = NULL, UINT argc = 0, VARIANT* ret = NULL);
	HRESULT ProcessImportedScripts(script_preprocessor& preprocessor, IActiveScriptParsePtr& parser);

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

	void Finalize();
	void ReportError(IActiveScriptError* err);
	void Stop();

private:
	DWORD m_lastSourceContext;
	HostComm* m_host;
	IActiveScriptPtr m_script_engine;
	IDispatchPtr m_script_root;
	IFbPlaylistManagerPtr m_playlistman;
	IFbUtilsPtr m_fb2k;
	IFbWindowPtr m_window;
	IGdiUtilsPtr m_gdi;
	IJSUtilsPtr m_utils;
	IJSConsolePtr m_console;
	ScriptCallbackInvoker m_callback_invoker;
	bool m_engine_inited;
	bool m_has_error;
	pfc::map_t<DWORD, pfc::string8> m_contextToPathMap;
	pfc::tickcount_t m_dwStartTime;
	volatile DWORD m_dwRef;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSiteWindow)
	END_COM_QI_IMPL()
};

class FbWindow : public IDispatchImpl3<IFbWindow>
{
protected:
	FbWindow(HostComm* p);
	virtual ~FbWindow();

public:
	STDMETHODIMP ClearInterval(UINT intervalID);
	STDMETHODIMP ClearTimeout(UINT timeoutID);
	STDMETHODIMP CreatePopupMenu(IMenuObj** pp);
	STDMETHODIMP CreateThemeManager(BSTR classid, IThemeManager** pp);
	STDMETHODIMP CreateTooltip(BSTR name, float pxSize, INT style, IFbTooltip** pp);
	STDMETHODIMP GetColourCUI(UINT type, BSTR guidstr, int* p);
	STDMETHODIMP GetColourDUI(UINT type, int* p);
	STDMETHODIMP GetFontCUI(UINT type, BSTR guidstr, IGdiFont** pp);
	STDMETHODIMP GetFontDUI(UINT type, IGdiFont** pp);
	STDMETHODIMP GetProperty(BSTR name, VARIANT defaultval, VARIANT* p);
	STDMETHODIMP NotifyOthers(BSTR name, VARIANT info);
	STDMETHODIMP Reload();
	STDMETHODIMP Repaint(VARIANT_BOOL force);
	STDMETHODIMP RepaintRect(LONG x, LONG y, LONG w, LONG h, VARIANT_BOOL force);
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
	STDMETHODIMP get_Name(BSTR* p);
	STDMETHODIMP get_Width(INT* p);
	STDMETHODIMP put_DlgCode(UINT code);
	STDMETHODIMP put_MaxHeight(UINT height);
	STDMETHODIMP put_MaxWidth(UINT width);
	STDMETHODIMP put_MinHeight(UINT height);
	STDMETHODIMP put_MinWidth(UINT width);

private:
	HostComm * m_host;
};
