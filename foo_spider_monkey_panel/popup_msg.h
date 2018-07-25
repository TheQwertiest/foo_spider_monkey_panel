#pragma once

/* WARNING: Non thread safety */
class NOVTABLE delay_loader_action
{
public:
	virtual void execute() = 0;
};

class delay_loader
{
public:
	static void g_enqueue(delay_loader_action* callback)
	{
		if (!callback)
			return;

		if (g_ready())
		{
			callback->execute();
		}
		else
		{
			callbacks_.add_item(callback);
		}
	}

	static void g_set_ready()
	{
		services_initialized_ = true;

		for (t_size i = 0; i < callbacks_.get_count(); ++i)
		{
			callbacks_[i]->execute();
			delete callbacks_[i];
		}

		callbacks_.remove_all();
	}

	static bool g_ready()
	{
		return services_initialized_;
	}

private:
	static pfc::list_t<delay_loader_action *> callbacks_;
	static bool services_initialized_;
};

FOOGUIDDECL bool delay_loader::services_initialized_ = false;
FOOGUIDDECL pfc::list_t<delay_loader_action *> delay_loader::callbacks_;

class popup_msg
{
public:
	struct action : delay_loader_action
	{
		action(const char* p_msg, const char* p_title) : msg_(p_msg), title_(p_title)
		{
		}

		pfc::string_simple msg_, title_;

		virtual void execute() override
		{
			::popup_message::g_show(msg_, title_);
		}
	};

	static void g_show(const char* p_msg, const char* p_title)
	{
		delay_loader::g_enqueue(new action(p_msg, p_title));
	}
};
