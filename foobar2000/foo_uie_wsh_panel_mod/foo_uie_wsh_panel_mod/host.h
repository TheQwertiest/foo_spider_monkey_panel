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


class HostComm : public wsh_panel_vars
{
public:
	enum 
	{
		KInstanceTypeCUI = 0,
		KInstanceTypeDUI,
	};

protected:
	HWND                        m_hwnd;
	INT                         m_width;
	INT                         m_height;
	POINT                       m_max_size;
	POINT                       m_min_size;
	UINT                        m_dlg_code;
	HDC                         m_hdc;
	HBITMAP                     m_gr_bmp;
	HBITMAP                     m_gr_bmp_bk;
	UINT                        m_accuracy;
	metadb_handle_ptr           m_watched_handle;
	t_script_info               m_script_info;
	ui_selection_holder::ptr    m_selection_holder;
	int                         m_instance_type;
	bool                        m_suppress_drawing;
	bool                        m_paint_pending;
	HostTimerDispatcher         m_host_timer_dispatcher;
	panel_tooltip_param_ptr     m_panel_tooltip_param_ptr;
	
	HostComm();
	virtual ~HostComm();

public:
	GUID GetGUID() { return get_config_guid(); }
	inline HDC GetHDC() { return m_hdc; }
	inline HWND GetHWND() { return m_hwnd; }
	inline INT GetWidth() { return m_width; }
	inline INT GetHeight() { return m_height; }
	inline UINT GetInstanceType() { return m_instance_type; }
	inline POINT & MaxSize() { return m_max_size; }
	inline POINT & MinSize() { return m_min_size; }
	inline UINT & DlgCode() { return m_dlg_code; }
	inline metadb_handle_ptr & GetWatchedMetadbHandle() { return m_watched_handle; }
	IGdiBitmap * GetBackgroundImage();
	inline void PreserveSelection() { m_selection_holder = static_api_ptr_t<ui_selection_manager>()->acquire(); }
	inline t_script_info & ScriptInfo() { return m_script_info; }
	inline panel_tooltip_param_ptr & PanelTooltipParam() { return m_panel_tooltip_param_ptr; }

	void Redraw();
	void Repaint(bool force = false);
	void RepaintRect(UINT x, UINT y, UINT w, UINT h, bool force = false);
	void RefreshBackground(LPRECT lprcUpdate = NULL);
	ITimerObj * CreateTimerTimeout(UINT timeout);
	ITimerObj * CreateTimerInterval(UINT delay);
	void KillTimer(ITimerObj * p);
	unsigned SetTimeout(IDispatch * func, INT delay);
	unsigned SetInterval(IDispatch * func, INT delay);
	void ClearIntervalOrTimeout(UINT timerId);

	virtual DWORD GetColorCUI(unsigned type, const GUID & guid) = 0;
	virtual HFONT GetFontCUI(unsigned type, const GUID & guid) = 0;
	virtual DWORD GetColorDUI(unsigned type) = 0;
	virtual HFONT GetFontDUI(unsigned type) = 0;
	
	static void CALLBACK g_timer_proc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
};

