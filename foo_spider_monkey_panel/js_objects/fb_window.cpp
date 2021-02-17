#include <stdafx.h>

#include "fb_window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <qwr/winapi_error_helpers.h>

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFbWindow::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbWindow",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Aero, JsFbWindow::get_Aero )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_BlockMaximize, JsFbWindow::get_BlockMaximize )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_FoobarCpuUsage, JsFbWindow::get_FoobarCpuUsage )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_FrameStyle, JsFbWindow::get_FrameStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_FullScreen, JsFbWindow::get_FullScreen )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MainWindowState, JsFbWindow::get_MainWindowState )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Sizing, JsFbWindow::get_Sizing )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_SystemCpuUsage, JsFbWindow::get_SystemCpuUsage )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_BlockMaximize, JsFbWindow::put_BlockMaximize )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_FrameStyle, JsFbWindow::put_FrameStyle )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_FullScreen, JsFbWindow::put_FullScreen )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MainWindowState, JsFbWindow::put_MainWindowState )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Sizing, JsFbWindow::put_Sizing )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Aero", get_Aero, kDefaultPropsFlags ),
        JS_PSGS( "BlockMaximize", get_BlockMaximize, put_BlockMaximize, kDefaultPropsFlags ),
        JS_PSG( "FoobarCpuUsage", get_FoobarCpuUsage, kDefaultPropsFlags ),
        JS_PSGS( "FrameStyle", get_FrameStyle, put_FrameStyle, kDefaultPropsFlags ),
        JS_PSGS( "FullScreen", get_FullScreen, put_FullScreen, kDefaultPropsFlags ),
        JS_PSGS( "MainWindowState", get_MainWindowState, put_MainWindowState, kDefaultPropsFlags ),
        JS_PSGS( "Sizing", get_Sizing, put_Sizing, kDefaultPropsFlags ),
        JS_PSG( "SystemCpuUsage", get_SystemCpuUsage, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbWindow::JsClass = jsClass;
const JSFunctionSpec* JsFbWindow::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbWindow::JsProperties = jsProperties.data();

JsFbWindow::JsFbWindow( JSContext* cx, HWND hFbWnd )
    : pJsCtx_( cx )
    , hFbWnd_( hFbWnd )
{
}

std::unique_ptr<JsFbWindow>
JsFbWindow::CreateNative( JSContext* cx )
{
    HWND hFbWnd = FindWindow( L"{E7076D1C-A7BF-4f39-B771-BCBE88F2A2A8}", nullptr );
    qwr::error::CheckWinApi( !!hFbWnd, "FindWindow" );

    return std::unique_ptr<JsFbWindow>( new JsFbWindow( cx, hFbWnd ) );
}

size_t JsFbWindow::GetInternalSize()
{
    return 0;
}

void JsFbWindow::SetPseudoCaption( uint32_t x, uint32_t y, uint32_t w, uint32_t h )
{ // TODO: azaza
}

JSObject* JsFbWindow::get_Aero()
{ // TODO: azaza
    return nullptr;
}

bool JsFbWindow::get_BlockMaximize()
{ // TODO: azaza
    return false;
}

float JsFbWindow::get_FoobarCpuUsage()
{
    HANDLE handle = GetCurrentProcess();
    FILETIME dummyTime;
    FILETIME dummyTime2;

    if ( !cpuUsageStats.time )
    {
        cpuUsageStats.time = timeGetTime();
        GetProcessTimes( handle, &dummyTime, &dummyTime2, &cpuUsageStats.kernelTime, &cpuUsageStats.userTime );

        return 0.0f;
    }

    CpuUsageStats newStats;
    newStats.time = timeGetTime();
    if ( !GetProcessTimes( handle, &dummyTime, &dummyTime2, &newStats.kernelTime, &newStats.userTime ) )
    {
        return 0.0f;
    }

    uint64_t procTime = uint64_t( newStats.kernelTime.dwLowDateTime - cpuUsageStats.kernelTime.dwLowDateTime )
                        + uint64_t( newStats.userTime.dwLowDateTime - cpuUsageStats.userTime.dwLowDateTime );
    uint32_t timeDiff = newStats.time - cpuUsageStats.time;

    if ( timeDiff )
    {
        cpuUsageStats = newStats;
    }

    return ( timeDiff ? procTime * 100.0f / timeDiff : 0.0f );
}

uint8_t JsFbWindow::get_FrameStyle()
{ // TODO: azaza
    return 0;
}

bool JsFbWindow::get_FullScreen()
{ // TODO: azaza
    return false;
}

uint8_t JsFbWindow::get_MainWindowState()
{ // TODO: azaza
    return 0;
}

bool JsFbWindow::get_Sizing()
{ // TODO: azaza
    return false;
}

float JsFbWindow::get_SystemCpuUsage()
{ /*
    FILETIME idleTime;
    FILETIME userTime;
    FILETIME kernelTime;
    DWORD time = timeGetTime();
    if ( !GetSystemTimes( &idleTime, &kernelTime, &userTime ) )
    {
        return 0.0f;
    }

    FILETIME newIdleTime;
    FILETIME newUserTime;
    FILETIME newKernelTime;
    DWORD newTime = timeGetTime();
    if ( !GetSystemTimes( &newIdleTime, &newKernelTime, &newUserTime ) )
    {
        return 0.0f;
    }

    uint64_t totalTime = uint64_t( newKernelTime.dwLowDateTime - kernelTime.dwLowDateTime )
        + uint64_t( newUserTime.dwLowDateTime - userTime.dwLowDateTime );
    uint64_t usageTime = totalTime
        - uint64_t( newIdleTime.dwLowDateTime - idleTime.dwLowDateTime );
        
    return totalTime ? ((usageTime * 100) / totalTime) : 0.0f;*/
    return 0.0f;
}

void JsFbWindow::put_BlockMaximize( bool block )
{ // TODO: azaza
}

void JsFbWindow::put_FrameStyle( uint8_t style )
{
    LONG lStyle = GetWindowLong( hFbWnd_, GWL_STYLE );
    switch ( style )
    {
    case 0:
    {
        lStyle &= ( WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU );
        break;
    }
    case 1:
    {
        lStyle &= ~( WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU );
        lStyle &= ( WS_CAPTION | WS_THICKFRAME );
        break;
    }
    case 2:
    {
        lStyle &= ~( WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU );
        lStyle &= ( WS_THICKFRAME );
        break;
    }
    case 3:
    {
        lStyle &= ~( WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU );
        break;
    }
    default:
    {
        throw qwr::QwrException( "Unknown style: %u", style );
    }
    }

    SetWindowLong( hFbWnd_, GWL_STYLE, lStyle );
    SetWindowPos( hFbWnd_, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
}

void JsFbWindow::put_FullScreen( bool is )
{ // TODO: azaza
}

void JsFbWindow::put_MainWindowState( uint8_t state )
{
}

void JsFbWindow::put_Sizing( bool enable )
{ // TODO: azaza
}

} // namespace mozjs
