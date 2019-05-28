#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

#include <assert.h>

namespace smp
{

class ThreadPool
{
private:
    using Task = std::function<void()>;

public:
    ~ThreadPool();
    ThreadPool( const ThreadPool& ) = delete;
    ThreadPool& operator=( const ThreadPool& ) = delete;

    static ThreadPool& GetInstance();

    template <typename T>
    void AddTask( T&& task )
    {
        static_assert( std::is_invocable_v<T> );
        static_assert( std::is_move_constructible_v<T> || std::is_copy_constructible_v<T> );
        // This method can be easily modified to be thread-safe, but we don't need that currently
        assert( core_api::is_main_thread() );

        if ( isExiting_ )
        {
            return;
        }

        {
            std::scoped_lock sl( queueMutex_ );

            if constexpr ( !std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> )
            {
                auto taskLambda = [taskWrapper = std::make_shared<T>( std::forward<T>( task ) )] {
                    std::invoke( *taskWrapper );
                };
                tasks_.emplace( std::make_unique<Task>( taskLambda ) );
            }
            else
            {
                tasks_.emplace( std::make_unique<Task>( task ) );
            }            

            hasTask_.notify_one();
        }

        {
            std::mutex queueMutex_;
            if ( !tasks_.empty() && threads_.size() < maxThreadCount_ && !idleThreadCount_ )
            {
                AddThread();
            }
        }
    }

    void Finalize();

private:
    ThreadPool();

    void AddThread();
    void ThreadProc();

private:
    const size_t maxThreadCount_;

    std::vector<std::unique_ptr<std::thread>> threads_;
    std::atomic_uint32_t idleThreadCount_ = 0;
    std::atomic_bool isExiting_ = false;

    std::mutex queueMutex_;
    std::condition_variable hasTask_;
    std::queue<std::unique_ptr<Task>> tasks_;
};

} // namespace smp
