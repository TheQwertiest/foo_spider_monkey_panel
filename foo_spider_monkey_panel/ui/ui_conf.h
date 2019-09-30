#pragma once

#include <scintilla/editorctrl.h>

#include <resource.h>
#include <user_message.h>

// Forward declarations
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
    CDialogConf( smp::panel::js_panel_window* p_parent );

    BEGIN_DLGRESIZE_MAP( CDialogConf )
        DLGRESIZE_CONTROL( IDC_STATIC_GUID, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    // TODO: add hook for KEYBOARD somewhere for CCustomEditFindReplaceDlg (e.g. ESC, Enter, TAB and etc)

    BEGIN_MSG_MAP( CDialogConf )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_NOTIFY( OnNotify )
        MESSAGE_HANDLER( static_cast<UINT>( smp::MiscMessage::key_down ), OnUwmKeyDown )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
        COMMAND_ID_HANDLER_EX( IDAPPLY, OnCloseCmd )
        // Menu
        COMMAND_ID_HANDLER_EX( ID_FILE_SAVE, OnFileSave )
        COMMAND_ID_HANDLER_EX( ID_FILE_IMPORT, OnFileImport )
        COMMAND_ID_HANDLER_EX( ID_FILE_EXPORT, OnFileExport )
        COMMAND_ID_HANDLER_EX( ID_EDIT_RESETTODEFAULT, OnEditResetDefault )
        COMMAND_RANGE_HANDLER_EX( ID_EDGESTYLE_NONE, ID_EDGESTYLE_GREY, OnFeaturesEdgeStyle )
        COMMAND_ID_HANDLER_EX( ID_PANELFEATURES_PSEUDOTRANSPARENT, OnFeaturesPseudoTransparent )
        COMMAND_ID_HANDLER_EX( ID_PANELFEATURES_GRABFOCUS, OnFeaturesGrabFocus )
        COMMAND_ID_HANDLER_EX( ID_HELP, OnHelp )
        COMMAND_ID_HANDLER_EX( ID_APP_ABOUT, OnAbout )
        CHAIN_MSG_MAP( CDialogResize<CDialogConf> )
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_CONFIG
    };

    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnNotify( int idCtrl, LPNMHDR pnmh );
    LRESULT OnUwmKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // Menu
    LRESULT OnFileSave( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFileImport( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFileExport( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnEditResetDefault( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFeaturesEdgeStyle( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFeaturesPseudoTransparent( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFeaturesGrabFocus( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnHelp( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnAbout( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    bool ProcessKey( unsigned vk );
    void Apply();

private:
    smp::panel::js_panel_window* m_parent = nullptr;

    scintilla::CScriptEditorCtrl sciEditor_;
    CMenu menu;

    std::u8string m_caption;
};

} // namespace smp::ui
