#include <stdafx.h>

#include "context_menu_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/menu_object.h>
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
    JsFinalizeOp<JsContextMenuManager>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ContextMenuManager",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsContextMenuManager, BuildMenu, BuildMenuWithOpt, 1 )
MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, ExecuteByID )
MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, InitContext )
MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, InitNowPlaying )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "BuildMenu",  BuildMenu, 2, DefaultPropsFlags() ),
    JS_FN( "ExecuteByID",  ExecuteByID, 1, DefaultPropsFlags() ),
    JS_FN( "InitContext",  InitContext, 1, DefaultPropsFlags() ),
    JS_FN( "InitNowPlaying",  InitNowPlaying, 0, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsContextMenuManager::JsClass = jsClass;
const JSFunctionSpec* JsContextMenuManager::JsFunctions = jsFunctions;
const JSPropertySpec* JsContextMenuManager::JsProperties = jsProperties;
const JsPrototypeId JsContextMenuManager::PrototypeId = JsPrototypeId::ContextMenuManager;

JsContextMenuManager::JsContextMenuManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsContextMenuManager::~JsContextMenuManager()
{
}

std::unique_ptr<mozjs::JsContextMenuManager> 
JsContextMenuManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsContextMenuManager>( new JsContextMenuManager( cx ) );
}

std::optional<std::nullptr_t> 
JsContextMenuManager::BuildMenu( JsMenuObject* menuObject, int32_t base_id, int32_t max_id )
{
    if ( contextMenu_.is_empty() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Context menu is not initialized" );
        return std::nullopt;
    }

    if ( !menuObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "menuObject argument is null" );
        return std::nullopt;
    }

    HMENU hMenu = menuObject->HMenu();
    contextmenu_node* parent = contextMenu_->get_root();
    contextMenu_->win32_build_menu( hMenu, parent, base_id, max_id );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsContextMenuManager::BuildMenuWithOpt( size_t optArgCount, JsMenuObject* menuObject, int32_t base_id, int32_t max_id )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return BuildMenu( menuObject, base_id );
    }

    return BuildMenu( menuObject, base_id, max_id );
}

std::optional<bool> 
JsContextMenuManager::ExecuteByID( uint32_t id )
{
    if ( contextMenu_.is_empty() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Context menu is not initialized" );
        return std::nullopt;
    }

    return contextMenu_->execute_by_id( id );
}

std::optional<std::nullptr_t> 
JsContextMenuManager::InitContext( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadb_handle_list_cref handles_ptr = handles->GetHandleList();
    contextmenu_manager::g_create( contextMenu_ );
    contextMenu_->init_context( handles_ptr, contextmenu_manager::flag_show_shortcuts );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsContextMenuManager::InitNowPlaying()
{
    contextmenu_manager::g_create( contextMenu_ );
    contextMenu_->init_context_now_playing( contextmenu_manager::flag_show_shortcuts );
    return nullptr;
}

}
