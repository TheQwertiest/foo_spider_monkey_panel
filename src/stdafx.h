#pragma once

#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define WINVER _WIN32_WINNT_WINXP
#define _WIN32_IE _WIN32_IE_IE60

#define TO_VARIANT_BOOL(v) ((v) ? (VARIANT_TRUE) : (VARIANT_FALSE))

#include <tchar.h>
#include <Windows.h>
#include <GdiPlus.h>
#include <ActivScp.h>
#include <activdbg.h>
#include <ShellApi.h>
#include <CommCtrl.h>
#include <ComDef.h>
#include <ComDefSp.h>
#include <ObjSafe.h>
#include <StrSafe.h>
#include <uxtheme.h>

// ATL/WTL
#include <atlstr.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atldlgs.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atltheme.h>

// Other controls
#define STATIC_BUILD 1
#define SCI_LEXER 1
#include <Scintilla.h>
#include <SciLexer.h>

// foobar2000 SDK
#include "../foobar2000/SDK/foobar2000.h"
// Columns UI SDK
#include "../columns_ui-sdk/ui_extension.h"

// Some macros defined by windowsx.h should be removed
#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif

#include "component_defines.h"
