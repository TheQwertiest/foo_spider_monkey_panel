// Extracted from SciTE
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
#pragma once

#include <panel/user_message.h>
#include <ui/scintilla/ui_sci_find_replace.h>
#include <ui/scintilla/ui_sci_goto.h>
#include <ui/scintilla/wtlscintilla.h>

#include <optional>
#include <set>
#include <span>

namespace smp::config::sci
{
struct ScintillaProp;
}

namespace smp::ui::sci
{

class CScriptEditorCtrl
    : public CScintillaCtrl
    , public CScintillaFindReplaceImpl<CScriptEditorCtrl>
    , public CScintillaGotoImpl
{
public:
    CScriptEditorCtrl();

    BEGIN_MSG_MAP( CScriptEditorCtrl )
        CHAIN_MSG_MAP( CScintillaGotoImpl )
        CHAIN_MSG_MAP_ALT( CScintillaFindReplaceImpl<CScriptEditorCtrl>, 1 )
        MESSAGE_HANDLER( WM_KEYDOWN, OnKeyDown )
        REFLECTED_NOTIFY_CODE_HANDLER_EX( SCN_UPDATEUI, OnUpdateUI )
        REFLECTED_NOTIFY_CODE_HANDLER_EX( SCN_CHARADDED, OnCharAdded )
        REFLECTED_NOTIFY_CODE_HANDLER_EX( SCN_ZOOM, OnZoom )
        REFLECTED_COMMAND_CODE_HANDLER_EX( SCEN_CHANGE, OnChange )
    END_MSG_MAP()

    LRESULT OnKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnUpdateUI( LPNMHDR pnmn );
    LRESULT OnCharAdded( LPNMHDR pnmh );
    LRESULT OnZoom( LPNMHDR pnmn );
    LRESULT OnChange( UINT uNotifyCode, int nID, HWND wndCtl );

    bool ProcessKey( uint32_t vk );

    void ReadAPI();
    void SetJScript();
    void SetContent( const char* text, bool clear_undo_buffer = false );
    void ReloadScintillaSettings();
    BOOL SubclassWindow( HWND hWnd );

private:
    enum class IndentationStatus
    {
        isNone,        // no effect on indentation
        isBlockStart,  // indentation block begin such as "{" or VB "function"
        isBlockEnd,    // indentation end indicator such as "}" or VB "end"
        isKeyWordStart // Keywords that cause indentation
    };

    struct KeyWordComparator
    {
        bool operator()( const qwr::u8string& a,
                         const qwr::u8string& b ) const;
    };

    struct BracePosition
    {
        std::optional<int> current;
        std::optional<int> matching;
    };

    struct StyledPart
    {
        StyledPart( qwr::u8string value, int style )
            : value( std::move( value ) )
            , style( style )
        {
        }
        qwr::u8string value;
        int style;
    };

private:
    // Operations and Implementation
    Sci_CharacterRange GetSelection();
    int GetCaretInLine();
    qwr::u8string GetCurrentLine();
    IndentationStatus GetIndentState( int line );
    std::vector<StyledPart> GetStyledParts( int line, std::span<const int> styles, size_t maxParts );
    bool RangeIsAllWhitespace( int start, int end );
    std::optional<DWORD> GetPropertyColor( const char* key );
    void Init();
    void RestoreDefaultStyle();
    void TrackWidth();
    void LoadStyleFromProperties();
    void AutoMarginWidth();
    bool StartCallTip();
    void ContinueCallTip();
    void FillFunctionDefinition( int pos = -1 );
    bool StartAutoComplete();
    int IndentOfBlock( int line );
    void AutomaticIndentation( char ch );
    BracePosition FindBraceMatchPos();
    std::optional<std::vector<qwr::u8string_view>> GetNearestWords( qwr::u8string_view wordPart, std::optional<char> separator = std::nullopt );
    std::optional<qwr::u8string_view> GetFullDefinitionForWord( qwr::u8string_view word );
    void SetIndentation( int line, int indent );
    std::optional<qwr::u8string> GetPropertyExpanded_Opt( const char* key );

private:
    int m_nBraceCount = 0;
    int m_nCurrentCallTip = 0;
    int m_nStartCalltipWord = 0;
    int m_nLastPosCallTip = 0;
    const int m_nStatementLookback = 10;

    qwr::u8string m_szCurrentCallTipWord;
    qwr::u8string m_szFunctionDefinition;

    std::set<qwr::u8string, KeyWordComparator> m_apis;
};

} // namespace smp::ui::sci
