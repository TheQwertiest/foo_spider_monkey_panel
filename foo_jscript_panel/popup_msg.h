#pragma once

#include "delay_loader.h"

class popup_msg
{
public:
	struct action : public delay_loader_action
	{
		action(const char * p_msg, const char * p_title, popup_message::t_icon p_icon)
			: msg_(p_msg), title_(p_title), icon_(p_icon) {}

		pfc::string_simple msg_, title_;
		popup_message::t_icon icon_;

		virtual void execute()
		{
			::popup_message::g_show(msg_, title_, icon_);
		}
	};

	static inline void g_show(const char * p_msg, const char * p_title, popup_message::t_icon p_icon = popup_message::icon_information)
	{
		delay_loader::g_enqueue(new action(p_msg, p_title, p_icon));
	}
};
