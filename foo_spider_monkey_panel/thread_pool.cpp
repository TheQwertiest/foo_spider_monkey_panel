#include "stdafx.h"
#include "thread_pool.h"

class simple_thread_worker_remover : public main_thread_callback
{
public:
    simple_thread_worker_remover( simple_thread_worker* worker ) : worker_( worker )
    {
    }

    virtual void callback_run()
    {
        delete worker_;
    }

private:
    simple_thread_worker * worker_;
};

simple_thread_pool simple_thread_pool::instance_;

simple_thread::simple_thread() 
    : m_thread( INVALID_HANDLE_VALUE )
{
}

simple_thread::~simple_thread()
{
    waitTillDone();
}

void simple_thread::start()
{
	close();
	HANDLE thread;
	thread = (HANDLE)_beginthreadex(NULL, 0, g_entry, reinterpret_cast<void*>(this), 0, NULL);
	if (thread == NULL) throw exception_creation();
	m_thread = thread;
}

bool simple_thread::isActive() const
{
	return m_thread != INVALID_HANDLE_VALUE;
}

void simple_thread::waitTillDone()
{
	close();
}

void simple_thread::close()
{
	if (isActive())
	{
		WaitForSingleObject(m_thread, INFINITE);
		CloseHandle(m_thread);
		m_thread = INVALID_HANDLE_VALUE;
	}
}

unsigned simple_thread::entry()
{
	try
	{
		threadProc();
	}
	catch (...)
	{
	}
	return 0;
}

unsigned CALLBACK simple_thread::g_entry(void* p_instance)
{
	return reinterpret_cast<simple_thread*>(p_instance)->entry();
}

void simple_thread_worker::threadProc()
{
	pfc::tickcount_t last_tick = pfc::getTickCount();

	while (WaitForSingleObject(simple_thread_pool::instance().exiting_, 0) == WAIT_TIMEOUT)
	{
		if (WaitForSingleObject(simple_thread_pool::instance().have_task_, 1000) == WAIT_OBJECT_0)
		{
			std::unique_ptr<simple_thread_task> task = simple_thread_pool::instance().acquire_task();
			if (task)
			{
				task->run();
				last_tick = pfc::getTickCount();
				continue;
			}
		}

		if (pfc::getTickCount() - last_tick >= 10000)
		{
			insync(simple_thread_pool::instance().cs_);

			if (simple_thread_pool::instance().is_queue_empty())
			{
				simple_thread_pool::instance().remove_worker_(this);
				return;
			}
		}
	}

	simple_thread_pool::instance().remove_worker_(this);
}


simple_thread_pool& simple_thread_pool::instance()
{
    return instance_;
}

simple_thread_pool::simple_thread_pool() 
    : num_workers_( 0 )
{
    empty_worker_ = CreateEvent( NULL, TRUE, TRUE, NULL );
    exiting_ = CreateEvent( NULL, TRUE, FALSE, NULL );
    have_task_ = CreateEvent( NULL, TRUE, FALSE, NULL );

    pfc::dynamic_assert( empty_worker_ != INVALID_HANDLE_VALUE );
    pfc::dynamic_assert( exiting_ != INVALID_HANDLE_VALUE );
    pfc::dynamic_assert( have_task_ != INVALID_HANDLE_VALUE );
}

simple_thread_pool::~simple_thread_pool()
{
    CloseHandle( empty_worker_ );
    CloseHandle( exiting_ );
    CloseHandle( have_task_ );
}

bool simple_thread_pool::enqueue(std::unique_ptr<simple_thread_task> task)
{
	if (WaitForSingleObject(exiting_, 0) == WAIT_OBJECT_0)
		return false;

	insync(cs_);

	int max_count = pfc::getOptimalWorkerThreadCount();
    track( std::move(task) );

	if (num_workers_ < max_count)
	{
		simple_thread_worker* worker = new simple_thread_worker;
		add_worker_(worker);
		worker->start();
	}

	return true;
}

bool simple_thread_pool::is_queue_empty()
{
	insync(cs_);

	return task_list_.empty();
}

void simple_thread_pool::track( std::unique_ptr<simple_thread_task> task )
{
	insync(cs_);

	bool empty = is_queue_empty();
	task_list_.emplace_back(std::move(task));

    if ( empty )
    {
        SetEvent( have_task_ );
    }
}

void simple_thread_pool::untrack_all()
{
	insync(cs_);

    task_list_.clear();

	ResetEvent(have_task_);
}

void simple_thread_pool::join()
{
	core_api::ensure_main_thread();

	SetEvent(exiting_);

	// Because the tasks may use blocking SendMessage() function, it should be avoid using
	// infinite wait here, or both main thread and worker thread will block, and it's also
	// important to dispatch windows messages here.
	while (WaitForSingleObject(empty_worker_, 0) == WAIT_TIMEOUT)
	{
		MSG msg;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	untrack_all();
}

std::unique_ptr<simple_thread_task> simple_thread_pool::acquire_task()
{
	insync(cs_);

    std::unique_ptr<simple_thread_task> task;
    if ( !task_list_.empty() )
    {
        task = std::move( task_list_.front() );
        task_list_.pop_front();
    }

    if ( is_queue_empty() )
    {
        ResetEvent( have_task_ );
    }

	return task;
}

void simple_thread_pool::add_worker_(simple_thread_worker* worker)
{
	insync(cs_);
	InterlockedIncrement(&num_workers_);
	ResetEvent(empty_worker_);
}

void simple_thread_pool::remove_worker_(simple_thread_worker* worker)
{
	InterlockedDecrement(&num_workers_);
	insync(cs_);

    if ( !num_workers_ )
    {
        SetEvent( empty_worker_ );
    }

	main_thread_callback_manager::get()->add_callback(new service_impl_t<simple_thread_worker_remover>(worker));
}
