#include "stdafx.h"
#include "thread_pool.h"
#include "delay_loader.h"
#include "popup_msg.h"
#include "panel_manager.h"
#include "user_message.h"
#include "version.h"


// Script TypeLib
ITypeLibPtr g_typelib;

namespace
{
	DECLARE_COMPONENT_VERSION(
		WSPM_NAME,
		WSPM_VERSION,
		"Windows Scripting Host Panel Modded v" WSPM_VERSION "\n"
		"Modded by T.P. Wang\n"
		"Changes since v1.5.6 by marc2003\n\n"
		"Build: "  __TIME__ ", " __DATE__ "\n"
		"Columns UI API Version: " UI_EXTENSION_VERSION "\n\n"
		"Scintilla and SciTE\n"
		"Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>\n\n"
		"Text Designer Outline Text Library\n"
		"Copyright (c) 2009 Wong Shao Voon\n\n"
		"box blur filter\n"
		"Copyright 2006 Jerry Huxtable\n\n"
		"CPropertyList - A Property List control\n"
		"Copyright (c) 2001-2003 Bjarke Viksoe"
	);

	//VALIDATE_COMPONENT_FILENAME(WSPM_DLL_NAME);

	// Is there anything not correctly loaded?
	enum t_load_status_error
	{
		E_OK		= 0,
		E_TYPELIB	= 1 << 0,
		E_SCINTILLA = 1 << 1,
		E_GDIPLUS	= 1 << 2,
		E_OLE		= 1 << 3,
	};

	static int g_load_status = E_OK;

	class wsh_initquit : public initquit
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

			if (IS_EXPIRED(__DATE__))
			{
				err_msg = "This beta version is over two weeks old, please get a new one now.\nVisit: http://foo-wsh-panel-mod.googlecode.com\n\n";
			}
			else if (g_load_status != E_OK)
			{
				err_msg = "This error message indicates that means this component will not function properly:\n\n";

				if (g_load_status & E_OLE)
					err_msg += "OLE: Initialize OLE Failed.\n\n";

				if (g_load_status & E_TYPELIB)
					err_msg += "Type Library: Load TypeLib Failed.\n\n";

				if (g_load_status & E_SCINTILLA)
					err_msg += "Scintilla: Load Scintilla Failed.\n\n";

				if (g_load_status & E_GDIPLUS)
					err_msg += "Gdiplus: Load Gdiplus Failed.\n\n";
			}

			if (!err_msg.is_empty())
				popup_msg::g_show(err_msg, WSPM_NAME, popup_message::icon_error);
		}
	};

	static initquit_factory_t<wsh_initquit> g_initquit;
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
					g_load_status |= E_OLE;

				if (FAILED(LoadTypeLibEx(path, REGKIND_NONE, &g_typelib)))
					g_load_status |= E_TYPELIB;

				// Load Scintilla
				if (!Scintilla_RegisterClasses(ins))
					g_load_status |= E_SCINTILLA;

				// Init GDI+
				Gdiplus::GdiplusStartupInput gdip_input;
				if (Gdiplus::GdiplusStartup(&g_gdip_token, &gdip_input, NULL) != Gdiplus::Ok)
					g_load_status |= E_GDIPLUS;

				// WTL
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
