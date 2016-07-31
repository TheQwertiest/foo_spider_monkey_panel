#pragma once

#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define WINVER _WIN32_WINNT_VISTA
#define _WIN32_IE _WIN32_IE_IE70

// Undefine to enable VLD
#define NO_VISUAL_LEAK_DETECTOR

#if defined(_DEBUG) && !defined(NO_VISUAL_LEAK_DETECTOR)
#include <vld.h>
#endif

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
#define _WTL_USE_CSTRING
#define _WTL_NO_WTYPES
#include <atlbase.h>
#include <atlapp.h>

#include <atlwin.h>
#include <atlframe.h>
#include <atldlgs.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atltheme.h>
#include <atlmisc.h>

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
