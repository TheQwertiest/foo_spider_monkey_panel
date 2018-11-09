// Copied from https://github.com/reupen/ui_helpers
// Copyright (c) Reupen Shah 2003-2017
// All rights reserved.

#pragma once

template <typename t_handle, class t_release>
class handle_container_t
{
    typedef handle_container_t<t_handle, t_release> t_self;

public:
    void release()
    {
        if ( is_valid() )
        {
            t_release::release( m_handle );
            t_release::set_invalid( m_handle );
        }
    }
    t_handle set( t_handle value )
    {
        release();
        return ( m_handle = value );
    }
    t_handle get() const
    {
        return m_handle;
    }
    operator t_handle() const
    {
        return get();
    }
    t_handle operator=( t_handle value )
    {
        return set( value );
    }
    t_self& operator=( const t_self& value ) = delete;
    t_self& operator=( t_self&& value )
    {
        set( value.detach() );
        return *this;
    }
    t_handle detach()
    {
        t_handle ret = m_handle;
        t_release::set_invalid( m_handle );
        return ret;
    }
    bool is_valid() const
    {
        return t_release::is_valid( m_handle );
    }

    handle_container_t()
    {
        t_release::set_invalid( m_handle );
    };
    handle_container_t( t_handle value )
        : m_handle( value ){};
    handle_container_t( t_self&& value )
    {
        set( value.detach() );
    }
    handle_container_t( const t_self& value ) = delete;

    ~handle_container_t()
    {
        release();
    }

private:
    t_handle m_handle;
};

template <typename t_gdi_type>
class gdi_object_t
{
public:
    class gdi_release_t
    {
    public:
        template <typename t_gdi_type>
        static void release( t_gdi_type handle )
        {
            DeleteObject( (t_gdi_type)handle );
        };
        template <typename t_gdi_type>
        static bool is_valid( t_gdi_type handle )
        {
            return handle != nullptr;
        };
        template <typename t_gdi_type>
        static void set_invalid( t_gdi_type& handle )
        {
            handle = nullptr;
        };
    };
    typedef handle_container_t<t_gdi_type, gdi_release_t> ptr_t;
};

class icon_release_t
{
public:
    static void release( HICON handle )
    {
        DestroyIcon( handle );
    };
    static bool is_valid( HICON handle )
    {
        return handle != nullptr;
    };
    static void set_invalid( HICON& handle )
    {
        handle = nullptr;
    };
};
using icon_ptr = handle_container_t<HICON, icon_release_t>;

namespace win32
{
class __handle_release_t
{
public:
    static void release( HANDLE handle )
    {
        CloseHandle( handle );
    };
    static bool is_valid( HANDLE handle )
    {
        return handle != INVALID_HANDLE_VALUE;
    };
    static void set_invalid( HANDLE& handle )
    {
        handle = INVALID_HANDLE_VALUE;
    };
};
typedef handle_container_t<HANDLE, __handle_release_t> handle_ptr_t;

} // namespace win32
