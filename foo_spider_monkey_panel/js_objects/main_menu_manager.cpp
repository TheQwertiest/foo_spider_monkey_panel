#include <stdafx.h>
#include "main_menu_manager.h"

#include <js_engine/js_to_native_invoker.h>
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
    JsFinalizeOp<JsMainMenuManager>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MainMenuManager",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsMainMenuManager, BuildMenu )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMainMenuManager, ExecuteByID )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMainMenuManager, Init )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "BuildMenu", BuildMenu, 3, DefaultPropsFlags() ),
    JS_FN( "ExecuteByID", ExecuteByID, 1, DefaultPropsFlags() ),
    JS_FN( "Init", Init, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {

    JS_PS_END
};

}

namespace mozjs
{


JsMainMenuManager::JsMainMenuManager( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsMainMenuManager::~JsMainMenuManager()
{
}

JSObject* JsMainMenuManager::Create( JSContext* cx )
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

    JS_SetPrivate( jsObj, new JsMainMenuManager( cx ) );

    return jsObj;
}

const JSClass& JsMainMenuManager::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t> 
JsMainMenuManager::BuildMenu( JsMenuObject* menu, int32_t base_id, int32_t count )
{
    if ( menuManager_.is_empty() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Main menu manager is not initialized" );
        return std::nullopt;
    }

    if ( !menu )
    {
        JS_ReportErrorUTF8( pJsCtx_, "menu argument is null" );
        return std::nullopt;
    }

    // HACK: workaround for foo_menu_addons
    try
    {
        menuManager_->generate_menu_win32( menu->HMenu(), base_id, count, mainmenu_manager::flag_show_shortcuts );
    }
    catch ( ... )
    {
    }

    return nullptr;
}

std::optional<bool> 
JsMainMenuManager::ExecuteByID( uint32_t id )
{
    if ( menuManager_.is_empty() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Main menu manager is not initialized" );
        return std::nullopt;
    }

    return menuManager_->execute_command( id );
}

std::optional<std::nullptr_t> 
JsMainMenuManager::Init( const pfc::string8_fast & root_name )
{
    std::string preparedRootName( root_name.c_str() ); ///< Don't care about UTF8 here: we need exact match
    std::transform( preparedRootName.begin(), preparedRootName.end(), preparedRootName.begin(), ::tolower );    

    struct RootElement
    {
        const char* name;
        const GUID* guid;
    };

    // In mainmenu_groups:
    // static const GUID file,view,edit,playback,library,help;
    const RootElement validRoots[] =
    {
        { "file", &mainmenu_groups::file },
        { "view", &mainmenu_groups::view },
        { "edit", &mainmenu_groups::edit },
        { "playback", &mainmenu_groups::playback },
        { "library", &mainmenu_groups::library },
        { "help", &mainmenu_groups::help },
    };

    // Find
    for ( int i = 0; i < _countof( validRoots ); ++i )
    {
        if ( preparedRootName == validRoots[i].name )
        {// found
            menuManager_ = standard_api_create_t<mainmenu_manager>();
            menuManager_->instantiate( *(validRoots[i].guid) );
            return nullptr;
        }
    }

    JS_ReportErrorUTF8( pJsCtx_, "Invalid menu root name" );
    return std::nullopt;
}

}
