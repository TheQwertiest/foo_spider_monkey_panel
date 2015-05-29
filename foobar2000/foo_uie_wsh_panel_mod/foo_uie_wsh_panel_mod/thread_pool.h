#pragma once

class simple_thread_task
{
public:
	virtual void run() { PFC_ASSERT(!"Should not go here"); }
};

// Rip from pfc
//! IMPORTANT: all classes derived from thread must call waitTillDone() in their destructor, to avoid object destruction during a virtual function call!
class simple_thread 
{
public:
	PFC_DECLARE_EXCEPTION(exception_creation, pfc::exception, "Could not create thread");

	simple_thread() : m_thread(INVALID_HANDLE_VALUE) {}
	~simple_thread() {PFC_ASSERT(!isActive()); waitTillDone();}

	void start();
	bool isActive() const;
	void waitTillDone();

protected:
	virtual void threadProc() {PFC_ASSERT(!"Stub thread entry - should not get here");}

private:
	void close();
	static unsigned CALLBACK g_entry(void* p_instance);
	unsigned entry();

	HANDLE m_thread;

	PFC_CLASS_NOT_COPYABLE_EX(simple_thread)
};

class simple_thread_worker : public simple_thread
{
public:
	simple_thread_worker() {}
	virtual ~simple_thread_worker() { waitTillDone(); }
	virtual void threadProc();

private:
	PFC_CLASS_NOT_COPYABLE_EX(simple_thread_worker)
};

class simple_thread_pool
{
public:
	static inline simple_thread_pool & instance()
	{
		return instance_;
	}

	inline simple_thread_pool() : num_workers_(0)
	{
		empty_worker_ = CreateEvent(NULL, TRUE, TRUE, NULL);
		exiting_ = CreateEvent(NULL, TRUE, FALSE, NULL);
		have_task_ = CreateEvent(NULL, TRUE, FALSE, NULL);

		pfc::dynamic_assert(empty_worker_ != INVALID_HANDLE_VALUE);
		pfc::dynamic_assert(exiting_ != INVALID_HANDLE_VALUE);
		pfc::dynamic_assert(have_task_ != INVALID_HANDLE_VALUE);
	}

	inline ~simple_thread_pool()
	{
		CloseHandle(empty_worker_);
		CloseHandle(exiting_);
		CloseHandle(have_task_);
	}

	bool enqueue(simple_thread_task * task);
	bool is_queue_empty();
	void track(simple_thread_task * task);
	void untrack(simple_thread_task * task);
	void untrack_all();
	// Should always called from the main thread
	void join();
	simple_thread_task * acquire_task();

private:
	void add_worker_(simple_thread_worker * worker);
	void remove_worker_(simple_thread_worker * worker);

	typedef pfc::chain_list_v2_t<simple_thread_task *> t_task_list;
	t_task_list task_list_;
	critical_section cs_;
	volatile LONG num_workers_;
	HANDLE empty_worker_;
	HANDLE exiting_;
	HANDLE have_task_;

	static simple_thread_pool instance_;

	friend class simple_thread_worker;

	PFC_CLASS_NOT_COPYABLE_EX(simple_thread_pool)
};
