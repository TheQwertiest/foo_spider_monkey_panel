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
	static void g_enqueue(std::unique_ptr<delay_loader_action> callback)
	{
		if (!callback)
			return;

		if (g_ready())
		{
			callback->execute();
		}
		else
		{
			callbacks_.emplace_back(std::move(callback));
		}
	}

	static void g_set_ready()
	{
		services_initialized_ = true;

		for (t_size i = 0; i < callbacks_.size(); ++i)
		{
			callbacks_[i]->execute();
            callbacks_[i].reset();
		}

		callbacks_.clear();
	}

	static bool g_ready()
	{
		return services_initialized_;
	}

private:
	static std::vector<std::unique_ptr<delay_loader_action>> callbacks_;
	static bool services_initialized_;
};

FOOGUIDDECL bool delay_loader::services_initialized_ = false;
FOOGUIDDECL std::vector<std::unique_ptr<delay_loader_action>> delay_loader::callbacks_;

class popup_msg
{
public:
	struct action : delay_loader_action
	{
		action(const char* p_msg, const char* p_title) 
            : msg_(p_msg)
            , title_(p_title)
		{
		}

        pfc::string_simple msg_;
        pfc::string_simple title_;

		virtual void execute() override
		{
			::popup_message::g_show(msg_, title_);
		}
	};

	static void g_show(const char* p_msg, const char* p_title)
	{
		delay_loader::g_enqueue(std::make_unique<action>(p_msg, p_title));
	}
};
