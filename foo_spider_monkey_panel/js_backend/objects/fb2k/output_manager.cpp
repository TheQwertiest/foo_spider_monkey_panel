#include <stdafx.h>

#include "output_manager.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/events/event.h>
#include <utils/guid_helpers.h>

#include <qwr/string_helpers.h>

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
    JsObjectBase<OutputManager>::FinalizeJsObject,
    nullptr,
    nullptr,
    OutputManager::Trace
};

JSClass jsClass = {
    "OutputManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( getActiveDeviceOutput, OutputManager::GetActiveDeviceOutput );
MJS_DEFINE_JS_FN_FROM_NATIVE( getDeviceOutputs, OutputManager::GetDeviceOutputs );
MJS_DEFINE_JS_FN_FROM_NATIVE( setActiveDeviceOutput, OutputManager::SetActiveDeviceOutput );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getActiveDeviceOutput", getActiveDeviceOutput, 0, kDefaultPropsFlags ),
        JS_FN( "getDeviceOutputs", getDeviceOutputs, 0, kDefaultPropsFlags ),
        JS_FN( "setActiveDeviceOutput", setActiveDeviceOutput, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::OutputManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<OutputManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<OutputManager>::JsFunctions = jsFunctions.data();

const std::unordered_set<smp::EventId> OutputManager::kHandledEvents = {
    smp::EventId::kNew_FbOutputConfigChange
};

OutputManager::OutputManager( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

OutputManager::~OutputManager()
{
}

std::unique_ptr<OutputManager>
OutputManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<OutputManager>( new OutputManager( cx ) );
}

size_t OutputManager::GetInternalSize() const
{
    return 0;
}

void OutputManager::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

const std::string& OutputManager::EventIdToType( smp::EventId eventId ) const
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbOutputConfigChange, "activeDeviceOutputChange" },
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus OutputManager::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedValue jsEvent( pJsCtx_ );
    jsEvent.setObjectOrNull( JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } ) );
    DispatchEvent( self, jsEvent );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

JSObject* OutputManager::GetActiveDeviceOutput() const
{
    auto api = output_manager_v2::get();

    JS::RootedObject jsItem( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    JsException::ExpectTrue( jsItem );

    outputCoreConfig_t config{};
    api->getCoreConfig( config );

    utils::SetProperty( pJsCtx_, jsItem, "outputId", pfc::print_guid( config.m_output ) );
    utils::SetProperty( pJsCtx_, jsItem, "deviceId", pfc::print_guid( config.m_device ) );
    if ( config.m_bitDepth )
    {
        utils::SetProperty( pJsCtx_, jsItem, "bitDepth", config.m_bitDepth );
    }

    output_entry::ptr outputEntry;
    if ( output_entry::g_find( config.m_output, outputEntry ) )
    {
        utils::SetProperty( pJsCtx_, jsItem, "name", outputEntry->get_device_name( config.m_device ) );
    }

    return jsItem.get();
}

JS::Value OutputManager::GetDeviceOutputs() const
{
    auto api = output_manager_v2::get();

    JS::RootedValue jsValue( pJsCtx_ );
    JS::RootedObject jsItem( pJsCtx_ );

    struct Device
    {
        qwr::u8string name;
        GUID output_id;
        GUID device_id;
    };

    std::vector<Device> devices;
    api->listDevices( [&]( const qwr::u8string& name, const GUID& output_id, const GUID& device_id ) {
        devices.emplace_back( name, output_id, device_id );
    } );

    // TODO: add prototype
    convert::to_js::ToArrayValue(
        pJsCtx_,
        devices.size(),
        [&]( auto i ) {
            const auto& device = devices[i];

            jsItem = JS_NewPlainObject( pJsCtx_ );
            JsException::ExpectTrue( jsItem );

            utils::SetProperty( pJsCtx_, jsItem, "name", device.name );
            utils::SetProperty( pJsCtx_, jsItem, "outputId", pfc::print_guid( device.output_id ) );
            utils::SetProperty( pJsCtx_, jsItem, "deviceId", pfc::print_guid( device.device_id ) );
            return jsItem.get();
        },
        &jsValue );
    return jsValue;
}

void OutputManager::SetActiveDeviceOutput( const std::wstring& outputId, const std::wstring& deviceId )
{
    auto api = output_manager_v2::get();

    const auto outputIdOpt = smp::utils::StrToGuid( outputId );
    qwr::QwrException::ExpectTrue( outputIdOpt.has_value(), "Invalid output id" );
    const auto deviceIdOpt = smp::utils::StrToGuid( deviceId );
    qwr::QwrException::ExpectTrue( deviceIdOpt.has_value(), "Invalid device id" );

    api->setCoreConfigDevice( *outputIdOpt, *deviceIdOpt );
}

} // namespace mozjs
