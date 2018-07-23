#pragma once

#include <tuple>

namespace mozjs::scope
{

template <typename T>
using unique_ptr = std::unique_ptr<T, void(*)(T*)>;

template <typename Fn, typename ... Args>
class auto_caller
{
public:
    auto_caller( Fn releaseFunc, Args... args )
        : releaseFunc_( releaseFunc )
    {
        args_ = std::make_tuple( args... );
    }

    ~auto_caller()
    {
        if ( !isReleased_ )
        {
            releaseFunc_( args_ );
        }
    }

    void cancel()
    {
        isReleased_ = true;
    }

private:
    bool isReleased_ = false;
    std::tuple<Args...> args_;
    Fn releaseFunc_;
};

}
