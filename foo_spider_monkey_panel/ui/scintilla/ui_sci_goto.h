#pragma once

#include <resource.h>

namespace scintilla
{

// TODO: replace with modeless dialog
class CDialogGoto : public CDialogImpl<CDialogGoto>
{
public:
    CDialogGoto( HWND p_hedit );

    BEGIN_MSG_MAP( CDialogGoto )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_GOTO
    };

    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );

private:
    HWND m_hedit;
};

} // namespace scintilla
