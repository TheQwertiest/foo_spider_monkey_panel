#pragma once

#include <tuple>

namespace mozjs::scope
{

template <typename T>
using unique_ptr = std::unique_ptr<T, void(*)(T*)>;

template <class F>
class final_action
{
public:
    explicit final_action( F f ) noexcept 
        : f_( std::move( f ) )
    {
    }

    final_action( final_action&& other ) noexcept 
        : f_( std::move( other.f_ ) )
        , invoke_( other.invoke_ )
    {
        other.invoke_ = false;
    }

    final_action( const final_action& ) = delete;
    final_action& operator=( const final_action& ) = delete;
    final_action& operator=( final_action&& ) = delete;

    ~final_action() noexcept
    {
        if ( invoke_ )
        {
            f_();
        }
    }

    void cancel()
    {
        invoke_ = false;
    }

private:
    F f_;
    bool invoke_{ true };
};

template <class F>
final_action<F> finally( const F& f ) noexcept
{
    return final_action<F>( f );
}

template <class F>
final_action<F> finally( F&& f ) noexcept
{
    return final_action<F>( std::forward<F>( f ));
}

}
