#include <stdafx.h>

#include "ui_selection_manager.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/track_list.h>
#include <js_backend/utils/panel_from_global.h>
#include <panel/panel_accessor.h>
#include <panel/panel_window.h>
#include <utils/guid_helpers.h>

#include <qwr/algorithm.h>

#include <unordered_set>

using namespace smp;

namespace
{

const qwr::u8string kUnknownSelectionSource = "unknown";
const std::unordered_map<qwr::u8string, GUID> kSelectionSourceStrToGuid{
    { "active-playlist-content", contextmenu_item::caller_active_playlist },
    { "active-playlist-track-selection", contextmenu_item::caller_active_playlist_selection },
    { "currently-playing-track", contextmenu_item::caller_now_playing },
    { "keyboard-shortcut", contextmenu_item::caller_keyboard_shortcut_list },
    { "library-viewer", contextmenu_item::caller_media_library_viewer },
    { "playlist-manager", contextmenu_item::caller_playlist_manager },
    { "undefined", contextmenu_item::caller_undefined },
};
const auto kSelectionSourceGuidToStr = kSelectionSourceStrToGuid
                                       | ranges::views::transform( []( const auto& elem ) { return std::make_pair( elem.second, elem.first ); } )
                                       | ranges::to<std::unordered_map<GUID, std::string, smp::utils::GuidHasher>>();

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
    JsObjectBase<UiSelectionManager>::FinalizeJsObject,
    nullptr,
    nullptr,
    UiSelectionManager::Trace
};

JSClass jsClass = {
    "UiSelectionManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( getSelection, UiSelectionManager::GetSelection, UiSelectionManager::GetSelectionWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( getSelectionSource, UiSelectionManager::GetSelectionSource, UiSelectionManager::GetSelectionSourceWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( setSelection, UiSelectionManager::SetSelection, UiSelectionManager::SetSelectionWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( trackActivePlaylist, UiSelectionManager::TrackActivePlaylist )
MJS_DEFINE_JS_FN_FROM_NATIVE( trackActivePlaylistSelection, UiSelectionManager::TrackActivePlaylistSelection )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getSelection", getSelection, 0, kDefaultPropsFlags ),
        JS_FN( "getSelectionSource", getSelectionSource, 0, kDefaultPropsFlags ),
        JS_FN( "setSelection", setSelection, 1, kDefaultPropsFlags ),
        JS_FN( "trackActivePlaylist", trackActivePlaylist, 0, kDefaultPropsFlags ),
        JS_FN( "trackActivePlaylistSelection", trackActivePlaylistSelection, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::UiSelectionManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<UiSelectionManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<UiSelectionManager>::JsFunctions = jsFunctions.data();

const std::unordered_set<EventId> UiSelectionManager::kHandledEvents{
    EventId::kNew_FbAnySelectionChange,
    EventId::kNew_FbPreferedSelectionChange
};

UiSelectionManager::UiSelectionManager( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
    , pHostPanel_( GetHostPanelForCurrentGlobal( cx ) )
{
}

std::unique_ptr<UiSelectionManager>
UiSelectionManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<UiSelectionManager>( new UiSelectionManager( cx ) );
}

size_t UiSelectionManager::GetInternalSize()
{
    return 0;
}

void UiSelectionManager::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

const std::string& UiSelectionManager::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbAnySelectionChange, "anySelectionChange" },
        { EventId::kNew_FbPreferedSelectionChange, "preferedSelectionChange" },
    };

    assert( idToType.size() == kHandledEvents.size() );
    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus UiSelectionManager::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
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

JSObject* UiSelectionManager::GetSelection( bool ignoreCurrentlyPlaying )
{
    metadb_handle_list handles;
    ui_selection_manager_v2::get()->get_selection( handles, ignoreCurrentlyPlaying ? ui_selection_manager_v2::flag_no_now_playing : 0 );

    return TrackList::CreateJs( pJsCtx_, handles );
}

JSObject* UiSelectionManager::GetSelectionWithOpt( size_t optArgCount, bool ignoreCurrentlyPlaying )
{
    switch ( optArgCount )
    {
    case 0:
        return GetSelection( ignoreCurrentlyPlaying );
    case 1:
        return GetSelection();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

const qwr::u8string& UiSelectionManager::GetSelectionSource( bool ignoreCurrentlyPlaying )
{
    const auto selectionTypeGuid = ui_selection_manager_v2::get()->get_selection_type( ignoreCurrentlyPlaying ? ui_selection_manager_v2::flag_no_now_playing : 0 );
    const auto pSelectionTypeStr = qwr::FindAsPointer( kSelectionSourceGuidToStr, selectionTypeGuid );
    return ( pSelectionTypeStr ? *pSelectionTypeStr : kUnknownSelectionSource );
}

const qwr::u8string& UiSelectionManager::GetSelectionSourceWithOpt( size_t optArgCount, bool ignoreCurrentlyPlaying )
{
    switch ( optArgCount )
    {
    case 0:
        return GetSelectionSource( ignoreCurrentlyPlaying );
    case 1:
        return GetSelectionSource();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void UiSelectionManager::SetSelection( JS::HandleValue tracks, const qwr::u8string& type )
{
    const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );
    qwr::QwrException::ExpectTrue( kSelectionSourceStrToGuid.contains( type ), L"Unknown selection type" );

    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    auto pHolder = pPanel->GetSelectionHolder();
    qwr::QwrException::ExpectTrue( pHolder.is_valid(), "Setting UI selection while panel is not focused" );

    pHolder->set_selection_ex( handles, kSelectionSourceStrToGuid.at( type ) );
}

void UiSelectionManager::SetSelectionWithOpt( size_t optArgCount, JS::HandleValue tracks, const qwr::u8string& type )
{
    switch ( optArgCount )
    {
    case 0:
        return SetSelection( tracks, type );
    case 1:
        return SetSelection( tracks );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void UiSelectionManager::TrackActivePlaylist()
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    auto pHolder = pPanel->GetSelectionHolder();
    if ( !pHolder.is_valid() )
    { // means panel lost focus
        return;
    }

    pHolder->set_playlist_tracking();
}

void UiSelectionManager::TrackActivePlaylistSelection()
{
    auto pPanel = pHostPanel_->GetPanel();
    assert( pPanel );

    auto pHolder = pPanel->GetSelectionHolder();
    if ( !pHolder.is_valid() )
    { // means panel lost focus
        return;
    }

    pHolder->set_playlist_selection_tracking();
}

} // namespace mozjs
