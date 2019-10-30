#include <stdafx.h>

#include "ui_sci_find_replace.h"

#include <utils/hook_handler.h>

namespace scintilla
{

bool FindReplaceState::operator!=( const FindReplaceState& other ) const
{
    return ( findText != other.findText
             || replaceText != other.replaceText
             || findDirection != other.findDirection
             || isCaseSensitive != other.isCaseSensitive
             || findWholeWord != other.findWholeWord
             || useWrapAround != other.useWrapAround
             || useRegExp != other.useRegExp );
}

bool FindReplaceState::operator==( const FindReplaceState& other ) const {
    return !( *this != other );
}

DWORD FindReplaceState::ToScintillaFlags( DWORD currentFlags ) const
{
    SetFlag( currentFlags, isCaseSensitive, SCFIND_MATCHCASE );
    SetFlag( currentFlags, findWholeWord, SCFIND_WHOLEWORD );
    SetFlag( currentFlags, useRegExp, SCFIND_REGEXP );

    return currentFlags;
}
void FindReplaceState::FromScintillaFlags( DWORD scintillaFlags )
{
    isCaseSensitive = !!( scintillaFlags & SCFIND_MATCHCASE );
    findWholeWord = !!( scintillaFlags & SCFIND_WHOLEWORD );
    useRegExp = !!( scintillaFlags & SCFIND_REGEXP );
}
DWORD FindReplaceState::ToFrFlags( DWORD currentFlags ) const
{
    SetFlag( currentFlags, findDirection == Direction::down, FR_DOWN );
    SetFlag( currentFlags, isCaseSensitive, FR_MATCHCASE );
    SetFlag( currentFlags, findWholeWord, FR_WHOLEWORD );

    return currentFlags;
}
void FindReplaceState::FromFrFlags( DWORD frFlags )
{
    findDirection = ( !!( frFlags & FR_DOWN ) ? Direction::down : Direction::up );
    isCaseSensitive = !!( frFlags & FR_MATCHCASE );
    findWholeWord = !!( frFlags & FR_WHOLEWORD );
}

void FindReplaceState::SetFlag( DWORD& currentFlags, bool newFlagValue, DWORD flag )
{
    if ( newFlagValue )
    {
        currentFlags |= flag;
    }
    else
    {
        currentFlags &= ~flag;
    }
}

CCustomFindReplaceDlg::CCustomFindReplaceDlg( bool useRegExp )
    : useRegExp_( useRegExp )
{
}

LRESULT CCustomFindReplaceDlg::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = false; ///< don't suppress base class methods
    uButton_SetCheck( m_hWnd, IDC_CHECK_USE_REGEXP, useRegExp_ );

    hookId_ = smp::utils::HookHandler::GetInstance().RegisterHook(
        [hParent = m_hWnd]( int code, WPARAM wParam, LPARAM lParam ) {
            GetMsgProc( code, wParam, lParam, hParent );
        } );

    return 0;
}

LRESULT CCustomFindReplaceDlg::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = false; ///< don't suppress base class methods
    if ( hookId_ )
    {
        smp::utils::HookHandler::GetInstance().UnregisterHook( hookId_ );
        hookId_ = 0;
    }
    return 0;
}

LRESULT CCustomFindReplaceDlg::OnActivate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = false; ///< don't suppress base class methods

    if ( LOWORD( wParam ) == WA_INACTIVE && IsWindowVisible() )
    {
        ModifyStyleEx( 0, WS_EX_LAYERED );
        SetLayeredWindowAttributes( m_hWnd, 0, 150, LWA_ALPHA );
    }
    else
    {
        ModifyStyleEx( WS_EX_LAYERED, 0 );
    }

    return 0;
}

LRESULT CCustomFindReplaceDlg::OnWrapAroundClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL bHandled )
{
    wrapAroundSearch_ = uButton_GetCheck( m_hWnd, wID );
    return 0;
}

LRESULT CCustomFindReplaceDlg::OnUseRegExpClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL bHandled )
{
    useRegExp_ = uButton_GetCheck( m_hWnd, wID );
    return 0;
}

void CCustomFindReplaceDlg::GetMsgProc( int code, WPARAM wParam, LPARAM lParam, HWND hParent )
{
    if ( LPMSG pMsg = reinterpret_cast<LPMSG>( lParam );
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

bool CCustomFindReplaceDlg::GetRegExpState() const
{
    return useRegExp_;
}

void CCustomFindReplaceDlg::SetRegExpState( bool newState )
{
    useRegExp_ = newState;
    uButton_SetCheck( m_hWnd, IDC_CHECK_USE_REGEXP, useRegExp_ );
}

bool CCustomFindReplaceDlg::GetWrapAroundSearchState() const
{
    return wrapAroundSearch_;
}

void CCustomFindReplaceDlg::SetWrapAroundSearchState( bool newState )
{
    wrapAroundSearch_ = newState;
    uButton_SetCheck( m_hWnd, chx3, wrapAroundSearch_ );
}

} // namespace scintilla
