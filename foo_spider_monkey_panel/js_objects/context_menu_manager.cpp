#include <stdafx.h>

#include "context_menu_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/menu_object.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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
    JsContextMenuManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ContextMenuManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( BuildMenu, JsContextMenuManager::BuildMenu, JsContextMenuManager::BuildMenuWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( ExecuteByID, JsContextMenuManager::ExecuteByID )
MJS_DEFINE_JS_FN_FROM_NATIVE( InitContext, JsContextMenuManager::InitContext )
MJS_DEFINE_JS_FN_FROM_NATIVE( InitContextPlaylist, JsContextMenuManager::InitContextPlaylist )
MJS_DEFINE_JS_FN_FROM_NATIVE( InitNowPlaying, JsContextMenuManager::InitNowPlaying )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "BuildMenu", BuildMenu, 2, kDefaultPropsFlags ),
        JS_FN( "ExecuteByID", ExecuteByID, 1, kDefaultPropsFlags ),
        JS_FN( "InitContext", InitContext, 1, kDefaultPropsFlags ),
        JS_FN( "InitContextPlaylist", InitContextPlaylist, 0, kDefaultPropsFlags ),
        JS_FN( "InitNowPlaying", InitNowPlaying, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsContextMenuManager::JsClass = jsClass;
const JSFunctionSpec* JsContextMenuManager::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsContextMenuManager::JsProperties = jsProperties.data();
const JsPrototypeId JsContextMenuManager::PrototypeId = JsPrototypeId::ContextMenuManager;

JsContextMenuManager::JsContextMenuManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

std::unique_ptr<mozjs::JsContextMenuManager>
JsContextMenuManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsContextMenuManager>( new JsContextMenuManager( cx ) );
}

size_t JsContextMenuManager::GetInternalSize()
{
    return 0;
}

void JsContextMenuManager::BuildMenu( JsMenuObject* menuObject, int32_t base_id, int32_t max_id )
{
    qwr::QwrException::ExpectTrue( contextMenu_.is_valid(), "Context menu is not initialized" );
    qwr::QwrException::ExpectTrue( menuObject, "menuObject argument is null" );

    contextMenu_->win32_build_menu( menuObject->HMenu(), contextMenu_->get_root(), base_id, max_id );
}

void JsContextMenuManager::BuildMenuWithOpt( size_t optArgCount, JsMenuObject* menuObject, int32_t base_id, int32_t max_id )
{
    switch ( optArgCount )
    {
    case 0:
        return BuildMenu( menuObject, base_id, max_id );
    case 1:
        return BuildMenu( menuObject, base_id );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool JsContextMenuManager::ExecuteByID( uint32_t id )
{
    qwr::QwrException::ExpectTrue( contextMenu_.is_valid(), "Context menu is not initialized" );

    return contextMenu_->execute_by_id( id );
}

void JsContextMenuManager::InitContext( JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    contextmenu_manager::g_create( contextMenu_ );
    contextMenu_->init_context( handles->GetHandleList(), contextmenu_manager::flag_show_shortcuts );
}

void JsContextMenuManager::InitContextPlaylist()
{
    contextmenu_manager::g_create( contextMenu_ );
    contextMenu_->init_context_playlist( contextmenu_manager::flag_show_shortcuts );
}

void JsContextMenuManager::InitNowPlaying()
{
    contextmenu_manager::g_create( contextMenu_ );
    contextMenu_->init_context_now_playing( contextmenu_manager::flag_show_shortcuts );
}

} // namespace mozjs
