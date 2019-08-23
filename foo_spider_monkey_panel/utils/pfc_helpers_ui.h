#pragma once

#include <string>

namespace smp::pfc_x
{

template <typename T>
T uGetWindowText( HWND wnd )
{
    auto size = ::GetWindowTextLength( wnd );
    if ( !size )
    {
        return T{};
    }

    std::wstring text;
    text.resize( size + 1 );
    (void)::GetWindowText( wnd, text.data(), text.size() );
    text.resize( wcslen( text.c_str() ) );

    if constexpr ( std::is_same_v<typename T::value_type, wchar_t> )
    {
        return text;
    }
    else
    {
        return smp::unicode::ToU8( text );
    }
}

template <typename T>
T uGetDlgItemText( HWND wnd, UINT id )
{
    const auto hControl = ::GetDlgItem( wnd, id );
    if ( !hControl )
    {
        return T{};
    }

    return smp::pfc_x::uGetWindowText<T>( hControl );
}

} // namespace smp::pfc_x