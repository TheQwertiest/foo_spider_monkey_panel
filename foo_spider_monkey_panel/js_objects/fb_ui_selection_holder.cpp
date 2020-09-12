#include <stdafx.h>

#include "fb_ui_selection_holder.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/array_x.h>

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
    JsFbUiSelectionHolder::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbUiSelectionHolder",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE, // selection_holder must be finalized in foreground
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaylistSelectionTracking, JsFbUiSelectionHolder::SetPlaylistSelectionTracking )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaylistTracking, JsFbUiSelectionHolder::SetPlaylistTracking )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetSelection, JsFbUiSelectionHolder::SetSelection, JsFbUiSelectionHolder::SetSelectionWithOpt, 1 )

constexpr auto jsFunctions = smp::to_array<JSFunctionSpec>(
    {
        JS_FN( "SetPlaylistSelectionTracking", SetPlaylistSelectionTracking, 0, kDefaultPropsFlags ),
        JS_FN( "SetPlaylistTracking", SetPlaylistTracking, 0, kDefaultPropsFlags ),
        JS_FN( "SetSelection", SetSelection, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = smp::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbUiSelectionHolder::JsClass = jsClass;
const JSFunctionSpec* JsFbUiSelectionHolder::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbUiSelectionHolder::JsProperties = jsProperties.data();
const JsPrototypeId JsFbUiSelectionHolder::PrototypeId = JsPrototypeId::FbUiSelectionHolder;

JsFbUiSelectionHolder::JsFbUiSelectionHolder( JSContext* cx, const ui_selection_holder::ptr& holder )
    : pJsCtx_( cx )
    , holder_( holder )
{
}

std::unique_ptr<JsFbUiSelectionHolder>
JsFbUiSelectionHolder::CreateNative( JSContext* cx, const ui_selection_holder::ptr& holder )
{
    return std::unique_ptr<JsFbUiSelectionHolder>( new JsFbUiSelectionHolder( cx, holder ) );
}

size_t JsFbUiSelectionHolder::GetInternalSize( const ui_selection_holder::ptr& /*holder*/ )
{
    return 0;
}

void JsFbUiSelectionHolder::SetPlaylistSelectionTracking()
{
    holder_->set_playlist_selection_tracking();
}

void JsFbUiSelectionHolder::SetPlaylistTracking()
{
    holder_->set_playlist_tracking();
}

const std::array<const GUID*, 7> guids = {
    &contextmenu_item::caller_undefined,
    &contextmenu_item::caller_active_playlist_selection,
    &contextmenu_item::caller_active_playlist,
    &contextmenu_item::caller_playlist_manager,
    &contextmenu_item::caller_now_playing,
    &contextmenu_item::caller_keyboard_shortcut_list,
    &contextmenu_item::caller_media_library_viewer
};

void JsFbUiSelectionHolder::SetSelection( JsFbMetadbHandleList* handles, uint8_t type )

{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    holder_->set_selection_ex( handles->GetHandleList(), *guids.at( type ) );
}

void JsFbUiSelectionHolder::SetSelectionWithOpt( size_t optArgCount, JsFbMetadbHandleList* handles, uint8_t type )
{
    SmpException::ExpectTrue( handles, "handles argument is null" );

    switch ( optArgCount )
    {
    case 0:
        holder_->set_selection_ex( handles->GetHandleList(), *guids.at( type ) );
        break;
    case 1:
        optArgCount = optArgCount;
        holder_->set_selection_ex( handles->GetHandleList(), contextmenu_item::caller_undefined );
        break;
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

} // namespace mozjs
