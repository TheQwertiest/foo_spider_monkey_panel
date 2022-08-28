#include <stdafx.h>

#include "selection_manager.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/fb_metadb_handle_list.h>
#include <utils/guid_helpers.h>

#include <unordered_set>

using namespace smp;

namespace
{

const std::unordered_set<GUID, smp::utils::GuidHasher> kKnownSelectionTypes{
    contextmenu_item::caller_undefined,
    contextmenu_item::caller_active_playlist_selection,
    contextmenu_item::caller_active_playlist,
    contextmenu_item::caller_playlist_manager,
    contextmenu_item::caller_now_playing,
    contextmenu_item::caller_keyboard_shortcut_list,
    contextmenu_item::caller_media_library_viewer,
};

}

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
    JsObjectBase<SelectionManager>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    SelectionManager::Trace
};

JSClass jsClass = {
    "SelectionManager",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE, // selection_holder must be finalized in foreground,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( getSelection, SelectionManager::GetSelection, SelectionManager::GetSelectionWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( getSelectionType, SelectionManager::GetSelectionType )
MJS_DEFINE_JS_FN_FROM_NATIVE( trackPlaylistSelection, SelectionManager::SetPlaylistSelectionTracking )
MJS_DEFINE_JS_FN_FROM_NATIVE( trackPlaylist, SelectionManager::SetPlaylistTracking )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( setSelection, SelectionManager::SetSelection, SelectionManager::SetSelectionWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getSelection", getSelection, 0, kDefaultPropsFlags ),
        JS_FN( "getSelectionType", getSelectionType, 0, kDefaultPropsFlags ),
        JS_FN( "setSelection", setSelection, 1, kDefaultPropsFlags ),
        JS_FN( "trackPlaylist", trackPlaylist, 0, kDefaultPropsFlags ),
        JS_FN( "trackPlaylistSelection", trackPlaylistSelection, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass SelectionManager::JsClass = jsClass;
const JSFunctionSpec* SelectionManager::JsFunctions = jsFunctions.data();
const JsPrototypeId SelectionManager::BasePrototypeId = JsPrototypeId::EventTarget;
const JsPrototypeId SelectionManager::ParentPrototypeId = JsPrototypeId::EventTarget;

std::unordered_set<EventId> SelectionManager::kHandledEvents{
    EventId::kNew_FbSelectionChange,
};

SelectionManager::SelectionManager( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

std::unique_ptr<SelectionManager>
SelectionManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<SelectionManager>( new SelectionManager( cx ) );
}

void SelectionManager::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

void SelectionManager::PrepareForGc()
{
    JsEventTarget::PrepareForGc();
}

size_t SelectionManager::GetInternalSize()
{
    return 0;
}

void SelectionManager::PostCreate( JSContext* cx, JS::HandleObject self )
{
    // set constants
    {
        JS::RootedObject jsObject( cx, JS_NewPlainObject( cx ) );
        JsException::ExpectTrue( jsObject );

        static const auto props = std::to_array<JSPropertySpec>(
            {
                JS_INT32_PS( "SELECTION_FLAG_DEFAULT", 0, kDefaultPropsFlags | JSPROP_READONLY ),
                JS_INT32_PS( "SELECTION_FLAG_NO_NOW_PLAYING", ui_selection_manager_v2::flag_no_now_playing, kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_UNDEFINED", "00000000-0000-0000-0000-000000000000", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_ACTIVE_PLAYLIST_SELECTION", "47502BA1-816D-4A3E-ADE5-A7A9860A67DB", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_ACTIVE_PLAYLIST", "B3CC1030-EF26-45CF-A84A-7FC169BC9FFB", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_KEYBOARD_SHORTCUT_LIST", "FABEE3E9-8901-4DF4-A2D7-B9898D86C39B", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_MEDIA_LIBRARY_VIEWER", "FDA07C56-05D0-4B84-9FBD-A8BE556D474D", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_NOW_PLAYING", "994C0D0E-319E-45F3-92FC-518616E73ADC", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_STRING_PS( "SELECTION_TYPE_PLAYLIST_MANAGER", "5FDCD5E8-6EB2-4454-9EDA-527522893BED", kDefaultPropsFlags | JSPROP_READONLY ),
                JS_PS_END,
            } );
        if ( !JS_DefineProperties( cx, jsObject, props.data() ) )
        {
            throw smp::JsException();
        }

        if ( !JS_DefineProperty( cx, self, "constants", jsObject, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ) )
        {
            throw JsException();
        }
    }
}

const std::string& SelectionManager::EventIdToType( smp::EventId eventId )
{
    static std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbSelectionChange, "selectionChange" },
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

void SelectionManager::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return;
    }

    JS::RootedValue jsEvent( pJsCtx_ );
    jsEvent.setObjectOrNull( mozjs::JsEvent::CreateJs( pJsCtx_, eventType, false ) );
    DispatchEvent( self, jsEvent );
}

JSObject* SelectionManager::GetSelection( uint32_t flags )
{
    metadb_handle_list items;
    ui_selection_manager_v2::get()->get_selection( items, flags );

    return JsFbMetadbHandleList::CreateJs( pJsCtx_, items );
}

JSObject* SelectionManager::GetSelectionWithOpt( size_t optArgCount, uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return GetSelection( flags );
    case 1:
        return GetSelection();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

GUID SelectionManager::GetSelectionType()
{
    return ui_selection_manager_v2::get()->get_selection_type( 0 );
}

void SelectionManager::SetPlaylistSelectionTracking()
{
    if ( holder_.is_empty() )
    {
        holder_ = ui_selection_manager::get()->acquire();
    }
    holder_->set_playlist_selection_tracking();
}

void SelectionManager::SetPlaylistTracking()
{
    if ( holder_.is_empty() )
    {
        holder_ = ui_selection_manager::get()->acquire();
    }
    holder_->set_playlist_tracking();
}

void SelectionManager::SetSelection( JsFbMetadbHandleList* handles, const GUID& type )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );
    qwr::QwrException::ExpectTrue( kKnownSelectionTypes.contains( type ), L"Unknown selection holder type: {}", smp::utils::GuidToStr( type, true ) );

    if ( holder_.is_empty() )
    {
        holder_ = ui_selection_manager::get()->acquire();
    }

    holder_->set_selection_ex( handles->GetHandleList(), type );
}

void SelectionManager::SetSelectionWithOpt( size_t optArgCount, JsFbMetadbHandleList* handles, const GUID& type )
{
    switch ( optArgCount )
    {
    case 0:
        SetSelection( handles, type );
        break;
    case 1:
        SetSelection( handles );
        break;
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

} // namespace mozjs
