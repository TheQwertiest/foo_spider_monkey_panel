#include <stdafx.h>

#include "heartbeat_window.h"

#include <js_engine/js_engine.h>
#include <panel/user_message.h>

#include <qwr/winapi_error_helpers.h>

namespace smp
{

using namespace mozjs;

HeartbeatWindow ::HeartbeatWindow( HWND hWnd )
    : hWnd_( hWnd ){};

HeartbeatWindow ::~HeartbeatWindow()
{
    DestroyWindow( hWnd_ );
};

std::unique_ptr<HeartbeatWindow> HeartbeatWindow::Create()
{
    static const wchar_t* class_name = L"DUMMY_CLASS";
    WNDCLASSEX wx = { 0 };
    wx.cbSize = sizeof( WNDCLASSEX );
    wx.lpfnWndProc = WndProc;
    wx.lpszClassName = class_name;

    ATOM atom = RegisterClassEx( &wx );
    qwr::error::CheckWinApi( !!atom, "RegisterClassEx" );

    HWND hWnd = CreateWindowEx( 0, MAKEINTATOM( atom ), nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr );
    qwr::error::CheckWinApi( hWnd, "CreateWindowEx" );

    return std::unique_ptr<HeartbeatWindow>( new HeartbeatWindow( hWnd ) );
}

HWND HeartbeatWindow::GetHwnd() const
{
    return hWnd_;
}

LRESULT CALLBACK HeartbeatWindow::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
    case WM_CREATE:
    case WM_DESTROY:
    {
        return 0;
    }
    case static_cast<UINT>( MiscMessage::heartbeat ):
    {
        mozjs::JsEngine::GetInstance().OnHeartbeat();
        return 0;
    }
    default:
    {
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    }
}

} // namespace smp
