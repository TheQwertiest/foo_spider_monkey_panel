#include <stdafx.h>

#include "ui_sci_goto.h"

#include <ui/scintilla/ui_sci_editor.h>

#include <qwr/hook_handler.h>
#include <qwr/pfc_helpers_ui.h>
#include <qwr/string_helpers.h>

namespace
{

enum class GotoMsg : uint32_t
{
    PerformGoto,
    FinalMessage
};

}

namespace smp::ui::sci
{

CDialogGoto::CDialogGoto( HWND hParent, int curLineNumber )
    : hParent_( hParent )
    , curLineNumber_( curLineNumber )
{
}

LRESULT CDialogGoto::OnInitDialog( HWND, LPARAM )
{
    uSetWindowText( GetDlgItem( IDC_EDIT_LINENUMBER ), std::to_string( curLineNumber_ ).c_str() );

    hookId_ = qwr::HookHandler::GetInstance().RegisterHook(
        [hParent = m_hWnd]( int code, WPARAM wParam, LPARAM lParam ) {
            GetMsgProc( code, wParam, lParam, hParent );
        } );

    return TRUE; // set focus to default control
}

void CDialogGoto::OnDestroy()
{
    if ( hookId_ )
    {
        qwr::HookHandler::GetInstance().UnregisterHook( hookId_ );
        hookId_ = 0;
    }
}

LRESULT CDialogGoto::OnCloseCmd( WORD, WORD wID, HWND )
{
    if ( wID == IDOK )
    {
        const auto text = qwr::pfc_x::uGetWindowText<char>( GetDlgItem( IDC_EDIT_LINENUMBER ) );
        const auto numRet = qwr::string::GetNumber<unsigned>( static_cast<qwr::u8string_view>( text ) );
        if ( numRet )
        {
            ::SendMessage( hParent_, CScintillaGotoImpl::GetGotoMsg(), (WPARAM)GotoMsg::PerformGoto, (LPARAM)*numRet );
        }
    }

    assert( !m_bModal );
    DestroyWindow();

    return 0;
}

void CDialogGoto::OnFinalMessage( _In_ HWND /*hWnd*/ )
{
    ::SendMessage( hParent_, CScintillaGotoImpl::GetGotoMsg(), (WPARAM)GotoMsg::FinalMessage, NULL );
    delete this;
}

void CDialogGoto::GetMsgProc( int, WPARAM, LPARAM lParam, HWND hParent )
{
    if ( auto pMsg = reinterpret_cast<LPMSG>( lParam );
         pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST )
    { // Only react to keypress events
        HWND hWndFocus = ::GetFocus();
        if ( hWndFocus != nullptr && ( ( hParent == hWndFocus ) || ::IsChild( hParent, hWndFocus ) ) )
        {
            if ( ::IsDialogMessage( hParent, pMsg ) )
            {
                pMsg->message = WM_NULL;
            }
        }
    }
}

CScintillaGotoImpl::CScintillaGotoImpl( CScriptEditorCtrl& sciEdit )
    : sciEdit_( sciEdit )
{
}
void CScintillaGotoImpl::ShowGoTo()
{
    if ( !pGoto_ )
    {
        pGoto_ = new CDialogGoto( static_cast<HWND>( sciEdit_ ), sciEdit_.LineFromPosition( sciEdit_.GetCurrentPos() ) + 1 );
        if ( !pGoto_->Create( static_cast<HWND>( sciEdit_ ) ) )
        {
            delete pGoto_;
            pGoto_ = nullptr;
            return;
        }

        pGoto_->SetActiveWindow();
        pGoto_->CenterWindow( sciEdit_ );
        pGoto_->ShowWindow( SW_SHOW );
    }
    else
    {
        pGoto_->SetActiveWindow();
        pGoto_->ShowWindow( SW_SHOW );
    }
}

UINT CScintillaGotoImpl::GetGotoMsg()
{
    static const UINT msgId = ::RegisterWindowMessage( L"smp_dlg_sci_goto" );
    return msgId;
}

LRESULT CScintillaGotoImpl::OnGotoCmd( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
{
    switch ( (GotoMsg)wParam )
    {
    case GotoMsg::PerformGoto:
        sciEdit_.GotoLine( (uint32_t)lParam - 1 );
        break;
    case GotoMsg::FinalMessage:
        pGoto_ = nullptr;
        break;
    default:
        assert( 0 );
        break;
    }

    return 0;
}

} // namespace smp::ui::sci
