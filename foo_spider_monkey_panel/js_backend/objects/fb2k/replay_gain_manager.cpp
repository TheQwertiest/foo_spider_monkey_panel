#include <stdafx.h>

#include "replay_gain_manager.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/utils/panel_from_global.h>
#include <panel/modal_blocking_scope.h>

using namespace smp;

namespace
{

const std::unordered_map<qwr::u8string, uint32_t> kSourceModeStrToIdx{
    { "none", t_replaygain_config::source_mode_none },
    { "track", t_replaygain_config::source_mode_track },
    { "album", t_replaygain_config::source_mode_album },
    { "playback-order-dependent", t_replaygain_config::source_mode_byPlaybackOrder },
};
const auto kSourceModeIdxToStr = kSourceModeStrToIdx
                                 | ranges::views::transform( []( const auto& elem ) { return std::make_pair( elem.second, elem.first ); } )
                                 | ranges::to<std::unordered_map>();

const std::unordered_map<qwr::u8string, uint32_t> kProcessingModeStrToIdx{
    { "none", t_replaygain_config::processing_mode_none },
    { "apply-gain", t_replaygain_config::processing_mode_gain },
    { "apply-gain-and-prevent-clipping", t_replaygain_config::processing_mode_gain_and_peak },
    { "prevent-clipping", t_replaygain_config::processing_mode_peak },
};
const auto kProcessingModeIdxToStr = kProcessingModeStrToIdx
                                     | ranges::views::transform( []( const auto& elem ) { return std::make_pair( elem.second, elem.first ); } )
                                     | ranges::to<std::unordered_map>();

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
    JsObjectBase<ReplayGainManager>::FinalizeJsObject,
    nullptr,
    nullptr,
    ReplayGainManager::Trace
};

JSClass jsClass = {
    "ReplayGainManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( showConfigure, ReplayGainManager::ShowConfigure );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "showConfigure", showConfigure, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_preampWithRG, ReplayGainManager::get_PreampWithRG );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_preampWithoutRG, ReplayGainManager::get_PreampWithoutRG );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_processingMode, ReplayGainManager::get_ProcessingMode );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_sourceMode, ReplayGainManager::get_SourceMode );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_preampWithRG, ReplayGainManager::put_PreampWithRG );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_preampWithoutRG, ReplayGainManager::put_PreampWithoutRG );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_processingMode, ReplayGainManager::put_ProcessingMode );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_sourceMode, ReplayGainManager::put_SourceMode );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "preampWithRG", get_preampWithRG, put_preampWithRG, kDefaultPropsFlags ),
        JS_PSGS( "preampWithoutRG", get_preampWithoutRG, put_preampWithoutRG, kDefaultPropsFlags ),
        JS_PSGS( "processingMode", get_processingMode, put_processingMode, kDefaultPropsFlags ),
        JS_PSGS( "sourceMode", get_sourceMode, put_sourceMode, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::ReplayGainManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ReplayGainManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<ReplayGainManager>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<ReplayGainManager>::JsProperties = jsProperties.data();

const std::unordered_set<smp::EventId> ReplayGainManager::kHandledEvents{
    EventId::kNew_FbReplayGainCfgChanged,
};

ReplayGainManager::ReplayGainManager( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

ReplayGainManager::~ReplayGainManager()
{
}

std::unique_ptr<ReplayGainManager>
ReplayGainManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<ReplayGainManager>( new ReplayGainManager( cx ) );
}

size_t ReplayGainManager::GetInternalSize() const
{
    return 0;
}

void ReplayGainManager::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

const std::string& ReplayGainManager::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbReplayGainCfgChanged, "settingsChange" },
    };

    assert( idToType.size() == kHandledEvents.size() );
    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus ReplayGainManager::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedValue jsEvent( pJsCtx_ );
    jsEvent.setObjectOrNull( mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } ) );

    DispatchEvent( self, jsEvent );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

void ReplayGainManager::ShowConfigure()
{
    if ( modal::IsModalBlocked() )
    {
        return;
    }

    modal::MessageBlockingScope scope;

    const auto api = replaygain_manager::get();

    auto cfg = api->get_core_settings();
    if ( api->configure_popup( cfg, GetPanelHwndForCurrentGlobal( pJsCtx_ ), nullptr ) )
    {
        api->set_core_settings( cfg );
    }
}

float ReplayGainManager::get_PreampWithRG() const
{
    const auto api = replaygain_manager::get();
    return api->get_core_settings().m_preamp_with_rg;
}

float ReplayGainManager::get_PreampWithoutRG() const
{
    const auto api = replaygain_manager::get();
    return api->get_core_settings().m_preamp_without_rg;
}

qwr::u8string ReplayGainManager::get_ProcessingMode() const
{

    const auto api = replaygain_manager::get();
    const auto cfg = api->get_core_settings();
    assert( kProcessingModeIdxToStr.contains( cfg.m_processing_mode ) );
    return kProcessingModeIdxToStr.at( cfg.m_processing_mode );
}

qwr::u8string ReplayGainManager::get_SourceMode() const
{
    const auto api = replaygain_manager::get();

    const auto cfg = api->get_core_settings();
    assert( kSourceModeIdxToStr.contains( cfg.m_source_mode ) );
    return kSourceModeIdxToStr.at( cfg.m_source_mode );
}

void ReplayGainManager::put_PreampWithRG( float value )
{
    const auto api = replaygain_manager::get();

    auto cfg = api->get_core_settings();
    if ( cfg.m_preamp_with_rg != value )
    {
        cfg.m_preamp_with_rg = value;
        api->set_core_settings( cfg );
    }
}

void ReplayGainManager::put_PreampWithoutRG( float value )
{
    const auto api = replaygain_manager::get();

    auto cfg = api->get_core_settings();
    if ( cfg.m_preamp_without_rg != value )
    {
        cfg.m_preamp_without_rg = value;
        api->set_core_settings( cfg );
    }
}

void ReplayGainManager::put_ProcessingMode( const qwr::u8string& value )
{
    qwr::QwrException::ExpectTrue( kProcessingModeStrToIdx.contains( value ), "Unknown processing mode" );

    const auto api = replaygain_manager::get();

    auto cfg = api->get_core_settings();
    const auto idx = kProcessingModeStrToIdx.at( value );
    if ( cfg.m_processing_mode != idx )
    {
        cfg.m_processing_mode = idx;
        api->set_core_settings( cfg );
    }
}

void ReplayGainManager::put_SourceMode( const qwr::u8string& value )
{
    qwr::QwrException::ExpectTrue( kSourceModeStrToIdx.contains( value ), "Unknown source mode" );

    const auto api = replaygain_manager::get();

    auto cfg = api->get_core_settings();
    const auto idx = kSourceModeStrToIdx.at( value );
    if ( cfg.m_source_mode != idx )
    {
        cfg.m_source_mode = idx;
        api->set_core_settings( cfg );
    }
}

} // namespace mozjs
