#pragma once

#include <com_objects/file_drop_target.h>
#include <config/parsed_panel_config.h>
#include <panel/user_message.h>
#include <resources/resource.h>
#include <ui/impl/ui_itab.h>

#include <qwr/ui_ddx.h>

#include <array>
#include <filesystem>

namespace smp::ui
{

class CDialogConf;

class CConfigTabPackage
    : public CDialogImpl<CConfigTabPackage>
    , public CDialogResize<CConfigTabPackage>
    , public ITab
{
public:
    enum
    {
        IDD = IDD_DIALOG_CONF_TAB_PACKAGE
    };

    BEGIN_DLGRESIZE_MAP( CConfigTabPackage )
        DLGRESIZE_CONTROL( IDC_LIST_PACKAGE_FILES, DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_GROUP_PKG_FILES, DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_BUTTON_NEW_SCRIPT, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_BUTTON_ADD_FILE, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_BUTTON_REMOVE_FILE, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_BUTTON_RENAME_FILE, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_BUTTON_EDIT_SCRIPT, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_BUTTON_OPEN_FOLDER, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_GROUP_PKG_INFO, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_EDIT_PACKAGE_NAME, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_EDIT_PACKAGE_VERSION, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_EDIT_PACKAGE_AUTHOR, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_EDIT_PACKAGE_DESCRIPTION, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_GROUP_PANEL_BEHAVIOUR, DLSZ_SIZE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_CHECK_SHOULD_GRAB_FOCUS, DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDC_CHECK_ENABLE_DRAG_N_DROP, DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CConfigTabPackage )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroy )
        MESSAGE_HANDLER_EX( static_cast<int>( InternalSyncMessage::ui_script_editor_saved ), OnScriptSaved );
        COMMAND_HANDLER_EX( IDC_EDIT_PACKAGE_NAME, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_PACKAGE_VERSION, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_PACKAGE_AUTHOR, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_PACKAGE_DESCRIPTION, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_SHOULD_GRAB_FOCUS, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_ENABLE_DRAG_N_DROP, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_LIST_PACKAGE_FILES, LBN_SELCHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_BUTTON_NEW_SCRIPT, BN_CLICKED, OnNewScript )
        COMMAND_HANDLER_EX( IDC_BUTTON_ADD_FILE, BN_CLICKED, OnAddFile )
        COMMAND_HANDLER_EX( IDC_BUTTON_REMOVE_FILE, BN_CLICKED, OnRemoveFile )
        COMMAND_HANDLER_EX( IDC_BUTTON_RENAME_FILE, BN_CLICKED, OnRenameFile )
        COMMAND_HANDLER_EX( IDC_BUTTON_OPEN_FOLDER, BN_CLICKED, OnOpenContainingFolder )
        COMMAND_HANDLER_EX( IDC_BUTTON_EDIT_SCRIPT, BN_CLICKED, OnEditScript )
        COMMAND_HANDLER_EX( ID_EDIT_WITH_EXTERNAL, BN_CLICKED, OnEditScriptWith )
        COMMAND_HANDLER_EX( ID_EDIT_WITH_INTERNAL, BN_CLICKED, OnEditScriptWith )
#pragma warning( push )
#pragma warning( disable : 26454 ) // Arithmetic overflow
        NOTIFY_HANDLER_EX( IDC_BUTTON_EDIT_SCRIPT, BCN_DROPDOWN, OnEditScriptDropDown )
#pragma warning( pop )
        MESSAGE_HANDLER_EX( com::FileDropTarget::GetOnDropMsg(), OnDropFiles )
        CHAIN_MSG_MAP( CDialogResize<CConfigTabPackage> )
    END_MSG_MAP()

public:
    CConfigTabPackage( CDialogConf& parent, config::ParsedPanelSettings& settings );
    ~CConfigTabPackage() override = default;

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
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDestroy();
    void OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl );

    void OnNewScript( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnAddFile( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnRemoveFile( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnRenameFile( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnOpenContainingFolder( UINT uNotifyCode, int nID, CWindow wndCtl );

    void OnEditScript( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnEditScriptWith( UINT uNotifyCode, int nID, CWindow wndCtl );
    LONG OnEditScriptDropDown( LPNMHDR pnmh );

    LRESULT OnScriptSaved( UINT uMsg, WPARAM wParam, LPARAM lParam );
    LRESULT OnDropFiles( UINT uMsg, WPARAM wParam, LPARAM lParam );

    void DoFullDdxToUi();
    void UpdateUiButtons();

    void InitializeLocalData();

    void InitializeFilesListBox();
    void SortFiles();
    void UpdateListBoxFromData();

    void AddFile( const std::filesystem::path& path );

private:
    bool suppressDdxFromUi_ = true;

    CDialogConf& parent_;
    config::ParsedPanelSettings& settings_; ///< used only for package data save

    std::filesystem::path packagePath_;
    bool isSample_;
    std::filesystem::path mainScriptPath_;

    qwr::u8string& scriptName_;
    qwr::u8string& scriptVersion_;
    qwr::u8string& scriptAuthor_;
    qwr::u8string& scriptDescription_;

    bool& shouldGrabFocus_;
    bool& enableDragDrop_;

    std::filesystem::path focusedFile_;
    int focusedFileIdx_ = 0;

    std::array<std::unique_ptr<qwr::ui::IUiDdx>, 7> ddx_;

    std::vector<std::filesystem::path> files_;
    CListBox filesListBox_;
    CComPtr<com::FileDropTarget> pFilesListBoxDrop_;
};

} // namespace smp::ui
