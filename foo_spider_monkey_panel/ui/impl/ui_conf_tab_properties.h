#pragma once

#include <config/panel_config.h>
#include <property_list/PropertyList.h>
#include <resources/resource.h>
#include <ui/impl/ui_itab.h>

namespace smp::panel
{
class js_panel_window;
}

namespace smp::ui
{

class CDialogConf;

class CConfigTabProperties
    : public CDialogImpl<CConfigTabProperties>
    , public CDialogResize<CConfigTabProperties>
    , public ITab
{
public:
    enum
    {
        IDD = IDD_DIALOG_CONF_TAB_PROPERTIES
    };

    BEGIN_DLGRESIZE_MAP( CConfigTabProperties )
        DLGRESIZE_CONTROL( IDC_LIST_PROPERTIES, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_DEL, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_CLEARALL, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_IMPORT, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_EXPORT, DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CConfigTabProperties )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_HANDLER_EX( IDC_CLEARALL, BN_CLICKED, OnClearAllBnClicked )
        COMMAND_HANDLER_EX( IDC_DEL, BN_CLICKED, OnDelBnClicked )
        COMMAND_HANDLER_EX( IDC_IMPORT, BN_CLICKED, OnImportBnClicked )
        COMMAND_HANDLER_EX( IDC_EXPORT, BN_CLICKED, OnExportBnClicked )
#pragma warning( push )
#pragma warning( disable : 26454 ) // Arithmetic overflow
        NOTIFY_CODE_HANDLER_EX( PIN_ITEMCHANGED, OnPinItemChanged )
        NOTIFY_CODE_HANDLER_EX( PIN_SELCHANGED, OnSelChanged )
#pragma warning( pop )
        CHAIN_MSG_MAP( CDialogResize<CConfigTabProperties> )
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    CConfigTabProperties( CDialogConf& parent, config::PanelProperties& properties );

    // > IUiTab
    HWND CreateTab( HWND hParent ) override;
    [[nodiscard]] CDialogImplBase& Dialog() override;
    [[nodiscard]] const wchar_t* Name() const override;
    [[nodiscard]] bool HasChanged() override;
    void Apply() override;
    void Revert() override;
    void Refresh() override;
    // < IUiTab

private:
    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnClearAllBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnDelBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnExportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnImportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnPinItemChanged( LPNMHDR pnmh );
    LRESULT OnSelChanged( LPNMHDR pnmh );

    void UpdateUiFromData();
    void UpdateUiDelButton();

private:
    CPropertyListCtrl propertyListCtrl_;
    CDialogConf& parent_;
    config::PanelProperties& properties_;
};

} // namespace smp::ui
