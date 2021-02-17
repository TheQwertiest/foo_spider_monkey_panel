#include <stdafx.h>

#include "fb_ui_selection_holder.h"

#include <fb2k/selection_holder_helper.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "SetPlaylistSelectionTracking", SetPlaylistSelectionTracking, 0, kDefaultPropsFlags ),
        JS_FN( "SetPlaylistTracking", SetPlaylistTracking, 0, kDefaultPropsFlags ),
        JS_FN( "SetSelection", SetSelection, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
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

void JsFbUiSelectionHolder::SetSelection( JsFbMetadbHandleList* handles, uint8_t type )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    const auto holderGuidOpt = GetSelectionHolderGuidFromType( type );
    qwr::QwrException::ExpectTrue( holderGuidOpt.has_value(), "Unknown selection holder type: {}", type );

    holder_->set_selection_ex( handles->GetHandleList(), *holderGuidOpt );
}

void JsFbUiSelectionHolder::SetSelectionWithOpt( size_t optArgCount, JsFbMetadbHandleList* handles, uint8_t type )
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
