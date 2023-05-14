#include <stdafx.h>

#include "mouse_message_handler.h"

#include <events/dispatcher/event_dispatcher.h>
#include <events/mouse_event.h>
#include <events/wheel_event.h>
#include <os/system_settings.h>
#include <panel/panel_window.h>
#include <panel/panel_window_graphics.h>

namespace
{

// TODO: test rbutton lbutton swap
// TODO: implement 4th and 5th message handling: https://searchfox.org/mozilla-central/source/widget/windows/nsWindow.cpp#6058
const std::unordered_map<int, smp::MouseEvent::KeyFlag> kMsgToMouseKey{
    { WM_LBUTTONDOWN, smp::MouseEvent::KeyFlag::kPrimary },
    { WM_LBUTTONUP, smp::MouseEvent::KeyFlag::kPrimary },
    { WM_LBUTTONDBLCLK, smp::MouseEvent::KeyFlag::kPrimary },
    { WM_MBUTTONDOWN, smp::MouseEvent::KeyFlag::kAuxiliary },
    { WM_MBUTTONUP, smp::MouseEvent::KeyFlag::kAuxiliary },
    { WM_MBUTTONDBLCLK, smp::MouseEvent::KeyFlag::kAuxiliary },
    { WM_RBUTTONDOWN, smp::MouseEvent::KeyFlag::kSecondary },
    { WM_RBUTTONUP, smp::MouseEvent::KeyFlag::kSecondary },
    { WM_RBUTTONDBLCLK, smp::MouseEvent::KeyFlag::kSecondary },
    { WM_CONTEXTMENU, smp::MouseEvent::KeyFlag::kSecondary },
    { WM_MOUSEMOVE, smp::MouseEvent::KeyFlag::kNoButtons },
    { WM_MOUSELEAVE, smp::MouseEvent::KeyFlag::kNoButtons },
    { WM_MOUSEWHEEL, smp::MouseEvent::KeyFlag::kNoButtons },
    { WM_MOUSEHWHEEL, smp::MouseEvent::KeyFlag::kNoButtons }
};

} // namespace

namespace
{

auto GenerateMouseEvent( smp::EventId eventId, const MSG& msg, LONG x, LONG y )
{
    return std::make_unique<smp::MouseEvent>( eventId,
                                              kMsgToMouseKey.at( msg.message ),
                                              static_cast<int32_t>( x ),
                                              static_cast<int32_t>( y ) );
}

auto GenerateMouseEvent( smp::EventId eventId, const MSG& msg )
{
    return GenerateMouseEvent( eventId, msg, GET_X_LPARAM( msg.lParam ), GET_Y_LPARAM( msg.lParam ) );
}

auto GenerateWheelEvent( smp::EventId eventId, const MSG& msg, LONG x, LONG y )
{
    const auto& systemSettings = smp::os::SystemSettings::Instance();

    const auto isVertical = ( msg.message == WM_MOUSEWHEEL );
    const auto dir = ( isVertical
                           ? smp::WheelEvent::WheelDirection::kVertical
                           : smp::WheelEvent::WheelDirection::kHorizontal );
    const auto delta = GET_WHEEL_DELTA_WPARAM( msg.wParam );
    const auto mode = ( systemSettings.IsPageScroll( isVertical )
                            ? smp::WheelEvent::WheelMode::kPage
                            : smp::WheelEvent::WheelMode::kLine );

    return std::make_unique<smp::WheelEvent>( smp::EventId::kNew_MouseWheel,
                                              kMsgToMouseKey.at( msg.message ),
                                              x,
                                              y,
                                              delta,
                                              dir,
                                              mode );
}

} // namespace

namespace smp::panel
{

MouseMessageHandler::MouseMessageHandler( PanelWindow& parent )
    : parent_( parent )
{
}

std::optional<LRESULT> MouseMessageHandler::HandleMessage( const MSG& msg )
{
    CWindow wnd = parent_.GetHWND();

    switch ( msg.message )
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        if ( parent_.GetScriptSettings().ShouldGrabFocus() )
        {
            wnd.SetFocus();
        }

        SetCaptureMouseState( true );

        lastMouseDownCount_ = 1;

        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateMouseEvent( EventId::kNew_MouseButtonDown, msg ),
                                         EventPriority::kInput );
        return std::nullopt;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    {
        if ( isMouseCaptured_ )
        {
            SetCaptureMouseState( false );
        }

        const auto mouseKey = kMsgToMouseKey.at( msg.message );
        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateMouseEvent( EventId::kNew_MouseButtonUp, msg ),
                                         EventPriority::kInput );
        if ( lastMouseDownCount_ >= 1 )
        {
            if ( mouseKey == MouseEvent::KeyFlag::kPrimary )
            {
                // https://w3c.github.io/uievents/#event-type-click
                EventDispatcher::Get().PutEvent( wnd,
                                                 GenerateMouseEvent( EventId::kNew_MouseButtonClick, msg ),
                                                 EventPriority::kInput );
            }
            else
            {
                // https://w3c.github.io/uievents/#event-type-auxclick
                EventDispatcher::Get().PutEvent( wnd,
                                                 GenerateMouseEvent( EventId::kNew_MouseButtonAuxClick, msg ),
                                                 EventPriority::kInput );
            }
        }
        if ( lastMouseDownCount_ >= 2 && mouseKey == MouseEvent::KeyFlag::kPrimary )
        { // https://w3c.github.io/uievents/#event-type-dblclick
            EventDispatcher::Get().PutEvent( wnd,
                                             GenerateMouseEvent( EventId::kNew_MouseButtonDoubleClick, msg ),
                                             EventPriority::kInput );
        }

