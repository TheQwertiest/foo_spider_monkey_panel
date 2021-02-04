#pragma once

#include <resources/resource.h>

#include <filesystem>
#include <mutex>
#include <thread>

namespace smp::ui
{

class CEditInProgress : public CDialogImpl<CEditInProgress>
{
public:
    enum
    {
        IDD = IDD_DIALOG_EDIT_IN_PROGRESS
    };

    BEGIN_MSG_MAP( CEditInProgress )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_ID_HANDLER_EX( IDC_EDIT_IN_PROGRESS_FOCUS, OnEditorFocusCmd )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
    END_MSG_MAP()

    CEditInProgress( const std::filesystem::path& editor, const std::filesystem::path& file );

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnEditorFocusCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );

private:
    void EditorHandler();

private:
    const std::filesystem::path editor_;
    const std::filesystem::path file_;

    std::mutex mutex_;
    bool hasEditorLaunched_ = false;
    bool isClosing_ = false;
    HWND hEditorWnd_ = nullptr;
    HANDLE hEditorProcess_ = nullptr;
    std::thread editorThread_;
    std::string errorMessage_;
};

} // namespace smp::ui
