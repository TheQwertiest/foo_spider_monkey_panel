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
	static inline void g_enqueue(delay_loader_action * callback)
	{
		if (!callback)
			return;

		if (!g_ready())
			callbacks_.add_item(callback);
		else
			callback->execute();
	}

	static inline void g_set_ready()
	{
		services_initialized_ = true;

		for (t_size i = 0; i < callbacks_.get_count(); ++i)
		{
			callbacks_[i]->execute();
			delete callbacks_[i];
		}

		callbacks_.remove_all();
	}

	static inline bool g_ready()
	{
		return services_initialized_;
	}

private:
	static pfc::list_t<delay_loader_action *> callbacks_;
	static bool services_initialized_;
};

FOOGUIDDECL bool delay_loader::services_initialized_ = false;
FOOGUIDDECL pfc::list_t<delay_loader_action *> delay_loader::callbacks_;
