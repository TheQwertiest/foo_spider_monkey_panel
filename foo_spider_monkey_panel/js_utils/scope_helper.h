#pragma once

#include <js_utils/js_error_helper.h>

// TODO: separate js specific stuff

struct JSContext;

namespace mozjs::scope
{

template <typename T>
using unique_ptr = std::unique_ptr<T, void ( * )( T* )>;

template <class F>
class final_action
{// Ripped from gsl
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

class JsScope
{
public:
    JsScope( JSContext* cx, JS::HandleObject global, bool enableAutoReport = true )
        : ar_( cx )
        , ac_( cx, global )
        , are_( cx )
    {
        if ( !enableAutoReport )
        {
            are_.Disable();
        }
    }

    JsScope( const JsScope& ) = delete;
    JsScope& operator=( const JsScope& ) = delete;

    void DisableReport()
    {
        are_.Disable();
    }

private:
    JSAutoRequest ar_;
    JSAutoCompartment ac_;
    error::AutoJsReport are_;
};

} // namespace mozjs::scope
