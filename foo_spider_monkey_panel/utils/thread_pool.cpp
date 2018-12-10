#include <stdafx.h>
#include "thread_pool.h"

namespace smp
{

ThreadPool::ThreadPool()
    : maxThreadCount_( std::max<size_t>( std::thread::hardware_concurrency(), 1 ) )
{
    threads_.reserve( maxThreadCount_ );
}

ThreadPool::~ThreadPool()
{
    assert( tasks_.empty() );
}

ThreadPool& ThreadPool::GetInstance()
{
    static ThreadPool tp;
    return tp;
}

void ThreadPool::Finalize()
{
    assert( core_api::is_main_thread() );

    isExiting_ = true;
    hasTask_.notify_all();

    for ( const auto& thread : threads_ )
    {
        assert( thread );
        if ( thread->joinable() )
        {
            thread->join();
        }
    }
    threads_.clear();
}

void ThreadPool::AddThread()
{
    threads_.emplace_back( std::make_unique<std::thread>( [&] { ThreadProc(); } ) );
}

void ThreadPool::ThreadProc()
{
    ++idleThreadCount_;
    auto idleCounter = [&idleThreadsCount = idleThreadCount_]() {
        --idleThreadsCount;
    };

    while ( true )
    {
        if ( isExiting_ )
        {
            return;
        }

        std::unique_ptr<Task> task;
        {
            std::unique_lock sl( queueMutex_ );
            hasTask_.wait(
                sl,
                [&tasks = tasks_, &isExiting = isExiting_] {
                    return ( !tasks.empty() || isExiting );
                } );

            if ( isExiting_ )
            {
                return;
            }

            task.swap( tasks_.front() );
            tasks_.pop();
        }

        --idleThreadCount_;
        try
        {
            std::invoke( *task );
        }
        catch ( ... )
        {
        }
        ++idleThreadCount_;
    }
}

} // namespace smp