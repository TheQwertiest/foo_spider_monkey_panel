#include <stdafx.h>

#include <panel/message_manager.h>
#include <panel/user_message.h>

#include <component_paths.h>

using namespace smp;

namespace
{

class MainMenuCommandsSmp : public mainmenu_commands
{
public:
    MainMenuCommandsSmp();

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

class MainMenuCommandsSmp_Help : public mainmenu_commands
{
public:
    MainMenuCommandsSmp_Help() = default;

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

MainMenuCommandsSmp::MainMenuCommandsSmp()
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

t_uint32 MainMenuCommandsSmp::get_command_count()
{
    return menuObjects_.size();
}
GUID MainMenuCommandsSmp::get_command( t_uint32 p_index )
{
    if ( p_index >= menuObjects_.size() )
    {
        uBugCheck();
        return pfc::guid_null;
    }

    return menuObjects_[p_index];
}
void MainMenuCommandsSmp::get_name( t_uint32 p_index, pfc::string_base& p_out )
{
    if ( p_index >= menuObjects_.size() )
    {
        uBugCheck();
    }

    p_out.reset();
    p_out << ( p_index + 1 );
}
bool MainMenuCommandsSmp::get_description( t_uint32 /* p_index */, pfc::string_base& p_out )
{
    p_out = "Invoke on_main_menu()";
    return true;
}
GUID MainMenuCommandsSmp::get_parent()
{
    return guid::mainmenu_group_predefined;
}
void MainMenuCommandsSmp::execute( t_uint32 p_index, service_ptr_t<service_base> )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( InternalAsyncMessage::main_menu_item ), p_index + 1 );
}
bool MainMenuCommandsSmp::get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags )
{
    get_name( p_index, p_out );
    p_flags = mainmenu_commands::flag_defaulthidden;
    return true;
}

t_uint32 MainMenuCommandsSmp_Help::get_command_count()
{
    return 1;
}
GUID MainMenuCommandsSmp_Help::get_command( t_uint32 p_index )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return pfc::guid_null;
    }

    return guid::mainmenu_node_help_docs;
}
void MainMenuCommandsSmp_Help::get_name( t_uint32 p_index, pfc::string_base& p_out )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return;
    }

    p_out.reset();
    p_out << "Spider Monkey Panel help";
}
bool MainMenuCommandsSmp_Help::get_description( t_uint32 p_index, pfc::string_base& p_out )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return false;
    }

    p_out = "View Spider Monkey Panel documentation files";
    return true;
}
GUID MainMenuCommandsSmp_Help::get_parent()
{
    return mainmenu_groups::help;
}
void MainMenuCommandsSmp_Help::execute( t_uint32 p_index, service_ptr_t<service_base> )
{
    if ( p_index != 0 )
    {
        uBugCheck();
        return;
    }
    ShellExecute( nullptr, L"open", path::JsDocsIndex().c_str(), nullptr, nullptr, SW_SHOW );
}
bool MainMenuCommandsSmp_Help::get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags )
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

FB2K_SERVICE_FACTORY( MainMenuCommandsSmp );

mainmenu_group_popup_factory g_mainmenu_group_help(
    smp::guid::mainmenu_group_help, mainmenu_groups::help, static_cast<t_uint32>( mainmenu_commands::sort_priority_dontcare ), SMP_NAME );

FB2K_SERVICE_FACTORY( MainMenuCommandsSmp_Help );

} // namespace
