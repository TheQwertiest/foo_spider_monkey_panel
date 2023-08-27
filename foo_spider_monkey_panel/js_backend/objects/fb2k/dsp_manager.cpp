#include <stdafx.h>

#include "dsp_manager.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/utils/js_property_helper.h>
#include <js_backend/utils/panel_from_global.h>
#include <panel/modal_blocking_scope.h>
#include <tasks/events/event.h>

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
    JsObjectBase<DspManager>::FinalizeJsObject,
    nullptr,
    nullptr,
    DspManager::Trace
};

JSClass jsClass = {
    "DspManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( activateChainPreset, DspManager::ActivateChainPreset );
MJS_DEFINE_JS_FN_FROM_NATIVE( getActiveChainPreset, DspManager::GetActiveChainPreset );
MJS_DEFINE_JS_FN_FROM_NATIVE( getChainPresets, DspManager::GetChainPresets );
MJS_DEFINE_JS_FN_FROM_NATIVE( showConfigure, DspManager::ShowConfigure );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "activateChainPreset", activateChainPreset, 1, kDefaultPropsFlags ),
        JS_FN( "getActiveChainPreset", getActiveChainPreset, 0, kDefaultPropsFlags ),
        JS_FN( "getChainPresets", getChainPresets, 0, kDefaultPropsFlags ),
        JS_FN( "showConfigure", showConfigure, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::DspManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<DspManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<DspManager>::JsFunctions = jsFunctions.data();

const std::unordered_set<smp::EventId> DspManager::kHandledEvents{
    EventId::kNew_FbDspCoreSettingsChange
};

DspManager::DspManager( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

DspManager::~DspManager()
{
}

std::unique_ptr<DspManager>
DspManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<DspManager>( new DspManager( cx ) );
}

size_t DspManager::GetInternalSize() const
{
    return 0;
}

void DspManager::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

const std::string& DspManager::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbDspCoreSettingsChange, "activeChainChange" },
    };

    assert( idToType.size() == kHandledEvents.size() );
    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

mozjs::EventStatus DspManager::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedValue jsEvent( pJsCtx_ );
    jsEvent.setObjectOrNull( mozjs::JsEvent::CreateJs( pJsCtx_, eventType, mozjs::JsEvent::EventProperties{ .cancelable = false } ) );
    DispatchEvent( self, jsEvent );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

JS::Value DspManager::GetItem( uint32_t index ) const
{
    const auto api = dsp_config_manager_v2::get();
    qwr::QwrException::ExpectTrue( index < api->get_preset_count(), "Index is out of bounds" );

    JS::RootedObject jsItem( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    JsException::ExpectTrue( jsItem );

    pfc::string8_fast name;
    api->get_preset_name( index, name );

    utils::SetProperty( pJsCtx_, jsItem, "name", name );
    // TODO: implement DspPreset

    return JS::ObjectValue( *jsItem );
}

void DspManager::ActivateChainPreset( uint32_t index )
{
    const auto api = dsp_config_manager_v2::get();
    qwr::QwrException::ExpectTrue( index < api->get_preset_count(), "Index is out of bounds" );

    api->select_preset( index );
}

JS::Value DspManager::GetActiveChainPreset() const
{
    const auto api = dsp_config_manager_v2::get();
    auto index = api->get_selected_preset();
    if ( static_cast<int32_t>( index ) == -1 )
    {
        JS::RootedValue jsValue( pJsCtx_, JS::NullValue() );
        return jsValue;
    }

    return GetItem( index );
}

JS::Value DspManager::GetChainPresets() const
{
    const auto api = dsp_config_manager_v2::get();

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue(
        pJsCtx_,
        api->get_preset_count(),
        [&]( auto i ) { return GetItem( i ); },
        &jsValue );
    return jsValue;
}

void DspManager::ShowConfigure() const
{
    const auto api = dsp_config_manager_v2::get();
    const auto chainCount = api->get_preset_count();

    if ( modal::IsModalBlocked() )
    {
        return;
    }

    modal::MessageBlockingScope scope;

    dsp_chain_config_impl cfg{};
    api->get_core_settings( cfg );
    if ( api->configure_popup( cfg, GetPanelHwndForCurrentGlobal( pJsCtx_ ), nullptr ) )
    {
        api->set_core_settings( cfg );
    }
}

} // namespace mozjs
