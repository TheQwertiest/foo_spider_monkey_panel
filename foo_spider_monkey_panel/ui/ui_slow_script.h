#pragma once

#include <resources/resource.h>

namespace smp::ui
{

class CDialogSlowScript : public CDialogImpl<CDialogSlowScript>
{
public:
    struct Data
    {
        bool askAgain = true;
        bool stop = false;
    };

    CDialogSlowScript( const qwr::u8string& panelName, const qwr::u8string& scriptInfo, CDialogSlowScript::Data& data );

    BEGIN_MSG_MAP( CDialogSlowScript )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_ID_HANDLER_EX( IDC_SLOWSCRIPT_CONTINUE, OnContinueScript )
        COMMAND_ID_HANDLER_EX( IDC_SLOWSCRIPT_STOP, OnStopScript )
        COMMAND_HANDLER_EX( IDC_SLOWSCRIPT_CHECK_DONTASK, BN_CLICKED, OnDontAskClick )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_SLOWSCRIPT
    };

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnContinueScript( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnStopScript( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnDontAskClick( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );

private:
    const qwr::u8string panelName_;
    const qwr::u8string scriptInfo_;
    CDialogSlowScript::Data& data_;
};

} // namespace smp::ui
