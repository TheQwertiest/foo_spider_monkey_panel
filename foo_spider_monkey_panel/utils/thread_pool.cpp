#include <stdafx.h>
#include "thread_pool.h"

#include <utils/thread_helpers.h>

namespace smp
{

ThreadPool::ThreadPool()
    : maxThreadCount_( std::max<size_t>( std::thread::hardware_concurrency(), 1 ) )
{
    threads_.reserve( maxThreadCount_ );
}

ThreadPool::~ThreadPool()
{
    assert( threads_.empty() );
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
    while ( !tasks_.empty() )
    { // Might be non-empty if thread was aborted
        tasks_.pop();
    }
}

void ThreadPool::AddThread()
{
    auto& ret = threads_.emplace_back( std::make_unique<std::thread>( [&] { ThreadProc(); } ) );
    smp::utils::SetThreadName( *ret, "SMP Worker" );
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