#include "stdafx.h"
#include "ui_slow_script.h"

namespace smp::ui
{

CDialogSlowScript::CDialogSlowScript( const pfc::string8_fast& scriptName, CDialogSlowScript::Data& data )
    : scriptName_( scriptName )
    , data_( data )
{
}

LRESULT CDialogSlowScript::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    uSetWindowText( GetDlgItem( IDC_SLOWSCRIPT_SCRIPT_NAME ), scriptName_ );
    return TRUE; // set focus to default control
}

LRESULT CDialogSlowScript::OnContinueScript( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    data_.stop = false;
    EndDialog( IDOK );
    return 0;
}

LRESULT CDialogSlowScript::OnStopScript( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    data_.stop = true;
    data_.askAgain = true;
    EndDialog( IDOK );
    return 0;
}

LRESULT CDialogSlowScript::OnDontAskClick( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    data_.askAgain = uButton_GetCheck( hWndCtl, wID );
    return 0;
}

LRESULT CDialogSlowScript::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    if ( wID == IDCANCEL )
    {
        data_.stop = false;
        data_.askAgain = true;
    }

    EndDialog( wID );
    return 0;
}

} // namespace smp::ui
