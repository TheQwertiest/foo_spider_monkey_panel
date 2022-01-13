#include <stdafx.h>

#include "mainmenu_dynamic.h"

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>

#include <component_paths.h>

#include <map>

using namespace smp;

namespace
{

class MainMenuNodeCommand_PanelCommand : public mainmenu_node_command
{
public:
    MainMenuNodeCommand_PanelCommand( HWND panelHwnd,
                                      const qwr::u8string& panelName,
                                      uint32_t commandId,
                                      const qwr::u8string& commandName,
                                      const std::optional<qwr::u8string>& commandDescription );

    void get_display( pfc::string_base& text, t_uint32& flags ) override;
    void execute( service_ptr_t<service_base> callback ) override;
    GUID get_guid() override;
    bool get_description( pfc::string_base& out ) override;

private:
    const HWND panelHwnd_;
    const qwr::u8string panelName_;
    const uint32_t commandId_;
    const qwr::u8string commandName_;
    const std::optional<qwr::u8string> commandDescriptionOpt_;
};

class MainMenuNodeGroup_PanelCommands : public mainmenu_node_group
{
public:
    MainMenuNodeGroup_PanelCommands( HWND panelHwnd,
                                     const qwr::u8string& panelName,
                                     const std::unordered_map<uint32_t, DynamicMainMenuManager::CommandData>& idToCommand );
    void get_display( pfc::string_base& text, t_uint32& flags ) override;
    t_size get_children_count() override;
    mainmenu_node::ptr get_child( t_size index ) override;

private:
    const smp::DynamicMainMenuManager::PanelData panelData_;
    const qwr::u8string panelName_;
    std::vector<mainmenu_node::ptr> commandNodes_;
};

class MainMenuNodeGroup_Panels : public mainmenu_node_group
{
public:
    MainMenuNodeGroup_Panels();
    void get_display( pfc::string_base& text, t_uint32& flags ) override;
    t_size get_children_count() override;
    mainmenu_node::ptr get_child( t_size index ) override;

private:
    std::vector<mainmenu_node::ptr> panelNodes_;
};

class MainMenuCommands_Panels : public mainmenu_commands_v2
{
public:
    // mainmenu_commands
    t_uint32 get_command_count() override;
    GUID get_command( t_uint32 p_index ) override;
    void get_name( t_uint32 p_index, pfc::string_base& p_out ) override;
    bool get_description( t_uint32 p_index, pfc::string_base& p_out ) override;
    GUID get_parent() override;
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) override;

    // mainmenu_commands_v2
    bool is_command_dynamic( t_uint32 index ) override;
    mainmenu_node::ptr dynamic_instantiate( t_uint32 index ) override;
    bool dynamic_execute( t_uint32 index, const GUID& subID, service_ptr_t<service_base> callback ) override;
};

} // namespace

