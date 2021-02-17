#include <stdafx.h>

#include "main_menu_manager.h"

#include <js_engine/js_to_native_invoker.h>
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
    JsMainMenuManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MainMenuManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( BuildMenu, JsMainMenuManager::BuildMenu )
MJS_DEFINE_JS_FN_FROM_NATIVE( ExecuteByID, JsMainMenuManager::ExecuteByID )
MJS_DEFINE_JS_FN_FROM_NATIVE( Init, JsMainMenuManager::Init )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "BuildMenu", BuildMenu, 3, kDefaultPropsFlags ),
        JS_FN( "ExecuteByID", ExecuteByID, 1, kDefaultPropsFlags ),
        JS_FN( "Init", Init, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsMainMenuManager::JsClass = jsClass;
const JSFunctionSpec* JsMainMenuManager::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsMainMenuManager::JsProperties = jsProperties.data();
const JsPrototypeId JsMainMenuManager::PrototypeId = JsPrototypeId::MainMenuManager;

JsMainMenuManager::JsMainMenuManager( JSContext* cx )
    : pJsCtx_( cx )
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
    qwr::QwrException::ExpectTrue( menuManager_.is_valid(), "Main menu manager is not initialized" );
    qwr::QwrException::ExpectTrue( menu, "menu argument is null" );

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
    qwr::QwrException::ExpectTrue( menuManager_.is_valid(), "Main menu manager is not initialized" );

    return menuManager_->execute_command( id );
}

void JsMainMenuManager::Init( const qwr::u8string& root_name )
{
    const auto preparedRootName = [&root_name]() {
        // Don't care about UTF8 here: we need exact match
        return std::string_view{ root_name }
               | ranges::views::transform( []( auto i ) { return static_cast<char>( ::tolower( i ) ); } )
               | ranges::to<std::string>;
    }();

    struct RootElement
    {
        const char* name;
        const GUID* guid;
    };

    // In mainmenu_groups:
    // static const GUID file,view,edit,playback,library,help;
    const auto validRoots = std::to_array<RootElement>(
        {
            { "file", &mainmenu_groups::file },
            { "view", &mainmenu_groups::view },
            { "edit", &mainmenu_groups::edit },
            { "playback", &mainmenu_groups::playback },
            { "library", &mainmenu_groups::library },
            { "help", &mainmenu_groups::help },
        } );

    auto result = ranges::find_if( validRoots, [&preparedRootName]( auto& root ) {
        return preparedRootName == root.name;
    } );
    qwr::QwrException::ExpectTrue( result != std::cend( validRoots ), "Invalid menu root name: {}", root_name );

    menuManager_ = standard_api_create_t<mainmenu_manager>();
    menuManager_->instantiate( *( result->guid ) );
}

} // namespace mozjs
