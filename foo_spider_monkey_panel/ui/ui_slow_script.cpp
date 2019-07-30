#include "stdafx.h"
#include "ui_slow_script.h"

namespace smp::ui
{

// TODO: add question icon like here - https://www.google.com/search?q=firefox+slow+script+warning&tbm=isch

CDialogSlowScript::CDialogSlowScript( const std::u8string& panelName, const std::u8string& scriptInfo, CDialogSlowScript::Data& data )
    : panelName_( panelName )
    , scriptInfo_( scriptInfo )
    , data_( data )
{
}

LRESULT CDialogSlowScript::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    (void)CenterWindow();

    const auto text = [& panelName = panelName_, &scriptInfo = scriptInfo_] {
        std::u8string tmp;
        if ( !panelName.empty() )
        {
            tmp += fmt::format( "Panel: {}", panelName );
        }
        if ( !scriptInfo.empty() )
        {
            if ( !tmp.empty() )
            {
                tmp += "\n";
            }
            tmp += fmt::format( "Script: {}", scriptInfo );
        }

        if ( tmp.empty() )
        {
            tmp = "<Unable to fetch panel info>";
        }

        return tmp;
    }();

    (void)uSetWindowText( GetDlgItem( IDC_SLOWSCRIPT_SCRIPT_NAME ), text.c_str() );

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
