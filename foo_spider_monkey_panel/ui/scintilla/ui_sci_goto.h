#pragma once

#include <resources/resource.h>

namespace smp::ui::sci
{

class CScriptEditorCtrl;

class CDialogGoto : public CDialogImpl<CDialogGoto>
{
public:
    BEGIN_MSG_MAP( CDialogGoto )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroy )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_GOTO
    };

    CDialogGoto( HWND hParent, int curLineNumber );

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDestroy();
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    // CDialogImpl

    void OnFinalMessage( _In_ HWND /*hWnd*/ ) override;

    static void GetMsgProc( int code, WPARAM wParam, LPARAM lParam, HWND hParent );

private:
    HWND hParent_;
    const int curLineNumber_;

    uint32_t hookId_ = 0;
};

class CScintillaGotoImpl
{
public:
    BEGIN_MSG_MAP( CScintillaGotoImpl )
        MESSAGE_HANDLER( GetGotoMsg(), OnGotoCmd )
    END_MSG_MAP()

    CScintillaGotoImpl( CScriptEditorCtrl& sciEdit );

    static UINT GetGotoMsg();

    void ShowGoTo();

private:
    LRESULT OnGotoCmd( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ );

private:
    CScriptEditorCtrl& sciEdit_;

    CDialogGoto* pGoto_ = nullptr;
};

} // namespace smp::ui::sci
