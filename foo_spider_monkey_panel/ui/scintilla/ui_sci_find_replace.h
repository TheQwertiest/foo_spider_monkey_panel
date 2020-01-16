#pragma once

#include <resource.h>

#include <Scintilla.h>

namespace scintilla
{

class CScriptEditorCtrl;

struct FindReplaceState
{
    enum class Direction
    {
        down,
        up
    };

    [[nodiscard]] bool operator!=( const FindReplaceState& other ) const;
    [[nodiscard]] bool operator==( const FindReplaceState& other ) const;

    [[nodiscard]] DWORD ToScintillaFlags( DWORD currentFlags = 0 ) const;
    void FromScintillaFlags( DWORD scintillaFlags );
    [[nodiscard]] DWORD ToFrFlags( DWORD currentFlags = 0 ) const;
    void FromFrFlags( DWORD frFlags );

    std::u8string findText;
    std::u8string replaceText;
    Direction findDirection = Direction::down;
    bool isCaseSensitive = false;
    bool findWholeWord = false;
    bool useWrapAround = true;
    bool useRegExp = false;

private:
    static void SetFlag( DWORD& currentFlags, bool newFlagValue, DWORD flag );
};

class CCustomFindReplaceDlg;

class CCustomFindReplaceDlg
    : public CFindReplaceDialog
{
protected:
    BEGIN_MSG_MAP( CCustomFindReplaceDlg )
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
        MESSAGE_HANDLER( WM_ACTIVATE, OnActivate )
        COMMAND_HANDLER( chx3, BN_CLICKED, OnWrapAroundClick )
        COMMAND_HANDLER( IDC_CHECK_USE_REGEXP, BN_CLICKED, OnUseRegExpClick )
        CHAIN_MSG_MAP( CFindReplaceDialog )
    END_MSG_MAP()

    CCustomFindReplaceDlg( bool useRegExp );
    ~CCustomFindReplaceDlg() override = default;

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnActivate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnWrapAroundClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL bHandled );
    LRESULT OnUseRegExpClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL bHandled );

    static void GetMsgProc( int code, WPARAM wParam, LPARAM lParam, HWND hParent );

public:
    [[nodiscard]] bool GetRegExpState() const;
    void SetRegExpState( bool newState );

    [[nodiscard]] bool GetWrapAroundSearchState() const;
    void SetWrapAroundSearchState( bool newState );

private:
    uint32_t hookId_ = 0;
    bool wrapAroundSearch_ = true;
    bool useRegExp_ = false;
};

