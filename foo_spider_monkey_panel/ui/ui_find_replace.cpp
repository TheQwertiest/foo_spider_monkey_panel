#include <stdafx.h>

#include "ui_find_replace.h"

namespace smp::ui
{

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
    SetFlag( currentFlags, findDown, FR_DOWN );
    SetFlag( currentFlags, isCaseSensitive, FR_MATCHCASE );
    SetFlag( currentFlags, findWholeWord, FR_WHOLEWORD );

    return currentFlags;
}
void FindReplaceState::FromFrFlags( DWORD frFlags )
{
    findDown = !!( frFlags & FR_DOWN );
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
    uButton_SetCheck( m_hWnd, IDC_CHECK_USE_REGEXP, useRegExp_ );
    bHandled = FALSE; ///< don't suppress base initialization
    return 0;
}

LRESULT CCustomFindReplaceDlg::OnUseRegExpClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL bHandled )
{
    useRegExp_ = uButton_GetCheck( m_hWnd, wID );
    return 0;
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

} // namespace smp::ui
