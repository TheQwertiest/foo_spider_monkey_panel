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

// Scintilla
#include <Scintilla.h>
#include <SciLexer.h>

// foobar2000 SDK
#pragma warning( push, 0 )
#   include <foobar2000/SDK/foobar2000.h>
#pragma warning( pop ) 

// Columns UI SDK
#pragma warning( push, 0 )
#   include <columns_ui-sdk/ui_extension.h>
#pragma warning( pop ) 

// Mozilla SpiderMonkey
#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning (STL4015)
#   include <jsapi.h>
#   include <jsfriendapi.h>
#pragma warning( pop ) 

// fmt
#define FMT_HEADER_ONLY
#include <fmt/format.h>

// range v3
#include <range/v3/all.hpp>

// span
// nonstd::span uses (signed) std::ptrdiff_t by default, which is incorrect
#define span_CONFIG_INDEX_TYPE size_t

// Some macros defined by windowsx.h should be removed
#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif

#if not __cpp_char8_t
// Dummy type
#include <string>

using char8_t = char;
namespace std // NOLINT(cert-dcl58-cpp)
{
using u8string = basic_string<char8_t, char_traits<char8_t>, allocator<char8_t>>;
using u8string_view = basic_string_view<char8_t>;
}
#endif

// Additional PFC wrappers
#include <utils/pfc_helpers_cnt.h>
#include <utils/pfc_helpers_stream.h>
#include <utils/pfc_helpers_ui.h>

// Unicode converters
#include <utils/unicode.h>

#include <component_defines.h>
#include <component_guids.h>
#include <smp_exception.h>

// clang-format on
