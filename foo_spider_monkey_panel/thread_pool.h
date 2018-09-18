#pragma once

class simple_thread_task
{
public:
    virtual ~simple_thread_task() = default;
    virtual void run() = 0;
};

// Rip from pfc
class simple_thread
{
public:
    PFC_DECLARE_EXCEPTION( exception_creation, pfc::exception, "Could not create thread" );

    simple_thread();
    virtual ~simple_thread();

    void start();
    bool isActive() const;
    void waitTillDone();

protected:
    virtual void threadProc() = 0;

private:
    void close();
    static unsigned CALLBACK g_entry( void* p_instance );
    unsigned entry();

    HANDLE m_thread;

    PFC_CLASS_NOT_COPYABLE_EX( simple_thread )
};

class simple_thread_worker : public simple_thread
{
public:
    simple_thread_worker() = default;
    virtual ~simple_thread_worker() = default;
    virtual void threadProc() override;

private:
    PFC_CLASS_NOT_COPYABLE_EX( simple_thread_worker )
};

class simple_thread_pool
{
public:
    static simple_thread_pool& instance();

    simple_thread_pool();
    ~simple_thread_pool();

    bool enqueue( std::unique_ptr<simple_thread_task> task );
    bool is_queue_empty();
    void track( std::unique_ptr<simple_thread_task> task );
    void untrack_all();
    // Should be always called from the main thread
    void join();
    std::unique_ptr<simple_thread_task> acquire_task();

private:
    void add_worker_( simple_thread_worker* worker );
    void remove_worker_( simple_thread_worker* worker );

    using t_task_list = std::list<std::unique_ptr<simple_thread_task>>;
    t_task_list task_list_;
    critical_section cs_;
    volatile LONG num_workers_;
    HANDLE empty_worker_;
    HANDLE exiting_;
    HANDLE have_task_;

    static simple_thread_pool instance_;

    friend class simple_thread_worker;

    PFC_CLASS_NOT_COPYABLE_EX( simple_thread_pool )
};
