#include <stdafx.h>

#include "window_new.h"

#include <events/mouse_event.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/dom/mouse_event.h>
#include <js_backend/objects/dom/paint_event.h>
#include <panel/panel_window.h>
#include <panel/panel_window_graphics.h>

using namespace smp;

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
    JsObjectBase<WindowNew>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    WindowNew::Trace
};

JSClass jsClass = {
    "WindowNew",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Repaint, WindowNew::Repaint, WindowNew::RepaintWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( RepaintRect, WindowNew::RepaintRect, WindowNew::RepaintRectWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "repaint", Repaint, 0, kDefaultPropsFlags ),
        JS_FN( "repaintRect", RepaintRect, 4, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, WindowNew::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, WindowNew::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "height", get_Height, kDefaultPropsFlags ),
        JS_PSG( "width", get_Width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass WindowNew::JsClass = jsClass;
const JSFunctionSpec* WindowNew::JsFunctions = jsFunctions.data();
const JSPropertySpec* WindowNew::JsProperties = jsProperties.data();
const JsPrototypeId WindowNew::BasePrototypeId = JsPrototypeId::EventTarget;
const JsPrototypeId WindowNew::ParentPrototypeId = JsPrototypeId::EventTarget;

const std::unordered_set<EventId> WindowNew::kHandledEvents{
    EventId::kNew_WndPaint,
    EventId::kNew_WndResize,
    EventId::kNew_InputBlur,
    EventId::kNew_InputFocus,
    EventId::kNew_MouseLeftButtonDown,
    EventId::kNew_MouseLeftButtonUp,
    EventId::kNew_MouseLeftButtonDoubleClick,
    EventId::kNew_MouseRightButtonDown,
    EventId::kNew_MouseRightButtonUp,
    EventId::kNew_MouseRightButtonDoubleClick,
    EventId::kNew_MouseMiddleButtonDown,
    EventId::kNew_MouseMiddleButtonUp,
    EventId::kNew_MouseMiddleButtonDoubleClick,
    EventId::kNew_MouseMove,
    EventId::kNew_MouseContextMenu,
    EventId::kNew_MouseLeave,
};

WindowNew::WindowNew( JSContext* cx, smp::panel::PanelWindow& parentPanel )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
    , parentPanel_( parentPanel )
{
    // paranoia check: there should be no need to do this
    isFocused_ = ( ::GetFocus() == parentPanel_.GetHWND() );
}

std::unique_ptr<WindowNew>
WindowNew::CreateNative( JSContext* cx, smp::panel::PanelWindow& parentPanel )
{
    return std::unique_ptr<WindowNew>( new WindowNew( cx, parentPanel ) );
}

size_t WindowNew::GetInternalSize()
{
    return 0;
}

void WindowNew::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::MouseEvent>>( cx, self, JsPrototypeId::New_MouseEvent );
}

void WindowNew::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<WindowNew>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JsEventTarget::Trace( trc, obj );
}

void WindowNew::PrepareForGc()
{
    JsEventTarget::PrepareForGc();

    isFinalized_ = true;
}

const std::string& WindowNew::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_WndPaint, "paint" },
        { EventId::kNew_WndResize, "resize" },
        { EventId::kNew_InputBlur, "blur" },
        { EventId::kNew_InputFocus, "focus" },
        { EventId::kNew_MouseLeftButtonDown, "mousedown" },
        { EventId::kNew_MouseLeftButtonUp, "mouseup" },
        { EventId::kNew_MouseLeftButtonDoubleClick, "dblclicknative" },
        { EventId::kNew_MouseRightButtonDown, "mousedown" },
        { EventId::kNew_MouseRightButtonUp, "mouseup" },
        { EventId::kNew_MouseRightButtonDoubleClick, "dblclicknative" },
        { EventId::kNew_MouseMiddleButtonDown, "mousedown" },
        { EventId::kNew_MouseMiddleButtonUp, "mouseup" },
        { EventId::kNew_MouseMiddleButtonDoubleClick, "dblclicknative" },
        { EventId::kNew_MouseMove, "mousemove" },
        { EventId::kNew_MouseContextMenu, "contextmenu" },
        { EventId::kNew_MouseLeave, "mouseleave" }
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus WindowNew::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;
    if ( isFinalized_ )
    {
        return status;
    }

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    if ( event.GetId() == EventId::kNew_WndPaint )
    {
        HandlePaintEvent( self );
        return status;
    }

    HandleEventPre( event );

    JS::RootedObject jsEvent( pJsCtx_, GenerateEvent( event, eventType ) );
    JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
    DispatchEvent( self, jsEventValue );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

void WindowNew::Repaint( bool force )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.Repaint( force );
}

void WindowNew::RepaintWithOpt( size_t optArgCount, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return Repaint( force );
    case 1:
        return Repaint();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void WindowNew::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.RepaintRect( CRect{ static_cast<int>( x ), static_cast<int>( y ), static_cast<int>( x + w ), static_cast<int>( y + h ) }, force );
}

void WindowNew::RepaintRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return RepaintRect( x, y, w, h, force );
    case 1:
        return RepaintRect( x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t WindowNew::get_Height()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    auto pGraphics = parentPanel_.GetGraphics();
    if ( !pGraphics )
    {
        return 0;
    }

    return pGraphics->GetHeight();
}