namespace
{

MainMenuNodeCommand_PanelCommand::MainMenuNodeCommand_PanelCommand( HWND panelHwnd,
                                                                    const qwr::u8string& panelName,
                                                                    uint32_t commandId,
                                                                    const qwr::u8string& commandName,
                                                                    const std::optional<qwr::u8string>& commandDescription )
    : panelHwnd_( panelHwnd )
    , panelName_( panelName )
    , commandId_( commandId )
    , commandName_( commandName )
    , commandDescriptionOpt_( commandDescription )
{
}

void MainMenuNodeCommand_PanelCommand::get_display( pfc::string_base& text, t_uint32& flags )
{
    text.clear();
    text << commandName_;
    flags = 0;
}

void MainMenuNodeCommand_PanelCommand::execute( service_ptr_t<service_base> callback )
{
    EventDispatcher::Get().PutEvent( panelHwnd_, GenerateEvent_JsCallback( EventId::kDynamicMainMenu, commandId_ ) );
}

GUID MainMenuNodeCommand_PanelCommand::get_guid()
{
    auto api = hasher_md5::get();
    hasher_md5_state state;
    api->initialize( state );
    // process termination character as well - it will act as a separator
    api->process( state, panelName_.data(), panelName_.size() + 1 );
    api->process( state, &commandId_, sizeof( commandId_ ) );

    return api->get_result_guid( state );
}

bool MainMenuNodeCommand_PanelCommand::get_description( pfc::string_base& out )
{
    if ( !commandDescriptionOpt_ )
    {
        return false;
    }

    out.clear();
    out << *commandDescriptionOpt_;
    return true;
}

MainMenuNodeGroup_PanelCommands::MainMenuNodeGroup_PanelCommands( HWND panelHwnd,
                                                                  const qwr::u8string& panelName,
                                                                  const std::unordered_map<uint32_t, DynamicMainMenuManager::CommandData>& idToCommand )
    : panelName_( panelName )
{
    // use map to sort commands by their name
    const auto commandNameToId =
        idToCommand
        | ranges::views::transform( []( const auto& elem ) { return std::make_pair( elem.second.name, elem.first ); } )
        | ranges::to<std::multimap>;
    for ( const auto& [name, id]: commandNameToId )
    {
        const auto& command = idToCommand.at( id );
        commandNodes_.emplace_back( fb2k::service_new<MainMenuNodeCommand_PanelCommand>( panelHwnd, panelName_, id, command.name, command.description ) );
    }
}

void MainMenuNodeGroup_PanelCommands::get_display( pfc::string_base& text, t_uint32& flags )
{
    text.clear();
    text << panelName_;
    flags = mainmenu_commands::flag_defaulthidden | mainmenu_commands::sort_priority_base;
}

t_size MainMenuNodeGroup_PanelCommands::get_children_count()
{
    return commandNodes_.size();
}

mainmenu_node::ptr MainMenuNodeGroup_PanelCommands::get_child( t_size index )
{
    assert( index < commandNodes_.size() );
    return commandNodes_.at( index );
}

MainMenuNodeGroup_Panels::MainMenuNodeGroup_Panels()
{
    const auto panels = smp::DynamicMainMenuManager::Get().GetAllCommandData();
    // use map to sort panels by their name
    const auto panelNameToHWnd =
        panels
        | ranges::views::transform( []( const auto& elem ) { return std::make_pair( elem.second.name, elem.first ); } )
        | ranges::to<std::map>;
    for ( const auto& [name, hWnd]: panelNameToHWnd )
    {
        const auto& panelData = panels.at( hWnd );
        if ( panelData.commands.empty() )
        {
            continue;
        }

        panelNodes_.emplace_back( fb2k::service_new<MainMenuNodeGroup_PanelCommands>( hWnd, panelData.name, panelData.commands ) );
    }
}

void MainMenuNodeGroup_Panels::get_display( pfc::string_base& text, t_uint32& flags )
{
    text = "Script commands";
    flags = mainmenu_commands::flag_defaulthidden | mainmenu_commands::sort_priority_base;
}

t_size MainMenuNodeGroup_Panels::get_children_count()
{
    return panelNodes_.size();
}

mainmenu_node::ptr MainMenuNodeGroup_Panels::get_child( t_size index )
{
    assert( index < panelNodes_.size() );
    return panelNodes_.at( index );
}

t_uint32 MainMenuCommands_Panels::get_command_count()
{
    return 1;
}

GUID MainMenuCommands_Panels::get_command( t_uint32 /*p_index*/ )
{
    return smp::guid::menu_script_commands;
}

void MainMenuCommands_Panels::get_name( t_uint32 /*p_index*/, pfc::string_base& p_out )
{
    p_out = "Script commands";
}

bool MainMenuCommands_Panels::get_description( t_uint32 /*p_index*/, pfc::string_base& p_out )
{
    p_out = "Commands provided by scripts.";
    return true;
}

GUID MainMenuCommands_Panels::get_parent()
{
    return smp::guid::mainmenu_group_predefined;
}

void MainMenuCommands_Panels::execute( t_uint32 /*p_index*/, service_ptr_t<service_base> /*p_callback*/ )
{
    // Should not get here, someone not aware of our dynamic status tried to invoke us?
}

bool MainMenuCommands_Panels::is_command_dynamic( t_uint32 /*index*/ )
{
    return true;
}

mainmenu_node::ptr MainMenuCommands_Panels::dynamic_instantiate( t_uint32 /*index*/ )
{
    return fb2k::service_new<MainMenuNodeGroup_Panels>();
}

bool MainMenuCommands_Panels::dynamic_execute( t_uint32 index, const GUID& subID, service_ptr_t<service_base> callback )
{
    return __super::dynamic_execute( index, subID, callback );
}
} // namespace

namespace
{

FB2K_SERVICE_FACTORY( MainMenuCommands_Panels );

} // namespace

namespace smp
{

DynamicMainMenuManager& DynamicMainMenuManager::Get()
{
    static DynamicMainMenuManager dmmm;
    return dmmm;
}

void DynamicMainMenuManager::RegisterPanel( HWND hWnd, const qwr::u8string& panelName )
{
    assert( !panels_.contains( hWnd ) );
    panels_.try_emplace( hWnd, PanelData{ panelName } );
}

void DynamicMainMenuManager::UnregisterPanel( HWND hWnd )
{
    // don't check hWnd presence, since this might be called several times during error handling
    panels_.erase( hWnd );
}

void DynamicMainMenuManager::RegisterCommand( HWND hWnd, uint32_t id, const qwr::u8string& name, const std::optional<qwr::u8string>& description )
{
    assert( panels_.contains( hWnd ) );

    auto& panelData = panels_.at( hWnd );
    qwr::QwrException::ExpectTrue( !panelData.commands.contains( id ), "Command with id `{}` was already registered", id );

    panelData.commands.try_emplace( id, CommandData{ name, description } );
}

void DynamicMainMenuManager::UnregisterCommand( HWND hWnd, uint32_t id )
{
    assert( panels_.contains( hWnd ) );

    auto& panelData = panels_.at( hWnd );
    qwr::QwrException::ExpectTrue( panelData.commands.contains( id ), "Unknown command id `{}`", id );

    panelData.commands.erase( id );
}

const std::unordered_map<HWND, DynamicMainMenuManager::PanelData>&
DynamicMainMenuManager::GetAllCommandData() const
{
    return panels_;
}

} // namespace smp
