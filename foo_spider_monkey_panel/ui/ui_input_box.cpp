#include <stdafx.h>

#include "ui_input_box.h"

#include <qwr/pfc_helpers_ui.h>

namespace smp::ui
{

CInputBox::CInputBox( const char* p_prompt, const char* p_caption, const char* p_value )
    : m_prompt( p_prompt )
    , m_caption( p_caption )
    , m_value( p_value )
{
}

LRESULT CInputBox::OnInitDialog( HWND, LPARAM )
{
    uSetWindowText( m_hWnd, m_caption.c_str() );
    uSendDlgItemMessageText( m_hWnd, IDC_INPUT_PROMPT, WM_SETTEXT, 0, m_prompt.c_str() );
    uSendDlgItemMessageText( m_hWnd, IDC_INPUT_VALUE, WM_SETTEXT, 0, m_value.c_str() );

    // Select all
    SendDlgItemMessage( IDC_INPUT_VALUE, EM_SETSEL, 0, -1 );
    ::SetFocus( GetDlgItem( IDC_INPUT_VALUE ) );
    CenterWindow();

    return FALSE;
}

LRESULT CInputBox::OnCommand( UINT, int id, HWND )
{
    if ( id == IDOK || id == IDCANCEL )
    {
        if ( id == IDOK )
        {
            m_value = qwr::pfc_x::uGetWindowText<char8_t>( GetDlgItem( IDC_INPUT_VALUE ) );
        }

        EndDialog( id );
    }

    return 0;
}

std::u8string CInputBox::GetValue()
{
    return m_value;
}

} // namespace smp::ui
