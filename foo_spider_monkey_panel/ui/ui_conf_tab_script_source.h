#pragma once

#include <config/parsed_panel_config.h>
#include <panel/user_message.h>
#include <resources/resource.h>
#include <ui/ui_itab.h>

#include <qwr/ui_ddx.h>

#include <array>

namespace smp::ui
{

class CDialogConf;

class CConfigTabScriptSource
    : public CDialogImpl<CConfigTabScriptSource>
    , public CDialogResize<CConfigTabScriptSource>
    , public ITab
{
public:
    enum
    {
        IDD = IDD_DIALOG_CONF_TAB_SCRIPT_SRC
    };

    BEGIN_DLGRESIZE_MAP( CConfigTabScriptSource )
        DLGRESIZE_CONTROL( IDC_GROUP_SCRIPT_SRC, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDC_COMBO_SRC_SAMPLE, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_TEXTEDIT_SRC_PATH, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_BUTTON_BROWSE, DLSZ_MOVE_X )
        DLGRESIZE_CONTROL( IDC_TEXTEDIT_SRC_PACKAGE, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_BUTTON_OPEN_PKG_MGR, DLSZ_MOVE_X )
        DLGRESIZE_CONTROL( IDC_BUTTON_EDIT_SCRIPT, DLSZ_MOVE_X | DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CConfigTabScriptSource )
        MSG_WM_INITDIALOG( OnInitDialog )
        MESSAGE_HANDLER_EX( static_cast<int>( InternalSyncMessage::ui_script_editor_saved ), OnScriptSaved );
        COMMAND_HANDLER_EX( IDC_RADIO_SRC_SAMPLE, BN_CLICKED, OnScriptSrcChange )
        COMMAND_HANDLER_EX( IDC_RADIO_SRC_FILE, BN_CLICKED, OnScriptSrcChange )
        COMMAND_HANDLER_EX( IDC_RADIO_SRC_MEMORY, BN_CLICKED, OnScriptSrcChange )
        COMMAND_HANDLER_EX( IDC_RADIO_SRC_PACKAGE, BN_CLICKED, OnScriptSrcChange )
        COMMAND_HANDLER_EX( IDC_COMBO_SRC_SAMPLE, CBN_SELCHANGE, OnScriptSrcChange )
        COMMAND_HANDLER_EX( IDC_BUTTON_BROWSE, BN_CLICKED, OnBrowseFile )
        COMMAND_HANDLER_EX( IDC_BUTTON_OPEN_PKG_MGR, BN_CLICKED, OnOpenPackageManager )
        COMMAND_HANDLER_EX( IDC_BUTTON_EDIT_SCRIPT, BN_CLICKED, OnEditScript )
        NOTIFY_HANDLER_EX( IDC_BUTTON_EDIT_SCRIPT, BCN_DROPDOWN, OnEditScriptDropDown )
        COMMAND_HANDLER_EX( ID_EDIT_WITH_EXTERNAL, BN_CLICKED, OnEditScriptWith )
        COMMAND_HANDLER_EX( ID_EDIT_WITH_INTERNAL, BN_CLICKED, OnEditScriptWith )
        CHAIN_MSG_MAP( CDialogResize<CConfigTabScriptSource> )
    END_MSG_MAP()

    struct SampleComboBoxElem
    {
        SampleComboBoxElem( std::wstring path, std::wstring displayedName )
            : path( std::move( path ) )
            , displayedName( std::move( displayedName ) )
        {
        }
        std::wstring path;
        std::wstring displayedName;
    };

public:
    CConfigTabScriptSource( CDialogConf& parent, config::ParsedPanelSettings& settings );
    ~CConfigTabScriptSource() override = default;

    // > IUiTab
    HWND CreateTab( HWND hParent ) override;
    CDialogImplBase& Dialog() override;
    const wchar_t* Name() const override;
    bool HasChanged() override;
    void Apply() override;
    void Revert() override;
    // < IUiTab

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnScriptSrcChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnDdxValueChange( int nID );

    void OnBrowseFile( UINT uNotifyCode, int nID, CWindow wndCtl );
    std::optional<std::filesystem::path> OnBrowseFileImpl();
    void OnOpenPackageManager( UINT uNotifyCode, int nID, CWindow wndCtl );
    std::optional<config::ParsedPanelSettings> OnOpenPackageManagerImpl( const std::u8string packageId );

    void OnEditScript( UINT uNotifyCode, int nID, CWindow wndCtl );
    LONG OnEditScriptDropDown( LPNMHDR pnmh ) const;
    void OnEditScriptWith( UINT uNotifyCode, int nID, CWindow wndCtl );

    LRESULT OnScriptSaved( UINT uMsg, WPARAM wParam, LPARAM lParam );

    void InitializeLocalOptions();
    void InitializeSourceType();
    void DoFullDdxToUi();
    void DoSourceTypeDdxToUi();
    void DoButtonsDdxToUi();
    void InitializeSamplesComboBox();

    bool RequestConfirmationForReset();

private:
    bool suppressUiDdx_ = true;

    CDialogConf& parent_;
    config::ParsedPanelSettings& settings_;

    int sourceTypeId_ = 0;
    int sampleIdx_ = 0;
    std::u8string path_;
    std::u8string packageName_;

    std::array<std::unique_ptr<qwr::ui::IUiDdx>, 4> ddx_;

    static std::vector<SampleComboBoxElem> sampleData_;
    CComboBox samplesComboBox_;
};

} // namespace smp::ui
