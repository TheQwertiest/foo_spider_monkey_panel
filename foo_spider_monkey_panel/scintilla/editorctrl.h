// Stolen from SciTE
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
#pragma once

#include "wtlscintilla.h"
#include "user_message.h"

#include <nonstd/span.hpp>
#include <set>
#include <optional>

enum class IndentationStatus
{
    isNone,        // no effect on indentation
    isBlockStart,  // indentation block begin such as "{" or VB "function"
    isBlockEnd,    // indentation end indicator such as "}" or VB "end"
    isKeyWordStart // Keywords that cause indentation
};

struct StyleAndWords
{
    StyleAndWords() = default;
    StyleAndWords( const std::u8string& words, int styleNumber = 0 )
        : words( words )
        , styleNumber( styleNumber )
    {
    }

    int styleNumber{};
    std::u8string words;
    bool IsEmpty() const
    {
        return words.length() == 0;
    }
    bool IsSingleChar() const
    {
        return words.length() == 1;
    }
};

// forward declaration
struct t_sci_prop_set;

struct t_style_to_key
{
    int style_num;
    const char* key;
};

class CScriptEditorCtrl : public CScintillaCtrl
{
public:
    CScriptEditorCtrl() = default;

    // Message map and handlers
    BEGIN_MSG_MAP( CScriptEditorCtrl )
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

    void ReadAPI();
    void SetJScript();
    void SetContent( const char* text, bool clear_undo_buffer = false );
    void SetScintillaSettings();
    BOOL SubclassWindow( HWND hWnd );

private:
    struct KeyWordComparator
    {
        bool operator()( const std::u8string& a,
                         const std::u8string& b );
    };

    // Operations and Implementation
    Sci_CharacterRange GetSelection();
    int GetCaretInLine();
    std::u8string GetCurrentLine();
    IndentationStatus GetIndentState( int line );
    size_t GetLinePartsInStyle( int line, int style1, int style2, nonstd::span<std::u8string> parts );
    bool RangeIsAllWhitespace( int start, int end );
    std::optional<DWORD> GetPropertyColor( const char* key );
    void Init();
    void LoadProperties( const pfc::list_t<t_sci_prop_set>& data );
    void RestoreDefaultStyle();
    void TrackWidth();
    void SetAllStylesFromTable( nonstd::span<const t_style_to_key> table );
    void AutoMarginWidth();
    bool StartCallTip();
    void ContinueCallTip();
    void FillFunctionDefinition( int pos = -1 );
    bool StartAutoComplete();
    int IndentOfBlock( int line );
    void AutomaticIndentation( char ch );
    bool FindBraceMatchPos( int& braceAtCaret, int& braceOpposite );
    std::optional<std::vector<std::u8string_view>> GetNearestWords( std::u8string_view wordPart, std::optional<char8_t> separator = std::nullopt );
    std::optional<std::u8string_view> GetFullDefinitionForWord( std::u8string_view word );
    void SetIndentation( int line, int indent );
    std::optional<std::u8string> GetPropertyExpanded_Opt( const char8_t* key );

private:
    int m_nBraceCount = 0;
    int m_nCurrentCallTip = 0;
    int m_nStartCalltipWord = 0;
    int m_nLastPosCallTip = 0;
    int m_nStatementLookback = 10;

    StyleAndWords m_BlockStart{ "{", SCE_C_OPERATOR };
    StyleAndWords m_BlockEnd{ "}", SCE_C_OPERATOR };
    StyleAndWords m_StatementEnd{ ";", SCE_C_OPERATOR };
    StyleAndWords m_StatementIndent{ "case default do else for if while", SCE_C_WORD };

    std::u8string m_szCurrentCallTipWord;
    std::u8string m_szFunctionDefinition;

    std::set<std::u8string, KeyWordComparator> m_apis;
};
