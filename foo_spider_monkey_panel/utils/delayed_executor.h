#pragma once

#include <memory>
#include <queue>
#include <functional>

namespace smp::utils
{

/// @brief This is used for executing tasks that need fully initialized fb2k to function properly.
///        For example, `popup_message::g_show`, which is invoked during panel creation
///        (panels are created before `initquit::on_init` is called).
class DelayedExecutor
{
private:
    using Task = std::function<void()>;

public:
    static DelayedExecutor& GetInstance();

    template <typename T>
    void AddTask( T&& task )
    {
        static_assert( std::is_invocable_v<T> );
        static_assert( std::is_move_constructible_v<T> || std::is_copy_constructible_v<T> );

        // This method can be easily modified to be thread-safe, but we don't need that currently
        assert( core_api::is_main_thread() );

        if ( canExecute_ )
        {
            std::invoke( task );
        }
        else
        {
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
        }
    }


	/// @details Should be invoked only from `initquit::on_init` 
    void EnableExecution();

private:
    DelayedExecutor() = default;

private:
    bool canExecute_ = false;
    std::queue<std::unique_ptr<Task>> tasks_;
};

} // namespace smp::utils
