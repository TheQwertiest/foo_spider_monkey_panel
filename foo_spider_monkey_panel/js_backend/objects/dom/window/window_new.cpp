#include <stdafx.h>

#include "window_new.h"

#include <dom/dom_codes.h>
#include <dom/dom_keys.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_rendering_context_2d.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/dom/window/image.h>
#include <js_backend/objects/dom/window/keyboard_event.h>
#include <js_backend/objects/dom/window/mouse_event.h>
#include <js_backend/objects/dom/window/wheel_event.h>
#include <js_backend/utils/js_property_helper.h>
#include <js_backend/utils/panel_from_global.h>
#include <panel/panel_accessor.h>
#include <panel/panel_window.h>
#include <panel/panel_window_graphics.h>
#include <tasks/events/keyboard_event.h>
#include <tasks/events/mouse_event.h>
#include <tasks/events/wheel_event.h>
#include <ui/ui_input_box.h>
#include <ui/ui_popup.h>

#include <qwr/delayed_executor.h>

using namespace smp;

namespace
{

auto GenerateMouseEventProps( const smp::MouseEvent& mouseEvent, HWND parentHwnd )
{
    const auto x = static_cast<double>( mouseEvent.GetX() );
    const auto y = static_cast<double>( mouseEvent.GetY() );
    const auto button = mouseEvent.GetButton();
    const auto buttons = mouseEvent.GetButtons();
    const auto modifiers = mouseEvent.GetModifiers();

    POINT screenPos{ mouseEvent.GetX(), mouseEvent.GetY() };
    ClientToScreen( parentHwnd, &screenPos );

    using enum smp::MouseEvent::ModifierFlag;
    using enum smp::MouseEvent::KeyFlag;

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

    return props;
}

auto GenerateWheelEventProps( const smp::WheelEvent& wheelEvent, HWND parentHwnd )
{
    using enum smp::WheelEvent::WheelDirection;
    using enum smp::WheelEvent::WheelMode;

    const auto isVerticalDirection = ( wheelEvent.GetDirection() == kVertical );

    mozjs::WheelEvent::EventProperties props{
        .baseProps = GenerateMouseEventProps( wheelEvent, parentHwnd ),
        .deltaX = static_cast<double>( isVerticalDirection ? 0 : wheelEvent.GetDelta() ),
        .deltaY = static_cast<double>( isVerticalDirection ? wheelEvent.GetDelta() : 0 ),
        .deltaZ = 0,
        .deltaMode = [&] {
            // https://developer.mozilla.org/en-US/docs/Web/API/WheelEvent
            switch ( wheelEvent.GetMode() )
            {
            case kPixel:
                return 0;
            case kLine:
                return 1;
            case kPage:
                return 2;
            default:
                throw qwr::QwrException( "Internal error: unexpected deltaMode value: {}", qwr::to_underlying( wheelEvent.GetMode() ) );
            }
        }(),
    };

    return props;
}

auto GenerateKeyboardEventProps( const smp::KeyboardEvent& keyboardEvent )
{
    const auto key = [&] {
        const auto chars = keyboardEvent.GetChars();
        if ( !chars.empty() )
        {
            return chars;
        }
        return smp::dom::GetDomKey( keyboardEvent.GetVirtualCode() ).value_or( L"Unidentified" );
    }();
    const auto code = smp::dom::GetDomCode( keyboardEvent.GetFullScanCode(), keyboardEvent.GetVirtualCode() ).value_or( "Unidentified" );

    const auto modifiers = keyboardEvent.GetModifiers();

    using enum smp::KeyboardEvent::ModifierFlag;
    mozjs::KeyboardEvent::EventProperties props{
        .baseProps = mozjs::JsEvent::EventProperties{ .cancelable = false },
        .altKey = !!qwr::to_underlying( modifiers & kAlt ),
        .ctrlKey = !!qwr::to_underlying( modifiers & kCtrl ),
        .metaKey = !!qwr::to_underlying( modifiers & kWin ),
        .shiftKey = !!qwr::to_underlying( modifiers & kShift ),
        .code = code,
        .key = key,
        .location = [&]() -> uint32_t {
            using enum smp::KeyboardEvent::KeyLocation;
            // https://w3c.github.io/uievents/#dom-keyboardevent-location
            const auto location = keyboardEvent.GetKeyLocation();
            switch ( location )
            {
            case kStandard:
                return 0;
            case kLeft:
                return 1;
            case kRight:
                return 2;
            case kNumpad:
                return 3;
            default:
                throw qwr::QwrException( "Internal error: unexpected location value: {}", qwr::to_underlying( location ) );
            }
        }(),
        .repeat = keyboardEvent.IsRepeating()
    };

    return props;
}

} // namespace

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
    WindowNew::Trace
};

