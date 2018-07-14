#include <stdafx.h>
#include "fb_ui_selection_holder.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>


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
    JsFinalizeOp<JsFbUiSelectionHolder>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbUiSelectionHolder",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUiSelectionHolder, SetPlaylistSelectionTracking )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUiSelectionHolder, SetPlaylistTracking )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUiSelectionHolder, SetSelection )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "SetPlaylistSelectionTracking", SetPlaylistTracking, 0, DefaultPropsFlags() ),
    JS_FN( "SetPlaylistTracking", SetPlaylistTracking, 0, DefaultPropsFlags() ),
    JS_FN( "SetSelection", SetSelection, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};


}

namespace mozjs
{

const JSClass JsFbUiSelectionHolder::JsClass = jsClass;
const JSFunctionSpec* JsFbUiSelectionHolder::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbUiSelectionHolder::JsProperties = jsProperties;
const JsPrototypeId JsFbUiSelectionHolder::PrototypeId = JsPrototypeId::FbUiSelectionHolder;

JsFbUiSelectionHolder::JsFbUiSelectionHolder( JSContext* cx, const ui_selection_holder::ptr& holder )
    : pJsCtx_( cx )
    , holder_(holder)
{
}


JsFbUiSelectionHolder::~JsFbUiSelectionHolder()
{
}

std::unique_ptr<JsFbUiSelectionHolder>
JsFbUiSelectionHolder::CreateNative( JSContext* cx, const ui_selection_holder::ptr& holder )
{
    return std::unique_ptr<JsFbUiSelectionHolder>( new JsFbUiSelectionHolder( cx, holder ) );
}

size_t JsFbUiSelectionHolder::GetInternalSize( const ui_selection_holder::ptr& holder )
{
    return 0;
}

std::optional<std::nullptr_t> 
JsFbUiSelectionHolder::SetPlaylistSelectionTracking()
{
    holder_->set_playlist_selection_tracking();
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbUiSelectionHolder::SetPlaylistTracking()
{
    holder_->set_playlist_tracking();
    return nullptr; 
}

std::optional<std::nullptr_t> 
JsFbUiSelectionHolder::SetSelection( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    holder_->set_selection( handles->GetHandleList() );
    return nullptr;
}

}
