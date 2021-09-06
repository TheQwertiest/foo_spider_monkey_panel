#include <stdafx.h>

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>

#include <component_paths.h>

using namespace smp;

namespace
{

class MainMenuCommands_Predefined : public mainmenu_commands
{
public:
    MainMenuCommands_Predefined();

    t_uint32 get_command_count() override;
    GUID get_command( t_uint32 p_index ) override;
    void get_name( t_uint32 p_index, pfc::string_base& p_out ) override;
    bool get_description( t_uint32 p_index, pfc::string_base& p_out ) override;
    GUID get_parent() override;
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) override;
    bool get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags ) override;

private:
    const std::array<GUID, 10> menuObjects_;
};

class MainMenuCommands_Help : public mainmenu_commands
{
public:
    MainMenuCommands_Help() = default;

    t_uint32 get_command_count() override;
    GUID get_command( t_uint32 p_index ) override;
    void get_name( t_uint32 p_index, pfc::string_base& p_out ) override;
    bool get_description( t_uint32 p_index, pfc::string_base& p_out ) override;
    GUID get_parent() override;
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) override;
    bool get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags ) override;
};

} // namespace

namespace
{

MainMenuCommands_Predefined::MainMenuCommands_Predefined()
    : menuObjects_{ smp::guid::menu_1,
                    smp::guid::menu_2,
                    smp::guid::menu_3,
                    smp::guid::menu_4,
                    smp::guid::menu_5,
                    smp::guid::menu_6,
                    smp::guid::menu_7,
                    smp::guid::menu_8,
                    smp::guid::menu_9,
                    smp::guid::menu_10 }
{
}

t_uint32 MainMenuCommands_Predefined::get_command_count()
{
    return menuObjects_.size();
}

GUID MainMenuCommands_Predefined::get_command( t_uint32 p_index )
{
    if ( p_index >= menuObjects_.size() )
    {
        uBugCheck();
        return pfc::guid_null;
    }

    return menuObjects_[p_index];
}

void MainMenuCommands_Predefined::get_name( t_uint32 p_index, pfc::string_base& p_out )
{
    if ( p_index >= menuObjects_.size() )
    {
        uBugCheck();
    }

    p_out.reset();
    p_out << ( p_index + 1 );
}

bool MainMenuCommands_Predefined::get_description( t_uint32 /* p_index */, pfc::string_base& p_out )
{
    p_out = "Invoke on_main_menu()";
    return true;
}

GUID MainMenuCommands_Predefined::get_parent()
{
    return guid::mainmenu_group_predefined;
}

void MainMenuCommands_Predefined::execute( t_uint32 p_index, service_ptr_t<service_base> )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kStaticMainMenu, p_index + 1 ), EventPriority::kInput );
}

bool MainMenuCommands_Predefined::get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags )
{
    get_name( p_index, p_out );
    p_flags = mainmenu_commands::flag_defaulthidden | ( mainmenu_commands::sort_priority_base * 1000 );
    return true;
}

t_uint32 MainMenuCommands_Help::get_command_count()
{
    return 1;
}

GUID MainMenuCommands_Help::get_command( t_uint32 p_index )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return pfc::guid_null;
    }

    return guid::mainmenu_node_help_docs;
}

void MainMenuCommands_Help::get_name( t_uint32 p_index, pfc::string_base& p_out )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return;
    }

    p_out.reset();
    p_out << "Spider Monkey Panel help";
}

bool MainMenuCommands_Help::get_description( t_uint32 p_index, pfc::string_base& p_out )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return false;
    }

    p_out = "View Spider Monkey Panel documentation files";
    return true;
}

GUID MainMenuCommands_Help::get_parent()
{
    return mainmenu_groups::help;
}

void MainMenuCommands_Help::execute( t_uint32 p_index, service_ptr_t<service_base> )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return;
    }
    ShellExecute( nullptr, L"open", path::JsDocsIndex().c_str(), nullptr, nullptr, SW_SHOW );
}

bool MainMenuCommands_Help::get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return false;
    }

    get_name( p_index, p_out );
    p_flags = mainmenu_commands::sort_priority_dontcare;
    return true;
}

} // namespace

namespace
{

mainmenu_group_popup_factory g_mainmenu_group_predefined(
    smp::guid::mainmenu_group_predefined, mainmenu_groups::file, static_cast<t_uint32>( mainmenu_commands::sort_priority_dontcare ), SMP_NAME );

FB2K_SERVICE_FACTORY( MainMenuCommands_Predefined );

mainmenu_group_popup_factory g_mainmenu_group_help(
    smp::guid::mainmenu_group_help, mainmenu_groups::help, static_cast<t_uint32>( mainmenu_commands::sort_priority_dontcare ), SMP_NAME );

FB2K_SERVICE_FACTORY( MainMenuCommands_Help );

} // namespace