JSClass jsClass = {
    "Window",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( alert, WindowNew::Alert, WindowNew::AlertWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT_AND_SELF( getContext, WindowNew::GetContext, WindowNew::GetContextWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( loadImage, WindowNew::LoadImage )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( prompt, WindowNew::Prompt, WindowNew::PromptWithOpt, 3 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( redraw, WindowNew::Redraw, WindowNew::RedrawWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( redrawRect, WindowNew::RedrawRect, WindowNew::RedrawRectWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( showFb2kPopup, WindowNew::ShowFb2kPopup, WindowNew::ShowFb2kPopupWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "alert", alert, 0, kDefaultPropsFlags ),
        JS_FN( "getContext", getContext, 1, kDefaultPropsFlags ),
        JS_FN( "loadImage", loadImage, 1, kDefaultPropsFlags ),
        JS_FN( "prompt", prompt, 0, kDefaultPropsFlags ),
        JS_FN( "redraw", redraw, 0, kDefaultPropsFlags ),
        JS_FN( "redrawRect", redrawRect, 4, kDefaultPropsFlags ),
        JS_FN( "showFb2kPopup", showFb2kPopup, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, WindowNew::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, WindowNew::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "height", get_height, kDefaultPropsFlags ),
        JS_PSG( "width", get_width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::WindowNew );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<WindowNew>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<WindowNew>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<WindowNew>::JsProperties = jsProperties.data();
const PostJsCreateFn JsObjectTraits<WindowNew>::PostCreate = WindowNew::PostCreate;

const std::unordered_set<EventId> WindowNew::kHandledEvents{
    EventId::kNew_InputBlur,
    EventId::kNew_InputFocus,
    EventId::kNew_KeyboardKeyDown,
    EventId::kNew_KeyboardKeyUp,
    EventId::kNew_MouseButtonAuxClick,
    EventId::kNew_MouseButtonClick,
    EventId::kNew_MouseButtonDoubleClick,
    EventId::kNew_MouseButtonDoubleClickSystem,
    EventId::kNew_MouseButtonDown,
    EventId::kNew_MouseButtonUp,
    EventId::kNew_MouseContextMenu,
    EventId::kNew_MouseEnter,
    EventId::kNew_MouseLeave,
    EventId::kNew_MouseMove,
    EventId::kNew_MouseWheel,
    EventId::kNew_WndPaint,
    EventId::kNew_WndResize
};

WindowNew::WindowNew( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
    , pHostPanel_( GetHostPanelForCurrentGlobal( cx ) )
{
}

std::unique_ptr<WindowNew>
WindowNew::CreateNative( JSContext* cx )
{
    return std::unique_ptr<WindowNew>( new WindowNew( cx ) );
}

size_t WindowNew::GetInternalSize()
{
    return 0;
}

void WindowNew::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::MouseEvent>>( cx, self );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::WheelEvent>>( cx, self );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::KeyboardEvent>>( cx, self );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::Image>>( cx, self );
}

void WindowNew::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );

    auto pNative = JsObjectBase<WindowNew>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsRenderingContext_, "Heap: WindowNew: rendering context" );
}

const std::string& WindowNew::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_InputBlur, "blur" },
        { EventId::kNew_InputFocus, "focus" },
        { EventId::kNew_KeyboardKeyDown, "keydown" },
        { EventId::kNew_KeyboardKeyUp, "keyup" },
        { EventId::kNew_MouseButtonAuxClick, "auxclick" },
        { EventId::kNew_MouseButtonClick, "click" },
        { EventId::kNew_MouseButtonDoubleClick, "dblclick" },
        { EventId::kNew_MouseButtonDoubleClickSystem, "dblclicksystem" },
        { EventId::kNew_MouseButtonDown, "mousedown" },
        { EventId::kNew_MouseButtonUp, "mouseup" },
        { EventId::kNew_MouseContextMenu, "contextmenu" },
        { EventId::kNew_MouseEnter, "mouseenter" },
        { EventId::kNew_MouseLeave, "mouseleave" },
        { EventId::kNew_MouseMove, "mousemove" },
        { EventId::kNew_MouseWheel, "wheel" },
        { EventId::kNew_WndPaint, "redraw" },
        { EventId::kNew_WndResize, "resize" }
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus WindowNew::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

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

    JS::RootedObject jsEvent( pJsCtx_, GenerateEvent( event, eventType ) );
    JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
    DispatchEvent( self, jsEventValue );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

bool WindowNew::IsDevice() const
{
    return true;
}

Gdiplus::Graphics& WindowNew::GetGraphics()
{
    assert( pGraphics_ );
    return *pGraphics_;
}

uint32_t WindowNew::GetHeight() const
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );
    return pPanel->GetGraphics().GetHeight();
}

