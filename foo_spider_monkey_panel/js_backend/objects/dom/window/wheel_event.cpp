#include <stdafx.h>

#include "wheel_event.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/events/wheel_event.h>

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
    JsObjectBase<mozjs::WheelEvent>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "WheelEvent",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Get_DeltaX, mozjs::WheelEvent::get_DeltaX )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_DeltaY, mozjs::WheelEvent::get_DeltaY )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_DeltaZ, mozjs::WheelEvent::get_DeltaZ )
MJS_DEFINE_JS_FN_FROM_NATIVE( Get_DeltaMode, mozjs::WheelEvent::get_DeltaMode )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "deltaX", Get_DeltaX, kDefaultPropsFlags ),
        JS_PSG( "deltaY", Get_DeltaY, kDefaultPropsFlags ),
        JS_PSG( "deltaZ", Get_DeltaZ, kDefaultPropsFlags ),
        JS_PSG( "deltaMode", Get_DeltaMode, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( WheelEvent_Constructor, WheelEvent::Constructor )

MJS_VERIFY_OBJECT( mozjs::WheelEvent );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<WheelEvent>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<WheelEvent>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<WheelEvent>::PrototypeId = JsPrototypeId::New_WheelEvent;
const JSNative JsObjectTraits<WheelEvent>::JsConstructor = ::WheelEvent_Constructor;

WheelEvent::WheelEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
    : MouseEvent( cx, type, props.baseProps )
    , pJsCtx_( cx )
    , props_( props )
{
}

WheelEvent::WheelEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
    : WheelEvent( cx,
                  type,
                  options.ToDefaultProps() )
{
}

std::unique_ptr<WheelEvent>
WheelEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props )
{
    return std::unique_ptr<WheelEvent>( new mozjs::WheelEvent( cx, type, props ) );
}

std::unique_ptr<WheelEvent>
WheelEvent::CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options )
{
    return std::unique_ptr<WheelEvent>( new mozjs::WheelEvent( cx, type, options ) );
}

size_t WheelEvent::GetInternalSize()
{
    return 0;
}

JSObject* WheelEvent::Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options )
{
    return JsObjectBase<WheelEvent>::CreateJs( cx, type, ExtractOptions( cx, options ) );
}

JSObject* WheelEvent::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, type, options );
    case 1:
        return Constructor( cx, type );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

double WheelEvent::get_DeltaX() const
{
    return props_.deltaX;
}

double WheelEvent::get_DeltaY() const
{
    return props_.deltaY;
}

double WheelEvent::get_DeltaZ() const
{
    return props_.deltaZ;
}

int32_t WheelEvent::get_DeltaMode() const
{
    return props_.deltaMode;
}

WheelEvent::EventOptions WheelEvent::ExtractOptions( JSContext* cx, JS::HandleValue options )
{
    EventOptions parsedOptions;

    if ( options.isNullOrUndefined() )
    {
        return parsedOptions;
    }

    qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
    JS::RootedObject jsOptions( cx, &options.toObject() );

    parsedOptions.baseOptions = ParentJsType::ExtractOptions( cx, options );

    utils::OptionalPropertyTo( cx, jsOptions, "deltaX", parsedOptions.deltaX );
    utils::OptionalPropertyTo( cx, jsOptions, "deltaY", parsedOptions.deltaY );
    utils::OptionalPropertyTo( cx, jsOptions, "deltaZ", parsedOptions.deltaZ );

    utils::OptionalPropertyTo( cx, jsOptions, "deltaMode", parsedOptions.deltaMode );
    static std::unordered_set<int32_t> kAllowedDeltaModes = { 0, 1, 2 };
    qwr::QwrException::ExpectTrue( kAllowedDeltaModes.contains( parsedOptions.deltaMode ),
                                   "Unexpected deltaMode value: {}",
                                   parsedOptions.deltaMode );

    return parsedOptions;
}

WheelEvent::EventProperties WheelEvent::EventOptions::ToDefaultProps() const
{
    return {
        .baseProps = baseOptions.ToDefaultProps(),
        .deltaX = deltaX,
        .deltaY = deltaY,
        .deltaZ = deltaZ,
        .deltaMode = deltaMode
    };
}

} // namespace mozjs
