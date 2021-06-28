#pragma once

#include <config/parsed_panel_config.h>
#include <resources/resource.h>
#include <ui/impl/ui_itab.h>

#include <qwr/ui_ddx.h>
#include <qwr/ui_option.h>

#include <optional>
#include <vector>

namespace smp::panel
{
class js_panel_window;
}

namespace smp::ui
{

class CDialogConf
    : public CDialogImpl<CDialogConf>
    , public CDialogResize<CDialogConf>
{
public:
    enum class Tab : uint8_t
    {
        script,
        properties,
        package,
        def = script,
    };

public:
    enum
    {
        IDD = IDD_DIALOG_CONF
    };

    BEGIN_DLGRESIZE_MAP( CDialogConf )
        DLGRESIZE_CONTROL( IDC_TAB_CONF, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_EDIT_PANEL_NAME, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_BUTTON_EDIT_PANEL_NAME, DLSZ_MOVE_X )
        DLGRESIZE_CONTROL( IDC_BUTTON_COMMIT_PANEL_NAME, DLSZ_MOVE_X )
        DLGRESIZE_CONTROL( ID_HELP, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CDialogConf )
        MSG_WM_INITDIALOG( OnInitDialog )
        MESSAGE_HANDLER( WM_SIZE, OnSize )
        MSG_WM_PARENTNOTIFY( OnParentNotify )
        COMMAND_ID_HANDLER_EX( IDOK, OnCloseCmd )
        COMMAND_ID_HANDLER_EX( IDCANCEL, OnCloseCmd )
        COMMAND_ID_HANDLER_EX( IDAPPLY, OnCloseCmd )
        COMMAND_HANDLER_EX( IDC_EDIT_PANEL_NAME, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_BUTTON_EDIT_PANEL_NAME, BN_CLICKED, OnStartEditPanelName )
        COMMAND_HANDLER_EX( IDC_BUTTON_COMMIT_PANEL_NAME, BN_CLICKED, OnCommitPanelName )
        COMMAND_ID_HANDLER_EX( ID_HELP, OnHelp )
        MESSAGE_HANDLER( WM_WINDOWPOSCHANGED, OnWindowPosChanged )
#pragma warning( push )
#pragma warning( disable : 26454 ) // Arithmetic overflow
        NOTIFY_HANDLER_EX( IDC_TAB_CONF, TCN_SELCHANGE, OnSelectionChanged )
#pragma warning( pop )
        CHAIN_MSG_MAP( CDialogResize<CDialogConf> )
    END_MSG_MAP()

    CDialogConf( smp::panel::js_panel_window* pParent, CDialogConf::Tab tabId = CDialogConf::Tab::def );

    bool IsCleanSlate() const;

    void OnDataChanged();
    void OnWholeScriptChange();
    bool HasChanged();

    void Apply( bool savePackageData = true );
    void Revert();

    void SwitchTab( CDialogConf::Tab tabId );

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    void OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    void OnParentNotify( UINT message, UINT nChildID, LPARAM lParam );
    LRESULT OnWindowPosChanged( UINT, WPARAM, LPARAM lp, BOOL& bHandled );
    LRESULT OnSelectionChanged( LPNMHDR pNmhdr );
    void OnStartEditPanelName( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnCommitPanelName( UINT uNotifyCode, int nID, CWindow wndCtl );
    LRESULT OnHelp( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    void DisablePanelNameControls();

    void DoFullDdxToUi();

    void OnDataChangedImpl( bool hasChanged = true );

    void InitializeLocalData();

    void InitializeTabData( CDialogConf::Tab tabId = CDialogConf::Tab::def );
    void ReinitializeTabData();
    void RefreshTabData();
    void InitializeTabControls();
    void ReinitializeTabControls();
    void CreateChildTab();
    void DestroyChildTab();
    size_t GetTabIdx( CDialogConf::Tab tabId ) const;
    void SetActiveTabIdx( CDialogConf::Tab tabId );

private:
    bool suppressDdxFromUi_ = true;

    panel::js_panel_window* pParent_ = nullptr;
    config::ParsedPanelSettings oldSettings_;
    config::ParsedPanelSettings localSettings_;

    config::PanelProperties oldProperties_;
    config::PanelProperties localProperties_;

    bool hasChanged_ = false;
    bool isCleanSlate_ = false;

    std::unique_ptr<qwr::ui::IUiDdx> panelNameDdx_;

    const CDialogConf::Tab startingTabId_;
    CTabCtrl cTabs_;
    CDialogImplBase* pcCurTab_ = nullptr;

    size_t activeTabIdx_ = 0;
    std::vector<std::unique_ptr<ITab>> tabs_;
};

} // namespace smp::ui
