#include <stdafx.h>

#include "window_new.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
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
};

WindowNew::WindowNew( JSContext* cx, smp::panel::PanelWindow& parentPanel )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
    , parentPanel_( parentPanel )
{
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
    };

    assert( idToType.size() == kHandledEvents.size() );
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

    JS::RootedValue jsEvent( pJsCtx_ );
    jsEvent.setObjectOrNull( mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } ) );
    DispatchEvent( self, jsEvent );

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
