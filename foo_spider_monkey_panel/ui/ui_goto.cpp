#include <stdafx.h>
#include "ui_goto.h"

#include <utils/string_helpers.h>

namespace smp::ui
{

CDialogGoto::CDialogGoto( HWND p_hedit )
    : m_hedit( p_hedit )
{
}

LRESULT CDialogGoto::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    const int cur_pos = SendMessage( m_hedit, SCI_GETCURRENTPOS, 0, 0 );
    const int cur_line = SendMessage( m_hedit, SCI_LINEFROMPOSITION, cur_pos, 0 );

    uSetWindowText( GetDlgItem( IDC_EDIT_LINENUMBER ), std::to_string( cur_line + 1 ).c_str() );

    return TRUE; // set focus to default control
}

LRESULT CDialogGoto::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    if ( wID == IDOK )
    {
        const auto text = smp::pfc_x::uGetWindowText<char8_t>( GetDlgItem( IDC_EDIT_LINENUMBER ) );
        const auto numRet = smp::string::GetNumber<unsigned>( static_cast<std::u8string_view>( text ) );
        if ( numRet )
        {
            SendMessage( m_hedit, SCI_GOTOLINE, *numRet, 0 );
        }
    }

    EndDialog( wID );
    return 0;
}

} // namespace smp::ui
