#include <stdafx.h>

#include "ui_sci_find_replace.h"

#include <qwr/hook_handler.h>

namespace smp::ui::sci
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

bool FindReplaceState::operator==( const FindReplaceState& other ) const
{
    return !( *this != other );
}

DWORD FindReplaceState::ToScintillaFlags( DWORD currentFlags ) const
{
    SetFlag( currentFlags, isCaseSensitive, SCFIND_MATCHCASE );
    SetFlag( currentFlags, findWholeWord, SCFIND_WHOLEWORD );
    SetFlag( currentFlags, useRegExp, SCFIND_REGEXP | SCFIND_POSIX );

    return currentFlags;
}
void FindReplaceState::FromScintillaFlags( DWORD scintillaFlags )
{
    isCaseSensitive = !!( scintillaFlags & SCFIND_MATCHCASE );
    findWholeWord = !!( scintillaFlags & SCFIND_WHOLEWORD );
    useRegExp = !!( scintillaFlags & ( SCFIND_REGEXP | SCFIND_POSIX ) );
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

LRESULT CCustomFindReplaceDlg::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& bHandled )
{
    bHandled = FALSE; ///< don't suppress base class methods
    uButton_SetCheck( m_hWnd, IDC_CHECK_USE_REGEXP, useRegExp_ );
    uButton_SetCheck( m_hWnd, IDC_CHECK_WRAPAROUND, wrapAroundSearch_ );
    UpdateRegExpHacksState();

    hookId_ = qwr::HookHandler::GetInstance().RegisterHook(
        [hParent = m_hWnd]( int code, WPARAM wParam, LPARAM lParam ) {
            GetMsgProc( code, wParam, lParam, hParent );
        } );

    return 0;
}

LRESULT CCustomFindReplaceDlg::OnDestroy( UINT, WPARAM, LPARAM, BOOL& bHandled )
{
    bHandled = FALSE; ///< don't suppress base class methods
    if ( hookId_ )
    {
        qwr::HookHandler::GetInstance().UnregisterHook( hookId_ );
        hookId_ = 0;
    }
    return 0;
}

LRESULT CCustomFindReplaceDlg::OnActivate( UINT, WPARAM wParam, LPARAM, BOOL& bHandled )
{
    bHandled = FALSE; ///< don't suppress base class methods

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

LRESULT CCustomFindReplaceDlg::OnWrapAroundClick( WORD, WORD wID, HWND, BOOL )
{
    wrapAroundSearch_ = uButton_GetCheck( m_hWnd, wID );
    return 0;
}

LRESULT CCustomFindReplaceDlg::OnUseRegExpClick( WORD, WORD wID, HWND, BOOL )
{
    useRegExp_ = uButton_GetCheck( m_hWnd, wID );
    UpdateRegExpHacksState();
    return 0;
}

void CCustomFindReplaceDlg::GetMsgProc( int, WPARAM, LPARAM lParam, HWND hParent )
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

bool CCustomFindReplaceDlg::GetRegExpState() const
{
    return useRegExp_;
}

void CCustomFindReplaceDlg::SetRegExpState( bool newState )
{
    useRegExp_ = newState;
    uButton_SetCheck( m_hWnd, IDC_CHECK_USE_REGEXP, useRegExp_ );
    UpdateRegExpHacksState();
}

bool CCustomFindReplaceDlg::GetWrapAroundSearchState() const
{
    return wrapAroundSearch_;
}

void CCustomFindReplaceDlg::SetWrapAroundSearchState( bool newState )
{
    wrapAroundSearch_ = newState;
    uButton_SetCheck( m_hWnd, IDC_CHECK_WRAPAROUND, wrapAroundSearch_ );
}

void CCustomFindReplaceDlg::UpdateRegExpHacksState()
{
    // Whole word search and up direction do not work with regexp (bug in Scintilla)
    CButton wholeWordSearchBtn( GetDlgItem( chx1 ) );
    if ( useRegExp_ )
    {
        wholeWordSearchBtn.SetCheck( BST_UNCHECKED );
    }
    wholeWordSearchBtn.EnableWindow( useRegExp_ ? FALSE : TRUE );

    const bool isFindOnly = !!GetDlgItem( rad1 );
    if ( isFindOnly )
    {
        CButton upDirectionBtn( GetDlgItem( rad1 ) );
        CButton downDirectionBtn( GetDlgItem( rad2 ) );
        if ( useRegExp_ )
        {
            upDirectionBtn.SetCheck( BST_UNCHECKED );
            downDirectionBtn.SetCheck( BST_CHECKED );
        }
        upDirectionBtn.EnableWindow( useRegExp_ ? FALSE : TRUE );
    }
}

} // namespace smp::ui::sci
