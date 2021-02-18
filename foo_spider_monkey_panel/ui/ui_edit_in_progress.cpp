#include <stdafx.h>

#include "ui_edit_in_progress.h"

#include <qwr/thread_helpers.h>
#include <qwr/ui_centered_message_box.h>
#include <qwr/winapi_error_helpers.h>

namespace
{

struct EnumHandleData
{
    unsigned long procId;
    HWND hWnd;
};

bool IsMainWindow( HWND handle )
{
    return ( GetWindow( handle, GW_OWNER ) == nullptr && IsWindowVisible( handle ) );
}

BOOL CALLBACK EnumWndCallback( HWND hWnd, LPARAM lParam )
{
    EnumHandleData& data = *(EnumHandleData*)lParam;
    unsigned long procId = 0;
    GetWindowThreadProcessId( hWnd, &procId );
    if ( data.procId != procId || !IsMainWindow( hWnd ) )
    {
        return TRUE;
    }
    else
    {
        data.hWnd = hWnd;
        return FALSE;
    }
}

HWND GetMainWndFromProcId( unsigned long process_id )
{
    EnumHandleData data{};
    data.procId = process_id;
    EnumWindows( EnumWndCallback, (LPARAM)&data );
    return data.hWnd;
}

std::wstring GetExternalEditorParams( const std::wstring& editorBinName )
{
    if ( editorBinName == L"notepad++.exe" )
    {
        return L"-multiInst -notabbar -nosession -noPlugin";
    }
    else if ( editorBinName == L"Code.exe"
              || editorBinName == L"subl.exe" )
    {
        return L"--new-window --wait";
    }
    else
    {
        return L"";
    }
}

} // namespace

namespace smp::ui
{

CEditInProgress::CEditInProgress( const std::filesystem::path& editor, const std::filesystem::path& file )
    : editor_( editor )
    , file_( file )
{
}

LRESULT CEditInProgress::OnInitDialog( HWND, LPARAM )
{
    (void)CenterWindow();

    editorThread_ = std::thread( [&] { EditorHandler(); } );
    qwr::SetThreadName( editorThread_, "SMP Editor Thread" );

    return FALSE; // set focus to default control
}

LRESULT CEditInProgress::OnEditorFocusCmd( WORD, WORD, HWND )
{
    // We can just call ShowWindow & SetForegroundWindow to bring hwnd to front.
    // But that would also take maximized window out of maximized state.
    // Using GetWindowPlacement preserves maximized state
    WINDOWPLACEMENT place{};
    place.length = sizeof( place );
    ::GetWindowPlacement( hEditorWnd_, &place );

    switch ( place.showCmd )
    {
    case SW_SHOWMAXIMIZED:
        ::ShowWindow( hEditorWnd_, SW_SHOWMAXIMIZED );
        break;
    case SW_SHOWMINIMIZED:
        ::ShowWindow( hEditorWnd_, SW_RESTORE );
        break;
    default:
        ::ShowWindow( hEditorWnd_, SW_NORMAL );
        break;
    }

    SetForegroundWindow( hEditorWnd_ );

    ::MessageBeep( MB_OK );

    FLASHWINFO fi{};
    fi.hwnd = hEditorWnd_;
    fi.cbSize = sizeof( fi );
    fi.dwFlags = FLASHW_CAPTION;
    fi.uCount = 7;
    fi.dwTimeout = 60;
    ::FlashWindowEx( &fi );

    return 0;
}

LRESULT CEditInProgress::OnCloseCmd( WORD, WORD wID, HWND )
{
    {
        std::scoped_lock sl{ mutex_ };
        if ( !hasEditorLaunched_ )
        {
            return 0;
        }
    }

    const auto hEditorProcess = [&] {
        std::scoped_lock sl{ mutex_ };
        return hEditorProcess_;
    }();

    if ( !hEditorProcess )
    { // process was closed
        if ( wID == IDCANCEL )
        {
            const auto errorMsg = ( errorMessage_.empty() ? std::string{ "Unknown error caused by editor" } : errorMessage_ );
            qwr::ui::MessageBoxCentered( *this,
                                         qwr::unicode::ToWide( errorMsg ).c_str(),
                                         L"Editor error",
                                         MB_ICONWARNING | MB_SETFOREGROUND | MB_OK );
        }

        if ( editorThread_.joinable() )
        {
            editorThread_.join();
        }

        EndDialog( wID == IDOK ? IDOK : IDCANCEL );
    }
    else
    { // requested by user
        assert( hEditorWnd_ );
        ::SendMessage( hEditorWnd_, WM_CLOSE, 0, 0 );
    }

    return 0;
}

void CEditInProgress::EditorHandler()
{
    try
    {
        const auto qPath = L"\"" + file_.wstring() + L"\"";
        const auto editorParams = GetExternalEditorParams( editor_.filename().wstring() ) + L" " + qPath;

        SHELLEXECUTEINFO ShExecInfo{};
        ShExecInfo.cbSize = sizeof( SHELLEXECUTEINFO );
        ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShExecInfo.lpFile = editor_.c_str();
        ShExecInfo.lpParameters = editorParams.c_str();
        ShExecInfo.nShow = SW_SHOW;

        BOOL bRet = ShellExecuteEx( &ShExecInfo );
        qwr::error::CheckWinApi( bRet, "ShellExecuteEx" );
        qwr::QwrException::ExpectTrue( !!ShExecInfo.hProcess, "Failed to get editor handle" );

        WaitForInputIdle( ShExecInfo.hProcess, INFINITE );

        {
            std::scoped_lock sl{ mutex_ };
            hEditorProcess_ = ShExecInfo.hProcess;
            hEditorWnd_ = GetMainWndFromProcId( GetProcessId( ShExecInfo.hProcess ) );
            hasEditorLaunched_ = true;
        }

        WaitForSingleObject( ShExecInfo.hProcess, INFINITE );

        {
            std::scoped_lock sl{ mutex_ };
            hEditorProcess_ = nullptr;
            if ( !isClosing_ )
            {
                PostMessage( WM_COMMAND, IDOK );
            }
        }
    }
    catch ( const std::exception& e )
    {
        std::scoped_lock sl{ mutex_ };

        hEditorProcess_ = nullptr;
        errorMessage_ = e.what();
        if ( !isClosing_ )
        {
            PostMessage( WM_COMMAND, IDCANCEL );
        }
    }
}

} // namespace smp::ui