uint32_t WindowNew::get_Width()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    auto pGraphics = parentPanel_.GetGraphics();
    if ( !pGraphics )
    {
        return 0;
    }

    return pGraphics->GetWidth();
}

void WindowNew::HandleEventPre( const smp::EventBase& event )
{
    switch ( event.GetId() )
    {
    case EventId::kNew_InputBlur:
    {
        isFocused_ = false;
        break;
    }
    case EventId::kNew_InputFocus:
    {
        isFocused_ = true;
        break;
    }
    case EventId::kNew_MouseMove:
    {
        break;
    }
    default:
        break;
    }
}

JSObject* WindowNew::GenerateEvent( const smp::EventBase& event, const qwr::u8string& eventType )
{
    JS::RootedObject jsEvent( pJsCtx_ );

    switch ( const auto event_id = event.GetId();
             event.GetId() )
    {
    case EventId::kNew_MouseLeftButtonDown:
    case EventId::kNew_MouseLeftButtonUp:
    case EventId::kNew_MouseLeftButtonDoubleClick:
    case EventId::kNew_MouseRightButtonDown:
    case EventId::kNew_MouseRightButtonUp:
    case EventId::kNew_MouseRightButtonDoubleClick:
    case EventId::kNew_MouseMiddleButtonDown:
    case EventId::kNew_MouseMiddleButtonUp:
    case EventId::kNew_MouseMiddleButtonDoubleClick:
    case EventId::kNew_MouseMove:
    case EventId::kNew_MouseContextMenu:
    case EventId::kNew_MouseLeave:
    {
        const auto& mouseEvent = static_cast<const smp::MouseEventNew&>( event );

        const auto x = static_cast<double>( mouseEvent.GetX() );
        const auto y = static_cast<double>( mouseEvent.GetY() );
        const auto button = mouseEvent.GetButton();
        const auto buttons = mouseEvent.GetButtons();
        const auto modifiers = mouseEvent.GetModifiers();

        POINT screenPos{ mouseEvent.GetX(), mouseEvent.GetY() };
        ClientToScreen( parentPanel_.GetHWND(), &screenPos );

        using enum smp::MouseEventNew::ModifierKeyFlag;
        using enum smp::MouseEventNew::MouseKeyFlag;

        mozjs::MouseEvent::EventProperties props{
            .baseProps = mozjs::JsEvent::EventProperties{ .cancelable = false },
            .altKey = !!qwr::to_underlying( modifiers & kAlt ),
            .ctrlKey = !!qwr::to_underlying( modifiers & kCtrl ),
            .metaKey = !!qwr::to_underlying( modifiers & kWin ),
            .shiftKey = !!qwr::to_underlying( modifiers & kShift ),
            .button = [&] {
            // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/button
            switch ( button )
            {
            case kNoButtons:
            case kPrimary:
                return 0;
            case kSecondary:
                return 1;
            case kAuxiliary:
                return 2;
            case k4:
                return 3;
            case k5:
                return 4;
            default:
                throw qwr::QwrException( "Internal error: unexpected button value: {}", qwr::to_underlying(button) );
            } }(),
            .buttons = [&] {
                // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/buttons
                int32_t buttons_raw = 0;

                if ( qwr::to_underlying( buttons & kPrimary ) )
                {
                    buttons_raw |= 1;
                }
                if ( qwr::to_underlying( buttons & kSecondary ) )
                {
                    buttons_raw |= 2;
                }
                if ( qwr::to_underlying( buttons & kAuxiliary ) )
                {
                    buttons_raw |= 4;
                }
                if ( qwr::to_underlying( buttons & k4 ) )
                {
                    buttons_raw |= 8;
                }
                if ( qwr::to_underlying( buttons & k5 ) )
                {
                    buttons_raw |= 16;
                } 

            return buttons_raw; }(),
            .screenX = static_cast<double>( screenPos.x ),
            .screenY = static_cast<double>( screenPos.y ),
            .x = x,
            .y = y,
        };

        jsEvent = mozjs::JsObjectBase<mozjs::MouseEvent>::CreateJs( pJsCtx_, eventType, props );
        break;
    }
    case EventId::kNew_InputBlur:
    {
        isFocused_ = false;
        [[fallthrough]];
    }
    case EventId::kNew_InputFocus:
    {
        isFocused_ = true;
        [[fallthrough]];
    }
    default:
        jsEvent = mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } );
        break;
    }

    return jsEvent;
}

void WindowNew::HandlePaintEvent( JS::HandleObject self )
{
    auto pGraphics = parentPanel_.GetGraphics();
    if ( !pGraphics )
    {
        return;
    }

    std::exception_ptr eptr;
    pGraphics->PaintWithCallback( [&]( Gdiplus::Graphics& gr ) {
        try
        {
            JS::RootedObject jsEvent( pJsCtx_,
                                      mozjs::JsObjectBase<PaintEvent>::CreateJs(
                                          pJsCtx_,
                                          gr ) );
            auto pNativeEvent = mozjs::JsObjectBase<PaintEvent>::ExtractNativeUnchecked( jsEvent );
            assert( pNativeEvent );

            JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
            DispatchEvent( self, jsEventValue );

            pNativeEvent->ResetGraphics();
        }
        catch ( ... )
        {
            eptr = std::current_exception();
        }
    } );
    if ( eptr )
    {
        std::rethrow_exception( eptr );
    }
}

} // namespace mozjs