        if ( msg.message == WM_RBUTTONUP )
        {
            return 0;
        }
        else
        {
            return std::nullopt;
        }
    }
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    {

        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateMouseEvent( EventId::kNew_MouseButtonDown, msg ),
                                         EventPriority::kInput );

        // TODO: check this when testing primary button swap
        if ( msg.message == WM_LBUTTONDBLCLK )
        { // https://w3c.github.io/uievents/#event-type-dblclick
            lastMouseDownCount_ = 2;
            EventDispatcher::Get().PutEvent( wnd,
                                             GenerateMouseEvent( EventId::kNew_MouseButtonDoubleClickNative, msg ),
                                             EventPriority::kInput );
        }

        return std::nullopt;
    }
    case WM_CONTEXTMENU:
    {
        const auto mousePos = [&]() -> POINT {
            if ( msg.lParam == -1 )
            { // happens if invoked via keyboard
                const auto pGraphics = parent_.GetGraphics();
                if ( !pGraphics )
                {
                    return {};
                }
                return { (LONG)pGraphics->GetWidth() * 2 / 3, (LONG)pGraphics->GetHeight() * 2 / 3 };
            }
            else
            {
                // WM_CONTEXTMENU receives screen coordinates
                POINT mousePos{ GET_X_LPARAM( msg.lParam ), GET_Y_LPARAM( msg.lParam ) };
                wnd.ScreenToClient( &mousePos );
                return mousePos;
            }
        }();

        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateMouseEvent( EventId::kNew_MouseContextMenu, msg, mousePos.x, mousePos.y ),
                                         EventPriority::kInput );

        return 1;
    }
    case WM_MOUSEMOVE:
    {
        const auto mouseKey = kMsgToMouseKey.at( msg.message );

        if ( !isMouseTracked_ )
        {
            isMouseTracked_ = true;

            TRACKMOUSEEVENT tme{ sizeof( TRACKMOUSEEVENT ), TME_LEAVE, wnd, HOVER_DEFAULT };
            TrackMouseEvent( &tme );

            // Restore default cursor
            SetCursor( LoadCursor( nullptr, IDC_ARROW ) );

            EventDispatcher::Get().PutEvent( wnd,
                                             GenerateMouseEvent( EventId::kNew_MouseEnter, msg ),
                                             EventPriority::kInput );
        }

        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateMouseEvent( EventId::kNew_MouseMove, msg ),
                                         EventPriority::kInput );

        return std::nullopt;
    }
    case WM_MOUSELEAVE:
    {
        isMouseTracked_ = false;

        // Restore default cursor
        SetCursor( LoadCursor( nullptr, IDC_ARROW ) );

        POINT mousePos{};
        ::GetCursorPos( &mousePos );
        wnd.ScreenToClient( &mousePos );

        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateMouseEvent( EventId::kNew_MouseLeave, msg, mousePos.x, mousePos.y ),
                                         EventPriority::kInput );

        return std::nullopt;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
        // WM_MOUSE*WHEEL receives screen coordinates
        POINT mousePos{ GET_X_LPARAM( msg.lParam ), GET_Y_LPARAM( msg.lParam ) };
        wnd.ScreenToClient( &mousePos );

        EventDispatcher::Get().PutEvent( wnd,
                                         GenerateWheelEvent( EventId::kNew_MouseWheel, msg, mousePos.x, mousePos.y ),
                                         EventPriority::kInput );
        return 0;
    }
    case WM_VSCROLL:
    case WM_HSCROLL:
        // TODO: uimpl
        return std::nullopt;
    case WM_SETCURSOR:
    {
        return 1;
    }
    default:
    {
        assert( false );
        return std::nullopt;
    }
    }
}

void MouseMessageHandler::OnFocusMessage()
{
    lastMouseDownCount_ = 0;
}

void MouseMessageHandler::SetCaptureMouseState( bool shouldCapture )
{
    if ( shouldCapture )
    {
        ::SetCapture( parent_.GetHWND() );
    }
    else
    {
        ::ReleaseCapture();
    }
    isMouseCaptured_ = shouldCapture;
}

} // namespace smp::panel
