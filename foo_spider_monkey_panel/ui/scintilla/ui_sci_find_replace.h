#pragma once

#include <resource.h>

namespace scintilla
{

class CScriptEditorCtrl;

struct FindReplaceState
{
    DWORD ToScintillaFlags( DWORD currentFlags = 0 ) const;
    void FromScintillaFlags( DWORD scintillaFlags );
    DWORD ToFrFlags( DWORD currentFlags = 0 ) const;
    void FromFrFlags( DWORD frFlags );

    std::u8string findText;
    std::u8string replaceText;
    bool findDown = true;
    bool isCaseSensitive = false;
    bool findWholeWord = false;
    bool useRegExp = false;

private:
    static void SetFlag( DWORD& currentFlags, bool newFlagValue, DWORD flag );
};

class CCustomFindReplaceDlg
    : public CFindReplaceDialog
{
protected:
    BEGIN_MSG_MAP( CCustomFindReplaceDlg )
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
        COMMAND_HANDLER( IDC_CHECK_USE_REGEXP, BN_CLICKED, OnUseRegExpClick )
        CHAIN_MSG_MAP( CFindReplaceDialog )
    END_MSG_MAP()

    CCustomFindReplaceDlg( bool useRegExp );
    ~CCustomFindReplaceDlg() override = default;

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnUseRegExpClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL bHandled );

    static void GetMsgProc( int code, WPARAM wParam, LPARAM lParam, HWND hParent, CCustomFindReplaceDlg* pParent );

public:
    bool GetRegExpState() const;
    void SetRegExpState( bool newState );

private:
    uint32_t hookId_ = 0;
    bool useRegExp_ = false;
};