template <typename T>
class CScintillaFindReplaceImpl
    : public CEditFindReplaceImplBase<T>
{
protected:
    using TFindReplaceDlg = CCustomFindReplaceDlg;
    using BaseClass = CEditFindReplaceImplBase<T>;
    using Range = std::pair<int, int>;

    BEGIN_MSG_MAP( CScintillaFindReplaceImpl )
        ALT_MSG_MAP( 1 )
        CHAIN_MSG_MAP_ALT( BaseClass, 1 )
    END_MSG_MAP()

public:
    CScintillaFindReplaceImpl( scintilla::CScriptEditorCtrl& sciEditor )
        : sciEditor_( sciEditor )
    {
    }

    [[nodiscard]] bool HasFindText() const
    {
        return !lastState_.findText.empty();
    }

    // CEditFindReplaceImplBase

    TFindReplaceDlg* CreateFindReplaceDialog( BOOL bFindOnly,
                                              LPCTSTR lpszFindWhat,
                                              LPCTSTR lpszReplaceWith = nullptr,
                                              DWORD dwFlags = FR_DOWN,
                                              HWND hWndParent = nullptr )
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
                // Need these in case called via F3, instead of dialog buttons
                lastState_.findText = smp::unicode::ToU8( findText );
                lastState_.replaceText = smp::unicode::ToU8( replaceText );

                findReplaceDialog->SetWrapAroundSearchState( lastState_.useWrapAround );
                findReplaceDialog->SetRegExpState( lastState_.useRegExp );
                findReplaceDialog->SetActiveWindow();
                findReplaceDialog->ShowWindow( SW_SHOW );
            }
        }

        return findReplaceDialog;
    }

    /// @brief Find method called via dialog
    bool FindTextSimple( LPCTSTR lpszFind, BOOL bMatchCase, BOOL bWholeWord, BOOL bFindDown /*= TRUE */ )
    {
        const bool hasChanged = UpdateFindReplaceState();
        if ( hasChanged )
        {
            ResetWrapAround();
        }

        return FindImpl( lastState_.findDirection );
    }

    /// @brief Find method called by hotkeys
    bool FindTextSimple( FindReplaceState::Direction direction )
    {
        const bool hasChanged = UpdateFindReplaceState();
        if ( hasChanged )
        {
            ResetWrapAround();
        }

        if ( !FindImpl( direction ) )
        {
            BaseClass::TextNotFound( smp::unicode::ToWide( lastState_.findText ).c_str() );
            return false;
        }
        else
        {
            if ( BaseClass::m_pFindReplaceDialog )
            {
                BaseClass::AdjustDialogPosition( *BaseClass::m_pFindReplaceDialog );
            }
            return true;
        }
    }

    void ReplaceSel( const CString& newText )
    {
        sciEditor_.ReplaceSelection( smp::unicode::ToU8( std::wstring_view{ static_cast<const wchar_t*>( newText ) } ).c_str() );
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

    [[nodiscard]] POINT PosFromChar( UINT nChar ) const
    {
        return POINT{
            sciEditor_.PointXFromPosition( nChar ),
            sciEditor_.PointYFromPosition( nChar )
        };
    }

private:
    bool FindImpl( FindReplaceState::Direction direction )
    {
        const Range selectionRange = { sciEditor_.GetSelectionStart(), sciEditor_.GetSelectionEnd() };
        if ( lastSearchPosition_ != selectionRange.first )
        { // user changed cursor position
            ResetWrapAround();
        }

        const int docLength = sciEditor_.GetLength();

        sciEditor_.SetSearchFlags( lastState_.ToScintillaFlags() );
        int pos = FindInRange( direction == FindReplaceState::Direction::down
                                   ? Range{ selectionRange.second, docLength }
                                   : Range{ selectionRange.first, 0 } );

        if ( lastState_.useWrapAround )
        {
            if ( pos >= 0 && !hasWrappedAround_ )
            { // continue as usual
                return ProcessFindResult( pos, lastState_.findText );
            }

            if ( pos == -1 && !hasWrappedAround_ )
            { // start search on the whole document
                pos = FindInRange( direction == FindReplaceState::Direction::down
                                       ? Range{ 0, docLength }
                                       : Range{ docLength, 0 } );
            }

            if ( pos == -1 )
            {
                if ( hasWrappedAround_ )
                { // we had found smth before, so it's not "not found" error
                    hasWrappedAround_ = false;
                    pos = selectionRange.first;
                    MessageOnWrapAroundFinish( lastState_.findText );
                }
            }
            else if ( direction == FindReplaceState::Direction::down && pos >= findStartPosition_
                      || ( direction == FindReplaceState::Direction::up ) && pos <= findStartPosition_ )
            {
                hasWrappedAround_ = false;
                MessageOnWrapAroundFinish( lastState_.findText );
            }
            else
            {
                hasWrappedAround_ = true;
            }
        }

        return ProcessFindResult( pos, lastState_.findText );
    }

    int FindInRange( const Range& searchRange )
    {
        ::SendMessage( hEdit_, SCI_SETTARGETRANGE, searchRange.first, searchRange.second );
        const int pos = sciEditor_.SearchInTarget( lastState_.findText.c_str(), lastState_.findText.length() );
        if ( pos >= 0 )
        {
            sciEditor_.SetSel( pos, pos + lastState_.findText.length() );
        }
        return pos;
    }

    bool ProcessFindResult( int pos, const std::u8string& which )
    {
        if ( pos != -1 )
        {
            lastSearchPosition_ = pos;
            sciEditor_.ScrollCaret();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool UpdateFindReplaceState()
    {
        auto pFindReplaceDialog = static_cast<TFindReplaceDlg*>( BaseClass::m_pFindReplaceDialog );
        if ( pFindReplaceDialog )
        {
            const auto prevState = lastState_;

            lastState_.FromFrFlags( pFindReplaceDialog->m_fr.Flags );
            const auto findCStr = pFindReplaceDialog->GetFindString();
            lastState_.findText = smp::unicode::ToU8( std::wstring_view{ findCStr ? findCStr : L"" } );
            if ( !isFindOnlyDialog_ )
            {
                const auto replaceCStr = pFindReplaceDialog->GetReplaceString();
                lastState_.replaceText = smp::unicode::ToU8( std::wstring_view{ replaceCStr ? replaceCStr : L"" } );
            }
            lastState_.useWrapAround = pFindReplaceDialog->GetWrapAroundSearchState();
            lastState_.useRegExp = pFindReplaceDialog->GetRegExpState();

            return ( prevState != lastState_ );
        }

        return false;
    }

    void ResetWrapAround()
    {
        hasWrappedAround_ = false;
        findStartPosition_ = sciEditor_.GetSelectionStart();
        lastSearchPosition_ = findStartPosition_;
    }

    void MessageOnWrapAroundFinish( const std::u8string& findText )
    {
        auto pFindReplaceDialog = static_cast<TFindReplaceDlg*>( BaseClass::m_pFindReplaceDialog );
        if ( pFindReplaceDialog )
        {
            pFindReplaceDialog->MessageBox( L"Find reached the starting point of the search",
                                            L"Find",
                                            MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
        }
        else
        {

            static_cast<T*>( this )->MessageBox( L"Find reached the starting point of the search",
                                                 L"Find",
                                                 MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
        }
    }

private:
    scintilla::CScriptEditorCtrl& sciEditor_;
    HWND hEdit_ = nullptr;

    bool isFindOnlyDialog_ = true;
    bool hasWrappedAround_ = false;
    int findStartPosition_ = -1;
    int lastSearchPosition_ = -1;
    static FindReplaceState lastState_;
};

template <typename T>
scintilla::FindReplaceState CScintillaFindReplaceImpl<T>::lastState_;

} // namespace scintilla
