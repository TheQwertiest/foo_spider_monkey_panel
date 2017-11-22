#include "stdafx.h"
#include "user_message.h"
#include "panel_manager.h"

static const GUID g_mainmenu_group_id = { 0x7b23ce27, 0x7d37, 0x4a1f,{ 0x80, 0x5b, 0xe5, 0x89, 0x19, 0x5b, 0xbd, 0xd0 } };

static mainmenu_group_popup_factory g_mainmenu_group(g_mainmenu_group_id, mainmenu_groups::file, mainmenu_commands::sort_priority_dontcare, JSP_NAME);

class my_mainmenu_commands : public mainmenu_commands {
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
	GUID get_command(t_uint32 p_index)
	{
		static const GUID guid_one = { 0xf56e5f2d, 0xf1a1, 0x4f54,{ 0x97, 0xf5, 0xe7, 0xc4, 0xbe, 0x47, 0x1a, 0xb3 } };
		static const GUID guid_two = { 0xc3bda2f5, 0xf011, 0x4f54,{ 0x99, 0xa, 0x77, 0xf9, 0xef, 0x27, 0xf1, 0xb } };
		static const GUID guid_three = { 0x9c610f78, 0x2eb7, 0x43b6,{ 0x89, 0x6d, 0x86, 0x9b, 0xd4, 0xec, 0xb9, 0xeb } };
		static const GUID guid_four = { 0x6123f3ee, 0xeb4f, 0x4170,{ 0x80, 0x49, 0x15, 0x18, 0xe6, 0xad, 0x8a, 0x62 } };
		static const GUID guid_five = { 0x59f48bd1, 0xa6a1, 0x4a8a,{ 0x93, 0x3c, 0x39, 0xcf, 0xef, 0x8, 0xee, 0x52 } };
		static const GUID guid_six = { 0x365377e0, 0x7a4b, 0x430f,{ 0x88, 0x3, 0xfd, 0xaf, 0x22, 0x60, 0xf6, 0xe1 } };
		static const GUID guid_seven = { 0x5e26ac8d, 0x38, 0x4743,{ 0x90, 0x72, 0x2c, 0x26, 0x56, 0x96, 0xcf, 0x14 } };
		static const GUID guid_eight = { 0x6b00b1c4, 0xa55, 0x46d8,{ 0x83, 0x2f, 0xfd, 0xd5, 0xd9, 0xde, 0x69, 0x43 } };
		static const GUID guid_nine = { 0xca840da4, 0xfc99, 0x44bc,{ 0x90, 0x71, 0xd2, 0xb0, 0x2b, 0x26, 0xd4, 0x35 } };
		static const GUID guid_ten = { 0xab05eee8, 0xbadc, 0x49ba,{ 0x80, 0x27, 0x84, 0x72, 0xa8, 0xbd, 0x49, 0xdb } };

		switch (p_index)
		{
		case cmd_one: return guid_one;
		case cmd_two: return guid_two;
		case cmd_three: return guid_three;
		case cmd_four: return guid_four;
		case cmd_five: return guid_five;
		case cmd_six: return guid_six;
		case cmd_seven: return guid_seven;
		case cmd_eight: return guid_eight;
		case cmd_nine: return guid_nine;
		case cmd_ten: return guid_ten;
		default: uBugCheck();
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base& p_out)
	{
		switch (p_index)
		{
		case cmd_one: p_out = "1"; break;
		case cmd_two: p_out = "2"; break;
		case cmd_three: p_out = "3"; break;
		case cmd_four: p_out = "4"; break;
		case cmd_five: p_out = "5"; break;
		case cmd_six: p_out = "6"; break;
		case cmd_seven: p_out = "7"; break;
		case cmd_eight: p_out = "8"; break;
		case cmd_nine: p_out = "9"; break;
		case cmd_ten: p_out = "10"; break;
		default: uBugCheck();
		}
	}
	bool get_description(t_uint32 p_index, pfc::string_base& p_out)
	{
		p_out = "Invoke on_main_menu()";
		return true;
	}
	GUID get_parent()
	{
		return g_mainmenu_group_id;
	}
	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
	{
		panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_MAIN_MENU, p_index + 1);
	}
	bool get_display(t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags)
	{
		get_name(p_index, p_out);
		p_flags = mainmenu_commands::flag_defaulthidden;
		return true;
	}
};

static mainmenu_commands_factory_t<my_mainmenu_commands> g_my_mainmenu_commands_factory;
