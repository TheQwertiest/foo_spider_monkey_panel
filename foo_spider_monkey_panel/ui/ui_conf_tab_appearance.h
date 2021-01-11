#pragma once

#include <config/parsed_panel_config.h>
#include <ui/ui_itab.h>

#include <resource.h>

#include <qwr/ui_ddx.h>

#include <array>

namespace smp::ui
{

class CDialogConfNew;

class CConfigTabAppearance
    : public CDialogImpl<CConfigTabAppearance>
    , public ITab
{
public:
    enum
    {
        IDD = IDD_DIALOG_CONF_TAB_APPEARANCE
    };

    BEGIN_MSG_MAP( CConfigTabAppearance )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_HANDLER_EX( IDC_RADIO_EDGE_NO, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_RADIO_EDGE_GREY, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_RADIO_EDGE_SUNKEN, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_PSEUDOTRANSPARENT, BN_CLICKED, OnDdxUiChange )
    END_MSG_MAP()

public:
    CConfigTabAppearance( CDialogConfNew& parent, config::ParsedPanelSettings& settings );
    ~CConfigTabAppearance() override = default;

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
    void OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl );

    void DoFullDdxToUi();
    void InitializeLocalOptions();

private:
    CDialogConfNew& parent_;

    config::EdgeStyle& edgeStyle_;
    bool& isPseudoTransparent_;

    int edgeStyleId_;

    std::array<std::unique_ptr<qwr::ui::IUiDdx>, 2> ddx_;
};

} // namespace smp::ui
