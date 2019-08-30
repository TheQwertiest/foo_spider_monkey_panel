#include <stdafx.h>
#include "ui_name_value_edit.h"

namespace smp::ui
{

CNameValueEdit::CNameValueEdit( const char* p_name, const char* p_value )
    : m_name( p_name )
    , m_value( p_value )
{
}

LRESULT CNameValueEdit::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    uSendDlgItemMessageText( m_hWnd, IDC_EDIT_NAME, WM_SETTEXT, 0, m_name.c_str() );
    uSendDlgItemMessageText( m_hWnd, IDC_EDIT_VALUE, WM_SETTEXT, 0, m_value.c_str() );

    // Select all
    SendDlgItemMessage( IDC_EDIT_VALUE, EM_SETSEL, 0, -1 );
    ::SetFocus( GetDlgItem( IDC_EDIT_VALUE ) );

    return FALSE;
}

LRESULT CNameValueEdit::OnCommand( UINT codeNotify, int id, HWND hwndCtl )
{
    if ( id == IDOK || id == IDCANCEL )
    {
        if ( id == IDOK )
        {
            m_value = smp::pfc_x::uGetWindowText<char8_t>( GetDlgItem( IDC_EDIT_VALUE ) );
        }

        EndDialog( id );
    }

    return 0;
}

std::u8string CNameValueEdit::GetValue()
{
    return m_value;
}

} // namespace smp::ui
