#include <stdafx.h>
#include "main_menu_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/menu_object.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/string_helpers.h>

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
    JsMainMenuManager::FinalizeJsObject,
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

MJS_DEFINE_JS_FN_FROM_NATIVE( BuildMenu, JsMainMenuManager::BuildMenu )
MJS_DEFINE_JS_FN_FROM_NATIVE( ExecuteByID, JsMainMenuManager::ExecuteByID )
MJS_DEFINE_JS_FN_FROM_NATIVE( Init, JsMainMenuManager::Init )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "BuildMenu", BuildMenu, 3, DefaultPropsFlags() ),
    JS_FN( "ExecuteByID", ExecuteByID, 1, DefaultPropsFlags() ),
    JS_FN( "Init", Init, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsMainMenuManager::JsClass = jsClass;
const JSFunctionSpec* JsMainMenuManager::JsFunctions = jsFunctions;
const JSPropertySpec* JsMainMenuManager::JsProperties = jsProperties;
const JsPrototypeId JsMainMenuManager::PrototypeId = JsPrototypeId::MainMenuManager;

JsMainMenuManager::JsMainMenuManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsMainMenuManager::~JsMainMenuManager()
{
}

std::unique_ptr<JsMainMenuManager>
JsMainMenuManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsMainMenuManager>( new JsMainMenuManager( cx ) );
}

size_t JsMainMenuManager::GetInternalSize()
{
    return sizeof( mainmenu_manager );
}

void JsMainMenuManager::BuildMenu( JsMenuObject* menu, int32_t base_id, int32_t count )
{
    SmpException::ExpectTrue( menuManager_.is_valid(), "Main menu manager is not initialized" );
    SmpException::ExpectTrue( menu, "menu argument is null" );

    // HACK: workaround for foo_menu_addons
    try
    {
        menuManager_->generate_menu_win32( menu->HMenu(), base_id, count, mainmenu_manager::flag_show_shortcuts );
    }
    catch ( ... )
    {
    }
}

bool JsMainMenuManager::ExecuteByID( uint32_t id )
{
    SmpException::ExpectTrue( menuManager_.is_valid(), "Main menu manager is not initialized" );

    return menuManager_->execute_command( id );
}

void JsMainMenuManager::Init( const pfc::string8_fast& root_name )
{
    const auto preparedRootName = [&root_name]() -> std::string {
        // Don't care about UTF8 here: we need exact match
        return std::string_view{ root_name.c_str() }
               | ranges::view::transform( []( auto i ) { return static_cast<char>( ::tolower( i ) ); } );
    }();

    struct RootElement
    {
        const char* name;
        const GUID* guid;
    };

    // In mainmenu_groups:
    // static const GUID file,view,edit,playback,library,help;
    const RootElement validRoots[] = {
        { "file", &mainmenu_groups::file },
        { "view", &mainmenu_groups::view },
        { "edit", &mainmenu_groups::edit },
        { "playback", &mainmenu_groups::playback },
        { "library", &mainmenu_groups::library },
        { "help", &mainmenu_groups::help },
    };

    auto result = ranges::find_if( validRoots, [&preparedRootName]( auto& root ) {
        return preparedRootName == root.name;
    } );
    if ( result == std::cend( validRoots ) )
    {
        throw SmpException( smp::string::Formatter() << "Invalid menu root name " << root_name.c_str() );
    }

    menuManager_ = standard_api_create_t<mainmenu_manager>();
    menuManager_->instantiate( *( result->guid ) );
}

} // namespace mozjs