template <typename T>
class CScintillaFindReplaceImpl
    : public CEditFindReplaceImplBase<T>
{
protected:
    using TFindReplaceDlg = CCustomFindReplaceDlg;
    using BaseClass = CEditFindReplaceImplBase<T>;

    BEGIN_MSG_MAP( CScintillaFindReplaceImpl )
        ALT_MSG_MAP( 1 )
        if ( uMsg != WM_DESTROY )
        { // TODO: workaround for https://sourceforge.net/p/wtl/discussion/374434/thread/cdeef79112/:
            // callback for WM_DESTROY attempts to delete already deleted pointer
            CHAIN_MSG_MAP_ALT( BaseClass, 1 )
        }
    END_MSG_MAP()

public:
    CScintillaFindReplaceImpl( scintilla::CScriptEditorCtrl& sciEditor )
        : sciEditor_( sciEditor )
    {
    }

    bool HasFindText() const
    {
        return !lastState_.findText.empty();
    }

    bool Find()
    {
        if ( lastState_.findDown )
        {
            return FindNext();
        }
        else
        {
            return FindPrevious();
        }
    }

    bool FindNext()
    {
        SendMessage( hEdit_, SCI_CHARRIGHT, 0, 0 );
        SendMessage( hEdit_, SCI_SEARCHANCHOR, 0, 0 );
        int pos = ::SendMessage( hEdit_, SCI_SEARCHNEXT, lastState_.ToScintillaFlags(), reinterpret_cast<LPARAM>( lastState_.findText.c_str() ) );
        return FindResult( pos, lastState_.findText );
    }

    bool FindPrevious()
    {
        SendMessage( hEdit_, SCI_SEARCHANCHOR, 0, 0 );
        int pos = ::SendMessage( hEdit_, SCI_SEARCHPREV, lastState_.ToScintillaFlags(), reinterpret_cast<LPARAM>( lastState_.findText.c_str() ) );
        return FindResult( pos, lastState_.findText );
    }

    // CEditFindReplaceImplBase

    TFindReplaceDlg* CreateFindReplaceDialog( BOOL bFindOnly,
                                              LPCTSTR lpszFindWhat,
                                              LPCTSTR lpszReplaceWith = NULL,
                                              DWORD dwFlags = FR_DOWN,
                                              HWND hWndParent = NULL )
    {
        assert( sciEditor_.operator HWND() == hWndParent );

        TFindReplaceDlg* findReplaceDialog = new TFindReplaceDlg( lastState_.useRegExp );
        if ( findReplaceDialog == NULL )
        {
            ::MessageBeep( MB_ICONHAND );
        }
        else
        {
            auto& fr = findReplaceDialog->m_fr;
            fr.Flags |= FR_ENABLETEMPLATE;
            fr.lpTemplateName = ( bFindOnly ? MAKEINTRESOURCE( IDD_FINDDLGORD_WITH_REGEXP ) : MAKEINTRESOURCE( IDD_REPLACEDLGORD_WITH_REGEXP ) );
            fr.hInstance = ModuleHelper::GetModuleInstance();

            const std::wstring findText = ( lpszFindWhat && wcslen( lpszFindWhat ) ? lpszFindWhat : smp::unicode::ToWide( lastState_.findText ) );
            const std::wstring replaceText = ( lpszReplaceWith && wcslen( lpszReplaceWith ) ? lpszReplaceWith : smp::unicode::ToWide( lastState_.replaceText ) );

            HWND hWndFindReplace = findReplaceDialog->Create( bFindOnly,
                                                              findText.c_str(),
                                                              ( replaceText.empty() ? nullptr : replaceText.c_str() ),
                                                              lastState_.ToFrFlags( dwFlags ),
                                                              sciEditor_ );
            if ( hWndFindReplace == NULL )
            {
                delete findReplaceDialog;
                findReplaceDialog = NULL;
            }
            else
            {
                hEdit_ = sciEditor_;
                findReplaceDialog->SetRegExpState( lastState_.useRegExp );
                findReplaceDialog->SetActiveWindow();
                findReplaceDialog->ShowWindow( SW_SHOW );
            }
        }

        return findReplaceDialog;
    }

    // TODO: workaround for https://sourceforge.net/p/wtl/discussion/374434/thread/cdeef79112/:
    // incorrect argument passed to `ClientToScreen`
    void AdjustDialogPosition( HWND hWndDialog )
    {
        ATLASSERT( ( hWndDialog != NULL ) && ::IsWindow( hWndDialog ) );

        T* pT = static_cast<T*>( this );
        LONG nStartChar = 0, nEndChar = 0;
        // Send EM_GETSEL so we can use both Edit and RichEdit
        // (CEdit::GetSel uses int&, and CRichEditCtrlT::GetSel uses LONG&)
        ::SendMessage( pT->m_hWnd, EM_GETSEL, (WPARAM)&nStartChar, (LPARAM)&nEndChar );
        POINT point = pT->PosFromChar( nStartChar );
        ::ClientToScreen( pT->operator HWND(), &point ); ///< the fix is here
        RECT rect = {};
        ::GetWindowRect( hWndDialog, &rect );
        if ( ::PtInRect( &rect, point ) != FALSE )
        {
            if ( point.y > ( rect.bottom - rect.top ) )
            {
                ::OffsetRect( &rect, 0, point.y - rect.bottom - 20 );
            }
            else
            {
                int nVertExt = GetSystemMetrics( SM_CYSCREEN );
                if ( ( point.y + ( rect.bottom - rect.top ) ) < nVertExt )
                    ::OffsetRect( &rect, 0, 40 + point.y - rect.top );
            }

            ::MoveWindow( hWndDialog, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE );
        }
    }

    BOOL FindTextSimple( LPCTSTR lpszFind, BOOL bMatchCase, BOOL bWholeWord, BOOL bFindDown /*= TRUE */ )
    {
        UpdateFindReplaceState();
        return Find();
    }

    void ReplaceSel( const CString& newText )
    {
        sciEditor_.ReplaceSelection( smp::unicode::ToU8( static_cast<const wchar_t*>( newText ) ).c_str() );
    }

    LONG GetSelText( CString& strText ) const
    {
        const size_t textSize = static_cast<size_t>( sciEditor_.GetSelectedText( nullptr ) );

        std::u8string text;
        text.resize( textSize + 1 );
        int iRet = sciEditor_.GetSelectedText( text.data() );
        text.resize( strlen( text.c_str() ) );

        strText = smp::unicode::ToWide( text ).c_str();

        return iRet;
    }

    void HideSelection( BOOL bHide, BOOL bChangeStyle )
    {
        (void)bChangeStyle;
        sciEditor_.ChangeSelectionColour( bHide );
    }

    POINT PosFromChar( UINT nChar ) const
    {
        return POINT{
            sciEditor_.PointXFromPosition( nChar ),
            sciEditor_.PointYFromPosition( nChar )
        };
    }

private:
    bool FindResult( int pos, const std::u8string& which )
    {
        if ( pos != -1 )
        { // Scroll to view
            ::SendMessage( hEdit_, SCI_SCROLLCARET, 0, 0 );
            return true;
        }
        else
        {
            return false;
        }
    }

    void UpdateFindReplaceState()
    {
        auto pFindReplaceDialog = static_cast<TFindReplaceDlg*>( BaseClass::m_pFindReplaceDialog );
        if ( pFindReplaceDialog )
        {
            lastState_.FromFrFlags( pFindReplaceDialog->m_fr.Flags );
            lastState_.findText = smp::unicode::ToU8( pFindReplaceDialog->GetFindString() );
            if ( !isFindOnlyDialog_ )
            {
                lastState_.replaceText = smp::unicode::ToU8( pFindReplaceDialog->GetReplaceString() );
            }
            lastState_.useRegExp = pFindReplaceDialog->GetRegExpState();
        }
    }

private:
    scintilla::CScriptEditorCtrl& sciEditor_;
    HWND hEdit_ = nullptr;

    bool isFindOnlyDialog_ = true;
    static FindReplaceState lastState_;
};

template <typename T>
scintilla::FindReplaceState CScintillaFindReplaceImpl<T>::lastState_;

} // namespace scintilla
