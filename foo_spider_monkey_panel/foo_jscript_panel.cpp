#include "stdafx.h"
#include "thread_pool.h"
#include "popup_msg.h"
#include "panel_manager.h"
#include "user_message.h"

// Script TypeLib
ITypeLibPtr g_typelib;

namespace
{
	DECLARE_COMPONENT_VERSION(
		JSP_NAME,
		JSP_VERSION,
		JSP_NAME " v" JSP_VERSION " by marc2003\n"
		"Based on WSH Panel Mod by T.P. Wang\n\n"
		"Build: " __TIME__ ", " __DATE__ "\n"
		"Columns UI SDK Version: " UI_EXTENSION_VERSION
	);

	VALIDATE_COMPONENT_FILENAME(JSP_DLL_NAME);

	// Is there anything not correctly loaded?
	enum t_load_status_error
	{
		E_OK = 0,
		E_TYPELIB = 1 << 0,
		E_SCINTILLA = 1 << 1,
		E_GDIPLUS = 1 << 2,
		E_OLE = 1 << 3,
	};

	int g_load_status = E_OK;

	class js_initquit : public initquit
	{
	public:
		void on_init()
		{
			// HACK: popup_message services will not be initialized soon after start.
			check_error();
			delay_loader::g_set_ready();
		}

		void on_quit()
		{
			panel_manager::instance().send_msg_to_all(UWM_SCRIPT_TERM, 0, 0);
			simple_thread_pool::instance().join();
		}

	private:
		void check_error()
		{
			// Check and show error message
			pfc::string8 err_msg;

			if (g_load_status != E_OK)
			{
				err_msg += "This error message indicates that this component will not function properly:\n\n";

				if (g_load_status & E_OLE)
				{
					err_msg += "OLE: Initialize OLE Failed.\n\n";
				}
				if (g_load_status & E_TYPELIB)
				{
					err_msg += "Type Library: Load TypeLib Failed.\n\n";
				}
				if (g_load_status & E_SCINTILLA)
				{
					err_msg += "Scintilla: Load Scintilla Failed.\n\n";
				}
				if (g_load_status & E_GDIPLUS)
				{
					err_msg += "Gdiplus: Load Gdiplus Failed.\n\n";
				}
			}

			if (!err_msg.is_empty())
			{
				popup_msg::g_show(err_msg, JSP_NAME);
			}
		}
	};

	initquit_factory_t<js_initquit> g_initquit;
	CAppModule _Module;

	extern "C" BOOL WINAPI DllMain(HINSTANCE ins, DWORD reason, LPVOID lp)
	{
		static ULONG_PTR g_gdip_token;

		switch (reason)
		{
		case DLL_PROCESS_ATTACH:
		{
			// Load TypeLib
			TCHAR path[MAX_PATH + 4];
			DWORD len = GetModuleFileName(ins, path, MAX_PATH);

			path[len] = 0;

			if (FAILED(OleInitialize(NULL)))
			{
				g_load_status |= E_OLE;
			}
			if (FAILED(LoadTypeLibEx(path, REGKIND_NONE, &g_typelib)))
			{
				g_load_status |= E_TYPELIB;
			}
			if (!Scintilla_RegisterClasses(ins))
			{
				g_load_status |= E_SCINTILLA;
			}
			Gdiplus::GdiplusStartupInput gdip_input;
			if (Gdiplus::GdiplusStartup(&g_gdip_token, &gdip_input, NULL) != Gdiplus::Ok)
			{
				g_load_status |= E_GDIPLUS;
			}
			_Module.Init(NULL, ins);
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			// Term WTL
			_Module.Term();

			// Shutdown GDI+
			Gdiplus::GdiplusShutdown(g_gdip_token);

			// Free Scintilla resource
			Scintilla_ReleaseResources();

			OleUninitialize();
		}
		break;
		}

		return TRUE;
	}
}
