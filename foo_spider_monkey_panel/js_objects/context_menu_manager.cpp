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
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, BuildMenu )
MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, ExecuteByID )
MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, InitContext )
MJS_DEFINE_JS_TO_NATIVE_FN( JsContextMenuManager, InitNowPlaying )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "BuildMenu",  BuildMenu, 3, 0 ),
    JS_FN( "ExecuteByID",  ExecuteByID, 1, 0 ),
    JS_FN( "InitContext",  InitContext, 1, 0 ),
    JS_FN( "InitNowPlaying",  InitNowPlaying, 0, 0 ),
    JS_FS_END
};

}

namespace mozjs
{

JsContextMenuManager::JsContextMenuManager( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsContextMenuManager::~JsContextMenuManager()
{
}

JSObject* JsContextMenuManager::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsContextMenuManager( cx ) );

    return jsObj;
}

const JSClass& JsContextMenuManager::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t> 
JsContextMenuManager::BuildMenu( JsMenuObject* menuObject, int32_t base_id, int32_t max_id )
{
    if ( !menuObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "menuObject argument is null" );
        return std::nullopt;
    }

    if ( !contextMenu_.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Context menu is not initialized" );
        return std::nullopt;
    }

    HMENU hMenu = menuObject->HMenu();
    contextmenu_node* parent = contextMenu_->get_root();
    contextMenu_->win32_build_menu( hMenu, parent, base_id, max_id );
    return nullptr;
}

std::optional<bool> 
JsContextMenuManager::ExecuteByID( uint32_t id )
{
    if ( !contextMenu_.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Context menu is not initialized" );
        return std::nullopt;
    }

    return contextMenu_->execute_by_id( id );
}

std::optional<std::nullptr_t> 
JsContextMenuManager::InitContext( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
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