class FbWindow : public IDispatchImpl3<IFbWindow>
{
private:
	HostComm * m_host;

protected:
	FbWindow(HostComm* p) : m_host(p) {}
	virtual ~FbWindow() {}

public:
	STDMETHODIMP get_ID(UINT* p);
	STDMETHODIMP get_Width(INT* p);
	STDMETHODIMP get_Height(INT* p);
	STDMETHODIMP get_InstanceType(UINT* p);
	STDMETHODIMP get_MaxWidth(UINT* p);
	STDMETHODIMP put_MaxWidth(UINT width);
	STDMETHODIMP get_MaxHeight(UINT* p);
	STDMETHODIMP put_MaxHeight(UINT height);
	STDMETHODIMP get_MinWidth(UINT* p);
	STDMETHODIMP put_MinWidth(UINT width);
	STDMETHODIMP get_MinHeight(UINT* p);
	STDMETHODIMP put_MinHeight(UINT height);
	STDMETHODIMP get_DlgCode(UINT* p);
	STDMETHODIMP put_DlgCode(UINT code);
	STDMETHODIMP get_IsTransparent(VARIANT_BOOL* p);
	STDMETHODIMP get_IsVisible(VARIANT_BOOL* p);
	STDMETHODIMP Repaint(VARIANT_BOOL force);
	STDMETHODIMP RepaintRect(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL force);
	STDMETHODIMP CreatePopupMenu(IMenuObj ** pp);
	STDMETHODIMP CreateTimerTimeout(UINT timeout, ITimerObj ** pp);
	STDMETHODIMP CreateTimerInterval(UINT delay, ITimerObj ** pp);
	STDMETHODIMP KillTimer(ITimerObj * p);
	STDMETHODIMP SetInterval(IDispatch * func, INT delay, UINT * outIntervalID);
	STDMETHODIMP ClearInterval(UINT intervalID);
	STDMETHODIMP SetTimeout(IDispatch * func, INT delay, UINT * outTimeoutID);
	STDMETHODIMP ClearTimeout(UINT timeoutID);
	STDMETHODIMP NotifyOthers(BSTR name, VARIANT info);
	STDMETHODIMP WatchMetadb(IFbMetadbHandle * handle);
	STDMETHODIMP UnWatchMetadb();
	STDMETHODIMP CreateTooltip(IFbTooltip ** pp);
	STDMETHODIMP ShowConfigure();
	STDMETHODIMP ShowProperties();
	STDMETHODIMP GetProperty(BSTR name, VARIANT defaultval, VARIANT * p);
	STDMETHODIMP SetProperty(BSTR name, VARIANT val);
	STDMETHODIMP GetBackgroundImage(IGdiBitmap ** pp);
	STDMETHODIMP SetCursor(UINT id);
	STDMETHODIMP GetColorCUI(UINT type, BSTR guidstr, int * p);
	STDMETHODIMP GetFontCUI(UINT type, BSTR guidstr, IGdiFont ** pp);
	STDMETHODIMP GetColorDUI(UINT type, int * p);
	STDMETHODIMP GetFontDUI(UINT type, IGdiFont ** pp);
	STDMETHODIMP CreateThemeManager(BSTR classid, IThemeManager ** pp);
};

class ScriptHost : 
	public IActiveScriptSite,
	public IActiveScriptSiteWindow
{
private:
	volatile DWORD          m_dwRef;
	HostComm*               m_host;
	IFbWindowPtr            m_window;
	IGdiUtilsPtr            m_gdi;
	IFbUtilsPtr             m_fb2k;
	IWSHUtilsPtr            m_utils;
	IFbPlaylistManagerPtr   m_playlistman;
	DWORD                   m_dwStartTime;

	// Scripting
	IActiveScriptPtr        m_script_engine;
	IDispatchPtr            m_script_root;

	bool  m_engine_inited;
	bool  m_has_error;

	typedef pfc::map_t<DWORD, pfc::string8> contextToPathMap;
	contextToPathMap m_contextToPathMap;
	DWORD m_lastSourceContext;

	ScriptCallbackInvoker   m_callback_invoker;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSiteWindow)
	END_COM_QI_IMPL()

public:
	ScriptHost(HostComm * host);
	virtual ~ScriptHost();

public:
	// IUnknown
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IActiveScriptSite
	STDMETHODIMP GetLCID(LCID* plcid);
	STDMETHODIMP GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** ppunk, ITypeInfo** ppti);
	STDMETHODIMP GetDocVersionString(BSTR* pstr);
	STDMETHODIMP OnScriptTerminate(const VARIANT* result, const EXCEPINFO* excep);
	STDMETHODIMP OnStateChange(SCRIPTSTATE state);
	STDMETHODIMP OnScriptError(IActiveScriptError* err);
	STDMETHODIMP OnEnterScript();
	STDMETHODIMP OnLeaveScript();

	// IActiveScriptSiteWindow
	STDMETHODIMP GetWindow(HWND *phwnd);
	STDMETHODIMP EnableModeless(BOOL fEnable);

public:
	HRESULT Initialize();
	HRESULT ProcessImportedScripts(script_preprocessor &preprocessor, IActiveScriptParsePtr& parser);
	HRESULT InitScriptEngineByName(const wchar_t * engineName);
	static void EnableSafeModeToScriptEngine(IActiveScript * engine, bool enable);
	void Finalize();

	inline void Stop() { m_engine_inited = false; if (m_script_engine) m_script_engine->SetScriptState(SCRIPTSTATE_DISCONNECTED); }
	inline bool Ready() { return m_engine_inited && m_script_engine; }
	inline bool HasError() { return m_has_error; }
	HRESULT InvokeCallback(int callbackId, VARIANTARG * argv = NULL, UINT argc = 0, VARIANT * ret = NULL);
	HRESULT GenerateSourceContext(const wchar_t * path, const wchar_t * code, DWORD & source_context);
	void ReportError(IActiveScriptError* err);
};
