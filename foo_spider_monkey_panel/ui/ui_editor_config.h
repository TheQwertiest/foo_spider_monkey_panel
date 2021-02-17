#pragma once

#include <resources/resource.h>

namespace smp::ui
{

class CDialogEditorConfig
    : public CDialogImpl<CDialogEditorConfig>
    , public CWinDataExchange<CDialogEditorConfig>
{
public:
    enum
    {
        IDD = IDD_DIALOG_EDITOR_CONFIG
    };

    BEGIN_DDX_MAP( CDialogEditorConfig )
        DDX_CONTROL_HANDLE( IDC_LIST_EDITOR_PROP, propertiesListView_ )
    END_DDX_MAP()

    BEGIN_MSG_MAP( CDialogEditorConfig )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_ID_HANDLER_EX( IDOK, OnCloseCmd )
        COMMAND_ID_HANDLER_EX( IDCANCEL, OnCloseCmd )
        COMMAND_HANDLER_EX( IDC_BUTTON_RESET, BN_CLICKED, OnButtonReset )
        COMMAND_HANDLER_EX( IDC_BUTTON_EXPORT, BN_CLICKED, OnButtonExportBnClicked )
        COMMAND_HANDLER_EX( IDC_BUTTON_IMPORT, BN_CLICKED, OnButtonImportBnClicked )
#pragma warning( push )
#pragma warning( disable : 26454 ) // Arithmetic overflow
        NOTIFY_HANDLER_EX( IDC_LIST_EDITOR_PROP, NM_DBLCLK, OnPropNMDblClk )
#pragma warning( pop )
    END_MSG_MAP()

    CDialogEditorConfig();

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    void OnButtonReset( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    void OnButtonExportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    void OnButtonImportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnPropNMDblClk( LPNMHDR pnmh );

    void LoadProps( bool reset = false );
    qwr::u8string GetItemTextStr( int nItem, int nSubItem );

private:
    CListViewCtrl propertiesListView_;
};

} // namespace smp::ui
