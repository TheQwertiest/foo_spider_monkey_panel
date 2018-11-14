#include <stdafx.h>
#include "user_message.h"

#include <panel_manager.h>

using namespace smp;

namespace
{

mainmenu_group_popup_factory g_mainmenu_group(
    g_guid_smp_mainmenu_group_id, mainmenu_groups::file, static_cast<t_uint32>( mainmenu_commands::sort_priority_dontcare ), SMP_NAME );

class my_mainmenu_commands : public mainmenu_commands
{
public:
    enum
    {
        cmd_one = 0,
        cmd_two,
        cmd_three,
        cmd_four,
        cmd_five,
        cmd_six,
        cmd_seven,
        cmd_eight,
        cmd_nine,
        cmd_ten,
        cmd_total
    };
    t_uint32 get_command_count()
    {
        return cmd_total;
    }
    GUID get_command( t_uint32 p_index )
    {
        switch ( p_index )
        {
        case cmd_one:
            return g_guid_smp_menu_one;
        case cmd_two:
            return g_guid_smp_menu_two;
        case cmd_three:
            return g_guid_smp_menu_three;
        case cmd_four:
            return g_guid_smp_menu_four;
        case cmd_five:
            return g_guid_smp_menu_five;
        case cmd_six:
            return g_guid_smp_menu_six;
        case cmd_seven:
            return g_guid_smp_menu_seven;
        case cmd_eight:
            return g_guid_smp_menu_eight;
        case cmd_nine:
            return g_guid_smp_menu_nine;
        case cmd_ten:
            return g_guid_smp_menu_ten;
        default:
            uBugCheck();
        }
    }
    void get_name( t_uint32 p_index, pfc::string_base& p_out )
    {
        switch ( p_index )
        {
        case cmd_one:
            p_out = "1";
            break;
        case cmd_two:
            p_out = "2";
            break;
        case cmd_three:
            p_out = "3";
            break;
        case cmd_four:
            p_out = "4";
            break;
        case cmd_five:
            p_out = "5";
            break;
        case cmd_six:
            p_out = "6";
            break;
        case cmd_seven:
            p_out = "7";
            break;
        case cmd_eight:
            p_out = "8";
            break;
        case cmd_nine:
            p_out = "9";
            break;
        case cmd_ten:
            p_out = "10";
            break;
        default:
            uBugCheck();
        }
    }
    bool get_description( t_uint32 /* p_index */, pfc::string_base& p_out )
    {
        p_out = "Invoke on_main_menu()";
        return true;
    }
    GUID get_parent()
    {
        return g_guid_smp_mainmenu_group_id;
    }
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback )
    {
        panel::panel_manager::instance().post_msg_to_all( static_cast<UINT>( InternalAsyncMessage::main_menu_item ), p_index + 1 );
    }
    bool get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags )
    {
        get_name( p_index, p_out );
        p_flags = mainmenu_commands::flag_defaulthidden;
        return true;
    }
};

mainmenu_commands_factory_t<my_mainmenu_commands> g_my_mainmenu_commands_factory;

} // namespace
