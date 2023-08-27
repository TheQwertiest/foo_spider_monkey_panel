#pragma once

#include <com/objects/file_drop_target.h>
#include <config/panel_config.h>
#include <config/smp_package/delayed_package_actions.h>
#include <config/smp_package/package.h>
#include <config/smp_package/package_manager.h>
#include <resources/resource.h>

#include <qwr/ui_ddx.h>

#include <filesystem>
#include <optional>

namespace smp::ui
{

class CDialogPackageManager : public CDialogImpl<CDialogPackageManager>
{
public:
    enum
    {
        IDD = IDD_DIALOG_PACKAGE_MANAGER
    };

    BEGIN_MSG_MAP( CDialogPackageManager )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroy )
        COMMAND_HANDLER_EX( IDC_LIST_PACKAGES, LBN_SELCHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_BUTTON_NEW_PACKAGE, BN_CLICKED, OnNewPackage )
        COMMAND_HANDLER_EX( IDC_BUTTON_DELETE_PACKAGE, BN_CLICKED, OnDeletePackage )
        COMMAND_HANDLER_EX( IDC_BUTTON_IMPORT_PACKAGE, BN_CLICKED, OnImportPackage )
        COMMAND_HANDLER_EX( IDC_BUTTON_EXPORT_PACKAGE, BN_CLICKED, OnExportPackage )
        COMMAND_HANDLER_EX( IDC_BUTTON_OPEN_FOLDER, BN_CLICKED, OnOpenFolder )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
#pragma warning( push )
#pragma warning( disable : 26454 ) // Arithmetic overflow
        NOTIFY_HANDLER_EX( IDC_RICHEDIT_PACKAGE_INFO, EN_LINK, OnRichEditLinkClick )
#pragma warning( pop )
        MESSAGE_HANDLER_EX( com::FileDropTarget::GetOnDropMsg(), OnDropFiles )
    END_MSG_MAP()

    CDialogPackageManager( const qwr::u8string& currentPackageId );

    [[nodiscard]] std::optional<config::RawSmpPackage> GetPackage() const;

private:
    struct PackageData
    {
        std::wstring displayedName;
        qwr::u8string id;
        std::optional<config::SmpPackage> packageOpt;
        std::wstring errorText;
        config::PackageDelayStatus status;
    };

private:
    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDestroy();
    void OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnNewPackage( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnDeletePackage( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnImportPackage( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnExportPackage( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnOpenFolder( UINT uNotifyCode, int nID, CWindow wndCtl );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnRichEditLinkClick( LPNMHDR pnmh );
    LRESULT OnDropFiles( UINT uMsg, WPARAM wParam, LPARAM lParam );

    void DoFullDdxToUi();
    void UpdateUiButtons();

    void LoadPackages();
    void SortPackages();

    void UpdateListBoxFromData();
    void UpdatedUiPackageInfo();

    PackageData GeneratePackageData( const config::SmpPackage& package );
    /// @return true if restart is needed
    bool ImportPackage( const std::filesystem::path& path );

    bool ConfirmPackageOverwrite( const std::filesystem::path& oldPackageJson, const config::SmpPackage& newPackage );
    bool ConfirmRebootOnPackageInUse();

private:
    qwr::u8string focusedPackageId_;
    int focusedPackageIdx_ = -1;
    std::array<std::unique_ptr<qwr::ui::IUiDdx>, 1> ddx_;

    config::SmpPackageManager packageManager_;

    std::vector<PackageData> packages_;
    CListBox packagesListBox_;
    CComPtr<com::FileDropTarget> pPackagesListBoxDrop_;

    CRichEditCtrl packageInfoEdit_;
};

} // namespace smp::ui
