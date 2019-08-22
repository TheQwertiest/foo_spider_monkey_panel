#pragma once

#include "resource.h"

namespace smp::ui
{

class CDialogPref
    : public CDialogImpl<CDialogPref>
    , public CWinDataExchange<CDialogPref>
    , public preferences_page_instance
{
public:
    CDialogPref( preferences_page_callback::ptr callback );

    BEGIN_DDX_MAP( CDialogPref )
		DDX_CONTROL_HANDLE( IDC_LIST_EDITOR_PROP, m_props )
    END_DDX_MAP()

    BEGIN_MSG_MAP( CDialogPref )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDC_BUTTON_EXPORT, BN_CLICKED, OnButtonExportBnClicked )
		COMMAND_HANDLER_EX( IDC_BUTTON_IMPORT, BN_CLICKED, OnButtonImportBnClicked )
		NOTIFY_HANDLER_EX( IDC_LIST_EDITOR_PROP, NM_DBLCLK, OnPropNMDblClk )
    END_MSG_MAP()

    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );

    enum
    {
        IDD = IDD_DIALOG_PREFERENCE
    };

    // preferences_page_instance
    t_uint32 get_state() override;
    HWND get_wnd() override;
    void apply() override;
    void reset() override;

    LRESULT OnPropNMDblClk( LPNMHDR pnmh );

    void LoadProps( bool reset = false );
    void OnButtonExportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    void OnButtonImportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    void OnChanged();
    void OnEditChange( WORD, WORD, HWND );
    std::u8string uGetItemText( int nItem, int nSubItem );

private:
    CListViewCtrl m_props;
    preferences_page_callback::ptr m_callback;
};

} // namespace smp::ui
