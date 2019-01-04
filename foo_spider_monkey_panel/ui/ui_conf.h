#pragma once

#include "resource.h"
#include <user_message.h>
#include <scintilla/editorctrl.h>

// Forward declarations
namespace smp::panel
{
class js_panel_window;
}

class CDialogFind;
class CDialogReplace;

class CDialogConf 
    : public CDialogImpl<CDialogConf>
    , public CDialogResize<CDialogConf>
{
public:
    CDialogConf( smp::panel::js_panel_window* p_parent );
    ~CDialogConf() override;

    BEGIN_DLGRESIZE_MAP( CDialogConf )
        DLGRESIZE_CONTROL( IDC_STATIC_GUID, DLSZ_SIZE_X )
        DLGRESIZE_CONTROL( IDC_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CDialogConf )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_NOTIFY( OnNotify )
        MESSAGE_HANDLER( static_cast<UINT>( smp::MiscMessage::key_down ), OnUwmKeyDown )
        MESSAGE_HANDLER( static_cast<UINT>( smp::MiscMessage::find_text_changed ), OnUwmFindTextChanged )
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
    LRESULT OnUwmFindTextChanged( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
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

    bool MatchShortcuts( unsigned vk );
    static bool FindNext( HWND hWnd, HWND hWndEdit, unsigned flags, const char* which );
    static bool FindPrevious( HWND hWnd, HWND hWndEdit, unsigned flags, const char* which );
    static bool FindResult( HWND hWnd, HWND hWndEdit, int pos, const char* which );
    void Apply();
    void OpenFindDialog();

private:
    smp::panel::js_panel_window* m_parent = nullptr;

    CScriptEditorCtrl m_editorctrl;
    CDialogFind* m_dlgfind = nullptr;
    CDialogReplace* m_dlgreplace = nullptr;
    CMenu menu;

    pfc::string8 m_caption;
    unsigned int m_lastFlags = 0;
    pfc::string8 m_lastSearchText;
};
