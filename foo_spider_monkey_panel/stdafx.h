#pragma once

// clang-format off
// !!! Include order is important here (esp. for Win headers) !!!

// Spider Monkey ESR60 and CUI support only Win7+
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

// Fix std min max conflicts
#define NOMINMAX
#include <algorithm>
namespace Gdiplus
{
using std::min;
using std::max;
};

#include <WinSock2.h>
#include <Windows.h>
#pragma warning( push, 0 )
#   include <GdiPlus.h>
#pragma warning( pop ) 

// COM objects
#include <ActivScp.h>
#include <activdbg.h>
#include <MsHTML.h>
#include <MsHtmHst.h>
#include <ShlDisp.h>
#include <exdisp.h>
#include <shobjidl_core.h>
// Generates wrappers for COM listed above
#include <ComDef.h>

// ATL/WTL
/// atlstr.h (includes atlbase.h) must be included first for CString to LPTSTR conversion to work.
/// windowsx.h must be included first to avoid conflicts.
#include <windowsx.h>
#include <atlstr.h> 
#include <atlapp.h>
#include <atlcom.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atldlgs.h>
#include <atlfind.h>
#include <atlframe.h>
#include <atltheme.h>
#include <atltypes.h>
#include <atlwin.h>

/// Restore some windowsx.h macros
#ifndef SelectFont
#    define SelectFont(hdc, hfont) ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT)(hfont)))
#endif
#ifndef SelectBitmap
#    define SelectBitmap(hdc, hbm) ((HBITMAP)SelectObject((hdc), (HGDIOBJ)(HBITMAP)(hbm)))
#endif

// foobar2000 SDK
#pragma warning( push, 0 )
#   include <foobar2000/SDK/foobar2000.h>
#pragma warning( pop ) 

// Columns UI SDK
#pragma warning( push, 0 )
#   include <columns_ui-sdk/ui_extension.h>
#pragma warning( pop ) 

#if defined(__clang__)

#define SMP_DO_PRAGMA_(x) _Pragma (#x)
#define SMP_DO_PRAGMA(x) SMP_DO_PRAGMA_(x)

#define SMP_CLANG_WARNING_PUSH \
    _Pragma( "clang diagnostic push" )

#define SMP_CLANG_SUPPRESS_WARNING(w) \
    SMP_DO_PRAGMA(clang diagnostic ignored w)

#define SMP_CLANG_WARNING_POP \
    _Pragma( "clang diagnostic pop" )

#else

#define SMP_CLANG_WARNING_PUSH
#define SMP_CLANG_SUPPRESS_WARNING(w)
#define SMP_CLANG_WARNING_POP

#endif

// 4251: dll interface warning
#define SMP_MJS_SUPPRESS_WARNINGS_PUSH \
    __pragma( warning( push ) )        \
    __pragma( warning( disable : 4251 ) ) 

#define SMP_MJS_SUPPRESS_WARNINGS_POP \
    __pragma( warning( pop ) )


// Mozilla SpiderMonkey
SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <jsapi.h>
#include <jsfriendapi.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

// fmt
#define FMT_HEADER_ONLY
#include <fmt/format.h>
/// wchar_t support
#include <fmt/xchar.h>

// range v3
#include <range/v3/all.hpp>

// json
/// Enable extended diagnostics
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>

#include <string>
namespace qwr
{// TODO: create a custom type
    using u8string = std::string;
    using u8string_view = std::string_view;
}

// Additional PFC wrappers
#include <qwr/pfc_helpers_cnt.h>
#include <qwr/pfc_helpers_stream.h>

#include <utils/fmt_pfc_adaptor.h>

#include <qwr/unicode.h>
#include <qwr/qwr_exception.h>

#include <utils/js_exception.h>

#include <component_defines.h>
#include <component_guids.h>

// clang-format on
