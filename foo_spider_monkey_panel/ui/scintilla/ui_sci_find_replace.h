#pragma once

#include <resources/resource.h>

#include <Scintilla.h>

namespace smp::ui::sci
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

    qwr::u8string findText;
    qwr::u8string replaceText;
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
        COMMAND_HANDLER( IDC_CHECK_WRAPAROUND, BN_CLICKED, OnWrapAroundClick )
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
    void UpdateRegExpHacksState();

private:
    uint32_t hookId_ = 0;
    bool wrapAroundSearch_ = true;
    bool useRegExp_ = false;
};

// TODO: cleanup this mess
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
    CScintillaFindReplaceImpl( smp::ui::sci::CScriptEditorCtrl& sciEditor )
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
        if ( !findReplaceDialog )
        {
            ::MessageBeep( MB_ICONHAND );
        }
        else
        {
            auto& fr = findReplaceDialog->m_fr;
            fr.Flags |= FR_ENABLETEMPLATE;
            fr.lpTemplateName = ( bFindOnly ? MAKEINTRESOURCE( IDD_FINDDLGORD_WITH_REGEXP ) : MAKEINTRESOURCE( IDD_REPLACEDLGORD_WITH_REGEXP ) );
            fr.hInstance = ModuleHelper::GetModuleInstance();

            const std::wstring findText = ( lpszFindWhat && wcslen( lpszFindWhat ) ? lpszFindWhat : qwr::unicode::ToWide( lastState_.findText ) );
            const std::wstring replaceText = ( lpszReplaceWith && wcslen( lpszReplaceWith ) ? lpszReplaceWith : qwr::unicode::ToWide( lastState_.replaceText ) );

            HWND hWndFindReplace = findReplaceDialog->Create( bFindOnly,
                                                              findText.c_str(),
                                                              ( replaceText.empty() ? nullptr : replaceText.c_str() ),
                                                              lastState_.ToFrFlags( dwFlags ),
                                                              sciEditor_ );
            if ( !hWndFindReplace )
            {
                delete findReplaceDialog;
                findReplaceDialog = nullptr;
            }
            else
            {
                hEdit_ = sciEditor_;
                isFindOnlyDialog_ = bFindOnly;
                // Need these in case called via F3, instead of dialog buttons
                lastState_.findText = qwr::unicode::ToU8( findText );
                lastState_.replaceText = qwr::unicode::ToU8( replaceText );

                findReplaceDialog->SetWrapAroundSearchState( lastState_.useWrapAround );
                findReplaceDialog->SetRegExpState( lastState_.useRegExp );
                findReplaceDialog->SetActiveWindow();
                findReplaceDialog->ShowWindow( SW_SHOW );
            }
        }

        return findReplaceDialog;
    }

    // reimplement base method, because it doesn't work out for us
    void OnReplaceSel( LPCTSTR lpszFind, BOOL bFindDown, BOOL bMatchCase, BOOL bWholeWord, LPCTSTR lpszReplace )
    {
        BaseClass* pBase = static_cast<BaseClass*>( this );

        pBase->m_sFindNext = lpszFind;
        pBase->m_sReplaceWith = lpszReplace;
        pBase->m_bMatchCase = bMatchCase;
        pBase->m_bWholeWord = bWholeWord;
        pBase->m_bFindDown = bFindDown;

        const bool hasChanged = UpdateFindReplaceState();
        if ( hasChanged )
        {
            ResetWrapAround();
        }

        sciEditor_.SetTargetStart( sciEditor_.GetSelectionStart() );
        sciEditor_.SetTargetEnd( sciEditor_.GetSelectionEnd() );
        if ( -1 != FindInRange( Range{ sciEditor_.GetSelectionStart(), sciEditor_.GetSelectionEnd() }, false ) )
        {
            ReplaceSel( pBase->m_sReplaceWith );
        }

        if ( !FindImpl( lastState_.findDirection ) )
        {
            pBase->TextNotFound( pBase->m_sFindNext );
        }
        else
        {
            pBase->AdjustDialogPosition( pBase->m_pFindReplaceDialog->operator HWND() );
        }
    }

    // reimplement base method, because it doesn't work out for us
    void OnReplaceAll( LPCTSTR lpszFind, LPCTSTR lpszReplace, BOOL bMatchCase, BOOL bWholeWord )
    {
        BaseClass* pBase = static_cast<BaseClass*>( this );

        pBase->m_sFindNext = lpszFind;
        pBase->m_sReplaceWith = lpszReplace;
        pBase->m_bMatchCase = bMatchCase;
        pBase->m_bWholeWord = bWholeWord;
        pBase->m_bFindDown = TRUE;

        UpdateFindReplaceState();
        sciEditor_.SetSearchFlags( lastState_.ToScintillaFlags() );

        const auto docLength = sciEditor_.GetLength();

        if ( -1 == FindInRange( Range{ 0, docLength }, false ) )
        {
            pBase->TextNotFound( qwr::unicode::ToWide( lastState_.findText ).c_str() );
            return;
        }

        pBase->OnReplaceAllCoreBegin();
        sciEditor_.BeginUndoAction();

        int curStartPos = 0;
        size_t replaceCount = 0;
        do
        {
            const auto pos = FindInRange( Range{ curStartPos, docLength }, false );
            if ( pos == -1 )
            {
                break;
            }
            const auto targetLen = sciEditor_.GetTargetEnd() - pos; // because might be a regexp

            sciEditor_.SetTargetStart( pos );
            sciEditor_.SetTargetEnd( pos + targetLen );
            const auto replaceLen =
                lastState_.useRegExp
                    ? sciEditor_.ReplaceTargetRE( lastState_.replaceText.c_str(), lastState_.replaceText.length() )
                    : sciEditor_.ReplaceTarget( lastState_.replaceText.c_str(), lastState_.replaceText.length() );
            ++replaceCount;
            curStartPos = pos + replaceLen;
        } while ( true );

        sciEditor_.EndUndoAction();
        pBase->OnReplaceAllCoreEnd( replaceCount );
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

        if ( lastState_.useRegExp && direction == FindReplaceState::Direction::up )
        { // Scintilla bug
            GenerateMessageBox( L"Up direction search is disabled when RegExp is enabled",
                                L"Find",
                                MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
            return false;
        }

        if ( !FindImpl( direction ) )
        {
            BaseClass::TextNotFound( qwr::unicode::ToWide( lastState_.findText ).c_str() );
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
        const Range prevSelectionRange = { sciEditor_.GetSelectionStart(), sciEditor_.GetSelectionEnd() };

        sciEditor_.SetTargetStart( prevSelectionRange.first );
        sciEditor_.SetTargetEnd( prevSelectionRange.second );
        const auto replaceLen =
            lastState_.useRegExp
                ? sciEditor_.ReplaceTargetRE( lastState_.replaceText.c_str(), lastState_.replaceText.length() )
                : sciEditor_.ReplaceTarget( lastState_.replaceText.c_str(), lastState_.replaceText.length() );

        sciEditor_.SetSelectionStart( prevSelectionRange.first );
        sciEditor_.SetSelectionEnd( prevSelectionRange.first + replaceLen );
        lastSearchPosition_ = prevSelectionRange.first + replaceLen;
        if ( findStartPosition_ > prevSelectionRange.first )
        {
            findStartPosition_ += replaceLen - ( prevSelectionRange.second - prevSelectionRange.first );
        }
    }

    LONG GetSelText( CString& strText ) const
    {
        const size_t textSize = static_cast<size_t>( sciEditor_.GetSelectedText( nullptr ) );

        qwr::u8string text;
        text.resize( textSize + 1 );
        int iRet = sciEditor_.GetSelectedText( text.data() );
        text.resize( strlen( text.c_str() ) );

        strText = qwr::unicode::ToWide( text ).c_str();

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
        // Scintilla bug
        assert( !lastState_.useRegExp || direction == FindReplaceState::Direction::down );

        const Range selectionRange = { sciEditor_.GetSelectionStart(), sciEditor_.GetSelectionEnd() };
        if ( lastSearchPosition_ != selectionRange.first )
        { // user changed cursor position
            ResetWrapAround();
        }

        const int docLength = sciEditor_.GetLength();

        sciEditor_.SetSearchFlags( lastState_.ToScintillaFlags() );
        int pos = FindInRange( direction == FindReplaceState::Direction::down
                                   ? Range{ selectionRange.second, docLength }
                                   : Range{ selectionRange.first, 0 },
                               true );

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
                                       : Range{ docLength, 0 },
                                   true );
            }

            if ( pos == -1 )
            {
                if ( hasWrappedAround_ )
                { // we had found smth before, so it's not "not found" error
                    hasWrappedAround_ = false;
                    pos = selectionRange.first;
                    MessageOnWrapAroundFinish();
                }
            }
            else if ( direction == FindReplaceState::Direction::down && pos >= findStartPosition_
                      || ( direction == FindReplaceState::Direction::up ) && pos <= findStartPosition_ )
            {
                hasWrappedAround_ = false;
                MessageOnWrapAroundFinish();
            }
            else
            {
                hasWrappedAround_ = true;
            }
        }

        return ProcessFindResult( pos, lastState_.findText );
    }

    int FindInRange( const Range& searchRange, bool updateSelection )
    {
        sciEditor_.SetTargetStart( searchRange.first );
        sciEditor_.SetTargetEnd( searchRange.second );
        const int pos = sciEditor_.SearchInTarget( lastState_.findText.c_str(), lastState_.findText.length() );
        if ( updateSelection && pos >= 0 )
        {
            // length calculated via target, because might be a regexp
            sciEditor_.SetSel( pos, sciEditor_.GetTargetEnd() );
        }
        return pos;
    }

    bool ProcessFindResult( int pos, const qwr::u8string& which )
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
            lastState_.findText = qwr::unicode::ToU8( std::wstring_view{ findCStr ? findCStr : L"" } );
            if ( !isFindOnlyDialog_ )
            {
                const auto replaceCStr = pFindReplaceDialog->GetReplaceString();
                lastState_.replaceText = qwr::unicode::ToU8( std::wstring_view{ replaceCStr ? replaceCStr : L"" } );
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

    void GenerateMessageBox( LPCTSTR lpszText,
                             LPCTSTR lpszCaption = _T(""),
                             UINT nType = MB_OK )
    {
        auto pFindReplaceDialog = static_cast<TFindReplaceDlg*>( BaseClass::m_pFindReplaceDialog );
        if ( pFindReplaceDialog )
        {
            pFindReplaceDialog->MessageBox( lpszText,
                                            lpszCaption,
                                            nType );
        }
        else
        {
            static_cast<T*>( this )->MessageBox( lpszText,
                                                 lpszCaption,
                                                 nType );
        }
    }

    void MessageOnWrapAroundFinish()
    {
        GenerateMessageBox( L"Find reached the starting point of the search",
                            L"Find",
                            MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
    }

private:
    smp::ui::sci::CScriptEditorCtrl& sciEditor_;
    HWND hEdit_ = nullptr;

    bool isFindOnlyDialog_ = true;
    bool hasWrappedAround_ = false;
    int findStartPosition_ = -1;
    int lastSearchPosition_ = -1;
    static FindReplaceState lastState_;
};

template <typename T>
smp::ui::sci::FindReplaceState CScintillaFindReplaceImpl<T>::lastState_;

} // namespace smp::ui::sci
