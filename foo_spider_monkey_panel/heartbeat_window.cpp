#include <stdafx.h>
#include "heartbeat_window.h"

#include <js_engine/js_engine.h>

#include <user_message.h>

namespace smp
{

HeartbeatWindow ::HeartbeatWindow( HWND hWnd )
    : hWnd_( hWnd )
{
};

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
    if ( !RegisterClassEx( &wx ) )
    {
        return nullptr;
    }

    HWND hWnd = CreateWindowEx( 0, class_name, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0 );
    if ( !hWnd )
    {
        return nullptr;
    }

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
        case UWM_HEARTBEAT:
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
