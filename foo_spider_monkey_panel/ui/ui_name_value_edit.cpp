#include <stdafx.h>

#include "ui_name_value_edit.h"

#include <qwr/pfc_helpers_ui.h>

namespace smp::ui
{

CNameValueEdit::CNameValueEdit( const char* name, const char* value, const char* caption )
    : name_( name )
    , value_( value )
    , caption_( caption )
{
}

LRESULT CNameValueEdit::OnInitDialog( HWND, LPARAM )
{
    uSetWindowText( m_hWnd, caption_.c_str() );
    uSendDlgItemMessageText( m_hWnd, IDC_EDIT_NAME, WM_SETTEXT, 0, name_.c_str() );
    uSendDlgItemMessageText( m_hWnd, IDC_EDIT_VALUE, WM_SETTEXT, 0, value_.c_str() );

    // Select all
    SendDlgItemMessage( IDC_EDIT_VALUE, EM_SETSEL, 0, -1 );
    ::SetFocus( GetDlgItem( IDC_EDIT_VALUE ) );

    return FALSE;
}

LRESULT CNameValueEdit::OnCommand( UINT, int id, HWND )
{
    switch ( id )
    {
    case IDOK:
    {
        value_ = qwr::pfc_x::uGetWindowText<char>( GetDlgItem( IDC_EDIT_VALUE ) );
        EndDialog( id );
        break;
    }
    case IDCANCEL:
    {
        EndDialog( id );
        break;
    }
    default:
        break;
    }

    return 0;
}

qwr::u8string CNameValueEdit::GetValue()
{
    return value_;
}

} // namespace smp::ui