uint32_t WindowNew::GetWidth() const
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    return pPanel->GetGraphics().GetWidth();
}

void WindowNew::Alert( const qwr::u8string& message, const qwr::u8string& caption ) const
{
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    if ( !hPanel )
    {
        return;
    }

    if ( modal_dialog_scope::can_create() )
    {
        modal_dialog_scope scope( hPanel );

        smp::ui::CPopup dlg( message.c_str(), caption.c_str() );
        dlg.DoModal( hPanel );
    }
}

void WindowNew::AlertWithOpt( size_t optArgCount, const qwr::u8string& message, const qwr::u8string& caption ) const
{
    switch ( optArgCount )
    {
    case 0:
        return Alert( message, caption );
    case 1:
        return Alert( message );
    case 2:
        return Alert();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* WindowNew::GetContext( JS::HandleObject jsSelf, const std::wstring& contextType, JS::HandleValue attributes )
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    qwr::QwrException::ExpectTrue( pPanel->GetGraphics().IsPaintInProgress(), "Canvas context is not available" );
    qwr::QwrException::ExpectTrue( contextType == L"2d", L"Unsupported contextType value: {}", contextType );

    // TODO: handle attributes
    if ( !jsRenderingContext_.get() )
    {
        jsRenderingContext_ = JsObjectBase<CanvasRenderingContext2D_Qwr>::CreateJs( pJsCtx_, jsSelf, *this );

        JS::RootedObject jsRootedRenderingContext( pJsCtx_, jsRenderingContext_.get() );
        pNativeRenderingContext_ = JsObjectBase<CanvasRenderingContext2D_Qwr>::ExtractNative( pJsCtx_, jsRootedRenderingContext );
        assert( pNativeRenderingContext_ );
    }

    return jsRenderingContext_.get();
}

JSObject* WindowNew::GetContextWithOpt( JS::HandleObject jsSelf, size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes )
{
    switch ( optArgCount )
    {
    case 0:
        return GetContext( jsSelf, contextType, attributes );
    case 1:
        return GetContext( jsSelf, contextType );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* WindowNew::LoadImage( JS::HandleValue source )
{
    return Image::LoadImage( pJsCtx_, source );
}

JS::Value WindowNew::Prompt( const qwr::u8string& message, const qwr::u8string& defaultValue, const qwr::u8string& caption ) const
{
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    if ( !hPanel )
    {
        return JS::NullValue();
    }

    if ( modal_dialog_scope::can_create() )
    {
        modal_dialog_scope scope( hPanel );

        smp::ui::CInputBox dlg( message.c_str(), caption.c_str(), defaultValue.c_str() );
        int status = dlg.DoModal( hPanel );
        if ( status == IDOK )
        {
            JS::RootedValue jsValue( pJsCtx_ );
            convert::to_js::ToValue( pJsCtx_, dlg.GetValue(), &jsValue );
            return jsValue;
        }
    }

    return JS::NullValue();
}

JS::Value WindowNew::PromptWithOpt( size_t optArgCount, const qwr::u8string& message, const qwr::u8string& defaultValue, const qwr::u8string& caption ) const
{
    switch ( optArgCount )
    {
    case 0:
        return Prompt( message, defaultValue, caption );
    case 1:
        return Prompt( message, defaultValue );
    case 2:
        return Prompt( message );
    case 3:
        return Prompt();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void WindowNew::Redraw( bool force )
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    pPanel->Repaint( force );
}

void WindowNew::RedrawWithOpt( size_t optArgCount, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return Redraw( force );
    case 1:
        return Redraw();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void WindowNew::RedrawRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    pPanel->RepaintRect( CRect{ static_cast<int>( x ), static_cast<int>( y ), static_cast<int>( x + w ), static_cast<int>( y + h ) }, force );
}

void WindowNew::RedrawRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return RedrawRect( x, y, w, h, force );
    case 1:
        return RedrawRect( x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void WindowNew::ShowFb2kPopup( const qwr::u8string& message, const qwr::u8string& caption ) const
{
    qwr::DelayedExecutor::GetInstance().AddTask( [message, caption] {
        popup_message::g_show( message.c_str(), caption.c_str() );
    } );
}

void WindowNew::ShowFb2kPopupWithOpt( size_t optArgCount, const qwr::u8string& message, const qwr::u8string& caption ) const
{
    switch ( optArgCount )
    {
    case 0:
        return ShowFb2kPopup( message, caption );
    case 1:
        return ShowFb2kPopup( message );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t WindowNew::get_Height()
{
    return GetHeight();
}

uint32_t WindowNew::get_Width()
{
    return GetWidth();
}

JSObject* WindowNew::GenerateEvent( const smp::EventBase& event, const qwr::u8string& eventType )
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    JS::RootedObject jsEvent( pJsCtx_ );

    switch ( event.GetId() )
    {
    case EventId::kNew_KeyboardKeyDown:
    case EventId::kNew_KeyboardKeyUp:
    {
        const auto& keyboardEvent = static_cast<const smp::KeyboardEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::KeyboardEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GenerateKeyboardEventProps( keyboardEvent ) );
        break;
    }
    case EventId::kNew_MouseButtonAuxClick:
    case EventId::kNew_MouseButtonClick:
    case EventId::kNew_MouseButtonDoubleClick:
    case EventId::kNew_MouseButtonDoubleClickSystem:
    case EventId::kNew_MouseButtonDown:
    case EventId::kNew_MouseButtonUp:
    case EventId::kNew_MouseContextMenu:
    case EventId::kNew_MouseEnter:
    case EventId::kNew_MouseLeave:
    case EventId::kNew_MouseMove:
    {
        const auto& mouseEvent = static_cast<const smp::MouseEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::MouseEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GenerateMouseEventProps( mouseEvent, pPanel->GetHWND() ) );
        break;
    }
    case EventId::kNew_MouseWheel:
    {
        const auto& wheelEvent = static_cast<const smp::WheelEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::WheelEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GenerateWheelEventProps( wheelEvent, pPanel->GetHWND() ) );
        break;
    }
    default:
        jsEvent = mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } );
        break;
    }

    return jsEvent;
}

void WindowNew::HandlePaintEvent( JS::HandleObject self )
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    std::exception_ptr eptr;
    pPanel->GetGraphics().PaintWithCallback( [&]( Gdiplus::Graphics& gr ) {
        try
        {
            pGraphics_ = &gr;

            JS::RootedObject jsEvent( pJsCtx_,
                                      mozjs::JsObjectBase<JsEvent>::CreateJs( pJsCtx_, "redraw", JsEvent::EventProperties{} ) );
            auto pNativeEvent = mozjs::JsObjectBase<JsEvent>::ExtractNativeUnchecked( jsEvent );
            assert( pNativeEvent );

            JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
            DispatchEvent( self, jsEventValue );

            pGraphics_ = nullptr;
            jsRenderingContext_ = nullptr;
            pNativeRenderingContext_ = nullptr;
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
