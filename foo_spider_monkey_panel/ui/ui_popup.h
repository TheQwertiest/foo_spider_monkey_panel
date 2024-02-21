#pragma once

#include <resources/resource.h>

namespace smp::ui
{

class CPopup
    : public CDialogImpl<CPopup>
    , public CDialogResize<CPopup>
{
public:
    CPopup( qwr::u8string_view message, qwr::u8string_view caption );

    BEGIN_DLGRESIZE_MAP( CPopup )
        DLGRESIZE_CONTROL( IDC_LTEXT_MESSAGE, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CPopup )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_COMMAND( OnCommand )
        CHAIN_MSG_MAP( CDialogResize<CPopup> )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_POPUP
    };

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnCommand( UINT codeNotify, int id, HWND hwndCtl );

private:
    void AdjustToFit();

private:
    const qwr::u8string message_;
    const qwr::u8string caption_;
};

} // namespace smp::ui
