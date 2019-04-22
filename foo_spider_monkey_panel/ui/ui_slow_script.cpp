#include "stdafx.h"
#include "ui_slow_script.h"

namespace smp::ui
{

// TODO: add question icon like here - https://www.google.com/search?q=firefox+slow+script+warning&tbm=isch

CDialogSlowScript::CDialogSlowScript( const pfc::string8_fast& panelName, const pfc::string8_fast& scriptInfo, CDialogSlowScript::Data& data )
    : panelName_( panelName )
    , scriptInfo_( scriptInfo )
    , data_( data )
{
}

LRESULT CDialogSlowScript::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    (void)CenterWindow();

    const auto text = [& panelName = panelName_, &scriptInfo = scriptInfo_] {
        pfc::string8_fast tmp;
        if ( !panelName.is_empty() )
        {
            tmp += "Panel: ";
            tmp += panelName;
        }
        if ( !scriptInfo.is_empty() )
        {
            if ( !tmp.is_empty() )
            {
                tmp += "\n";
            }
            tmp += "Script: ";
            tmp += scriptInfo;
        }

        if ( tmp.is_empty() )
        {
            tmp = "<Unable to fetch panel info>";
        }

        return tmp;
    }();

    (void)uSetWindowText( GetDlgItem( IDC_SLOWSCRIPT_SCRIPT_NAME ), text );

    return FALSE; // set focus to default control
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
