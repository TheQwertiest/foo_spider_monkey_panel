#include <stdafx.h>

#include "ui_sci_editor.h"

#include <ui/scintilla/sci_config.h>
#include <ui/scintilla/sci_prop_sets.h>
#include <ui/scintilla/ui_sci_goto.h>
#include <utils/colour_helpers.h>
#include <utils/logging.h>
#include <utils/resource_helpers.h>

#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/string_helpers.h>

#include <charconv>
#include <optional>

using namespace smp::ui::sci;

namespace
{

enum ScintillaStyleFlags
{
    ESF_NONE = 0,
    ESF_FONT = 1 << 0,
    ESF_SIZE = 1 << 1,
    ESF_FORE = 1 << 2,
    ESF_BACK = 1 << 3,
    ESF_BOLD = 1 << 4,
    ESF_ITALICS = 1 << 5,
    ESF_UNDERLINED = 1 << 6,
    ESF_CASEFORCE = 1 << 7,
};

struct ScintillaStyle
{
    unsigned flags{};
    bool italics{};
    bool bold{};
    bool underlined{};
    qwr::u8string font;
    unsigned size{};
    DWORD fore{};
    DWORD back{};
    int case_force{};
};

struct StyleToPropname
{
    int style_num;
    const char* propname;
};

constexpr std::array<StyleToPropname, 16> js_style_table = {
    { // Default
      { STYLE_DEFAULT, "style.default" },
      // Line number
      { STYLE_LINENUMBER, "style.linenumber" },
      // Bracelight
      { STYLE_BRACELIGHT, "style.bracelight" },
      // Bracebad
      { STYLE_BRACEBAD, "style.bracebad" },
      // Comments
      { SCE_C_COMMENT, "style.comment" },
      { SCE_C_COMMENTLINE, "style.comment" },
      { SCE_C_COMMENTDOC, "style.comment" },
      { SCE_C_COMMENTLINEDOC, "style.comment" },
      { SCE_C_COMMENTDOCKEYWORD, "style.comment" },
      { SCE_C_COMMENTDOCKEYWORDERROR, "style.comment" },
      // Keywords
      { SCE_C_WORD, "style.keyword" },
      // Indentifier
      { SCE_C_IDENTIFIER, "style.indentifier" },
      // Numbers
      { SCE_C_NUMBER, "style.number" },
      // String/Chars
      { SCE_C_STRING, "style.string" },
      { SCE_C_CHARACTER, "style.string" },
      // Operators
      { SCE_C_OPERATOR, "style.operator" } }
};

bool IsBraceChar( int ch )
{
    return ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '{' || ch == '}';
}

bool IsCSym( int ch )
{
    return __iswcsym( static_cast<wint_t>( ch ) );
}

DWORD GetColourFromHex( qwr::u8string_view hex )
{
    // Value format: #XXXXXX
    if ( hex.size() != 7 )
    {
        return 0;
    }

    const qwr::u8string_view hexView{ hex.substr( 1 ) }; // skip '#'

    const auto colour = qwr::string::GetNumber<uint32_t>( hexView, 16 ).value_or( 0 );

    return smp::colour::ArgbToColorref( colour );
}

ScintillaStyle ParseStyle( qwr::u8string_view p_definition )
{
    ScintillaStyle p_style;

    const auto attributes = qwr::string::Split( p_definition, ',' );
    for ( const auto& attribute: attributes )
    {
        if ( attribute.empty() )
        {
            continue;
        }

        const auto values = qwr::string::Split( attribute, ':' );
        if ( values.empty() || values[0].empty() )
        {
            continue;
        }

        const auto& name = values[0];
        if ( name == "italics" )
        {
            p_style.flags |= ESF_ITALICS;
            p_style.italics = true;
        }
        else if ( name == "notitalics" )
        {
            p_style.flags |= ESF_ITALICS;
            p_style.italics = false;
        }
        else if ( name == "bold" )
        {
            p_style.flags |= ESF_BOLD;
            p_style.bold = true;
        }
        else if ( name == "notbold" )
        {
            p_style.flags |= ESF_BOLD;
            p_style.bold = false;
        }
        else if ( name == "font" )
        {
            if ( values.size() >= 2 )
            {
                p_style.flags |= ESF_FONT;
                p_style.font.assign( values[1].data(), values[1].size() );
            }
        }
        else if ( name == "fore" )
        {
            if ( values.size() >= 2 )
            {
                p_style.flags |= ESF_FORE;
                p_style.fore = GetColourFromHex( values[1] );
            }
        }
        else if ( name == "back" )
        {
            if ( values.size() >= 2 )
            {
                p_style.flags |= ESF_BACK;
                p_style.back = GetColourFromHex( values[1] );
            }
        }
        else if ( name == "size" )
        {
            if ( values.size() >= 2 )
            {
                auto intRet = qwr::string::GetNumber<unsigned>( values[1] );
                if ( intRet )
                {
                    p_style.flags |= ESF_SIZE;
                    p_style.size = *intRet;
                }
            }
        }
        else if ( name == "underlined" )
        {
            p_style.flags |= ESF_UNDERLINED;
            p_style.underlined = true;
        }
        else if ( name == "notunderlined" )
        {
            p_style.flags |= ESF_UNDERLINED;
            p_style.underlined = false;
        }
        else if ( name == "case" )
        {
            p_style.flags |= ESF_CASEFORCE;
            p_style.case_force = SC_CASE_MIXED;

            if ( values.size() >= 2 && !values[1].empty() )
            {
                const char ch = values[1][0];
                if ( ch == 'u' )
                {
                    p_style.case_force = SC_CASE_UPPER;
                }
                else if ( ch == 'l' )
                {
                    p_style.case_force = SC_CASE_LOWER;
                }
            }
        }
    }

    return p_style;
}

template <typename T>
qwr::u8string JoinWithSpace( const T& cont )
{
    static_assert( std::is_same_v<typename T::value_type, qwr::u8string> || std::is_same_v<typename T::value_type, qwr::u8string_view> || std::is_same_v<typename T::value_type, const char*> );

    qwr::u8string words_str;
    words_str.reserve( cont.size() * 6 );
    for ( const auto& word: cont )
    {
        if constexpr ( std::is_constructible_v<qwr::u8string, typename T::value_type> )
        {
            words_str += word;
        }
        else
        {
            words_str += qwr::u8string{ word.data(), word.size() };
        }
        words_str += ' ';
    }
    if ( !words_str.empty() )
    {
        words_str.resize( words_str.size() - 1 );
    }
    return words_str;
}

bool StartsWith_CaseInsensitive( const qwr::u8string_view& a, const qwr::u8string_view& b )
{
    return ( ( a.size() >= b.size() ) && !pfc::stricmp_ascii_ex( a.data(), b.size(), b.data(), b.size() ) );
}

} // namespace

namespace smp::ui::sci
{

bool CScriptEditorCtrl::KeyWordComparator::operator()( const qwr::u8string& a, const qwr::u8string& b ) const
{
    const int result = _stricmp( a.c_str(), b.c_str() );
    if ( !result && !a.empty() && !b.empty() && std::isalpha( a[0] ) && ( a[0] != b[0] ) )
    {
        return std::isupper( static_cast<unsigned char>( a[0] ) );
    }
    else
    {
        return result < 0;
    }
}

CScriptEditorCtrl::CScriptEditorCtrl()
    : CScintillaFindReplaceImpl<CScriptEditorCtrl>( *this )
    , CScintillaGotoImpl( *this )
{
}

LRESULT CScriptEditorCtrl::OnKeyDown( UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // Pass the message to the parent window to handle all shortcuts (it will call us back)
    bHandled = FALSE;
    GetParent().PostMessage( static_cast<UINT>( smp::MiscMessage::key_down ), wParam, lParam );
    return 1;
}

LRESULT CScriptEditorCtrl::OnUpdateUI( LPNMHDR )
{
    const auto bracePos = FindBraceMatchPos();
    if ( bracePos.current )
    {
        if ( bracePos.matching )
        {
            const auto currentBrace = *bracePos.current;
            const auto matchingBrace = *bracePos.matching;

            BraceHighlight( currentBrace, matchingBrace );
            SetHighlightGuide( std::min( GetColumn( currentBrace ), GetColumn( matchingBrace ) ) );
        }
        else
        {
            BraceBadLight( *bracePos.current );
            SetHighlightGuide( 0 );
        }
    }
    else
    {
        BraceHighlight( -1, -1 );
        SetHighlightGuide( 0 );
    }

    return 0;
}

LRESULT CScriptEditorCtrl::OnCharAdded( LPNMHDR pnmh )
{
    auto* notification = reinterpret_cast<SCNotification*>( pnmh );
    int ch = notification->ch;
    Sci_CharacterRange crange = GetSelection();
    int selStart = crange.cpMin;
    int selEnd = crange.cpMax;

    if ( ( selEnd == selStart ) && ( selStart > 0 ) )
    {
        if ( CallTipActive() )
        {
            switch ( ch )
            {
            case ')':
                m_nBraceCount--;
                if ( m_nBraceCount < 1 )
                {
                    CallTipCancel();
                }
                else
                {
                    StartCallTip();
                }
                break;

            case '(':
                m_nBraceCount++;
                StartCallTip();
                break;

            default:
                ContinueCallTip();
                break;
            }
        }
        else if ( AutoCActive() )
        {
            if ( ch == '(' )
            {
                m_nBraceCount++;
                StartCallTip();
            }
            else if ( ch == ')' )
            {
                m_nBraceCount--;
            }
            else if ( !IsCSym( ch ) )
            {
                AutoCCancel();

                if ( ch == '.' )
                {
                    StartAutoComplete();
                }
            }
        }
        else
        {
            if ( ch == '(' )
            {
                m_nBraceCount = 1;
                StartCallTip();
            }
            else
            {
                AutomaticIndentation( ch );

                if ( IsCSym( ch ) || ch == '.' )
                {
                    StartAutoComplete();
                }
            }
        }
    }

    return 0;
}

LRESULT CScriptEditorCtrl::OnZoom( LPNMHDR )
{
    AutoMarginWidth();

    return 0;
}

LRESULT CScriptEditorCtrl::OnChange( UINT, int, HWND )
{
    AutoMarginWidth();

    return 0;
}

bool CScriptEditorCtrl::ProcessKey( uint32_t vk )
{
    const int modifiers = ( IsKeyPressed( VK_SHIFT ) ? SCMOD_SHIFT : 0 )
                          | ( IsKeyPressed( VK_CONTROL ) ? SCMOD_CTRL : 0 )
                          | ( IsKeyPressed( VK_MENU ) ? SCMOD_ALT : 0 );

    // Hotkeys
    if ( modifiers == SCMOD_CTRL )
    {
        switch ( vk )
        {
        case 'F':
        {
            FindReplace( TRUE );
            return true;
        }
        case 'H':
        {
            FindReplace( FALSE );
            return true;
        }
        case 'G':
        {
            ShowGoTo();
            return true;
        }
        default:
            break;
        }
    }
    else if ( modifiers == 0 )
    {
        if ( vk == VK_F3 )
        {
            if ( HasFindText() )
            {
                FindTextSimple( FindReplaceState::Direction::down );
            }
            else
            {
                FindReplace( TRUE );
            }
            return true;
        }
    }
    else if ( modifiers == SCMOD_SHIFT )
    {
        if ( vk == VK_F3 )
        {
            if ( HasFindText() )
            {
                FindTextSimple( FindReplaceState::Direction::up );
            }
            else
            {
                FindReplace( TRUE );
            }
            return true;
        }
    }

    return false;
}

void CScriptEditorCtrl::ReadAPI()
{
    namespace fs = std::filesystem;

    m_apis.clear();

    const auto readApi = [&m_apis = m_apis]( const auto& content ) {
        for ( const auto& line: qwr::string::SplitByLines( content ) )
        {
            if ( !line.empty() && IsCSym( line[0] ) )
            {
                m_apis.emplace( line.data(), line.size() );
            }
        }
    };

    readApi( LoadStringResource( IDR_SCINTILLA_JS_API, "Script" ).value_or( "" ) );
    readApi( LoadStringResource( IDR_SCINTILLA_INTERFACE_API, "Script" ).value_or( "" ) );

    const auto propvalRet = GetPropertyExpanded_Opt( "api.extra" );
    if ( !propvalRet || propvalRet->empty() )
    {
        return;
    }

    const auto files = qwr::string::Split<char>( *propvalRet, ';' );
    qwr::u8string content;
    for ( const auto& file: files )
    {
        try
        {
            const auto content = qwr::file::ReadFile( fs::u8path( qwr::u8string{ file.data(), file.size() } ), CP_UTF8 );
            readApi( content );
        }
        catch ( const qwr::QwrException& e )
        {
            smp::utils::LogWarning( fmt::format(
                "Could not load editor API file {}:\n"
                "  {}",
                qwr::u8string{ file.data(), file.size() },
                e.what() ) );
        }
    }
}

void CScriptEditorCtrl::SetContent( const char* text, bool clear_undo_buffer )
{
    SetText( text );
    ConvertEOLs( SC_EOL_CRLF );

    if ( clear_undo_buffer )
    {
        EmptyUndoBuffer();
    }

    Colourise( 0, std::numeric_limits<unsigned int>::max() );
    GrabFocus();
    TrackWidth();
}

void CScriptEditorCtrl::SetJScript()
{
    // clang-format off
    // protect array value format style

	// source: https://www.w3schools.com/js/js_reserved.asp
    constexpr std::array js_words{
		"abstract", "arguments", "await", "boolean",
		"break", "byte", "case", "catch",
		"char", "class", "const", "continue",
		"debugger", "default", "delete", "do",
		"double", "else", "enum", "eval",
		"export", "extends", "false", "final",
		"finally", "float", "for", "function",
		"goto", "if", "implements", "import",
		"in", "instanceof", "int", "interface",
		"let", "long", "native", "new",
		"null", "package", "private", "protected",
		"public", "return", "short", "static",
		"super", "switch", "synchronized", "this",
		"throw", "throws", "transient", "true",
		"try", "typeof", "var", "void",
		"volatile", "while", "with", "yield"
    };

	constexpr std::array js_builtins{
		"Array", "Date", "eval", "function",
		"hasOwnProperty", "Infinity", "isFinite", "isNaN",
		"isPrototypeOf", "length", "Math", "NaN",
		"name", "Number", "Object", "prototype",
		"String", "toString", "undefined", "valueOf"
	};

	constexpr std::array smp_words{
		"window", "fb", "gdi", "utils", "plman", "console"
	};
    // clang-format on

    std::string keywords_str;
    keywords_str.reserve( ( js_words.size() + js_builtins.size() + smp_words.size() ) * 6 );
    keywords_str += JoinWithSpace( js_words ) + ' ';
    keywords_str += JoinWithSpace( js_builtins ) + ' ';
    keywords_str += JoinWithSpace( smp_words );

    RestoreDefaultStyle();

    SetILexer( CreateLexer( "cpp" ) );
    SetKeyWords( 0, keywords_str.c_str() );
    LoadStyleFromProperties();
    Colourise( 0, std::numeric_limits<unsigned int>::max() );
}

void CScriptEditorCtrl::ReloadScintillaSettings()
{
    for ( const auto& prop: config::sci::props.val() )
    {
        SetProperty( prop.key.c_str(), prop.val.c_str() );
    }

    auto getIntFromProp = [&]( const std::string& propName ) -> std::optional<int> {
        const auto propvalRet = GetPropertyExpanded_Opt( propName.c_str() );
        if ( !propvalRet )
        {
            return std::nullopt;
        }

        return qwr::string::GetNumber<int>( static_cast<std::string_view>( *propvalRet ) );
    };

    auto retVal = getIntFromProp( "style.wrap.mode" );
    if ( retVal )
    {
        SetWrapMode( *retVal );
    }

    retVal = getIntFromProp( "style.wrap.visualflags" );
    if ( retVal )
    {
        SetWrapVisualFlags( *retVal );
    }

    retVal = getIntFromProp( "style.wrap.visualflags.location" );
    if ( retVal )
    {
        SetWrapVisualFlagsLocation( *retVal );
    }

    retVal = getIntFromProp( "style.wrap.indentmode" );
    if ( retVal )
    {
        SetWrapIndentMode( *retVal );
    }
}

BOOL CScriptEditorCtrl::SubclassWindow( HWND hWnd )
{
    BOOL bRet = CScintillaCtrl::SubclassWindow( hWnd );

    if ( bRet )
    {
        Init();
    }

    return bRet;
}

Sci_CharacterRange CScriptEditorCtrl::GetSelection()
{
    return Sci_CharacterRange{ static_cast<Sci_PositionCR>( GetSelectionStart() ), static_cast<Sci_PositionCR>( GetSelectionEnd() ) };
}

int CScriptEditorCtrl::GetCaretInLine()
{
    const int caret = GetCurrentPos();
    const int line = LineFromPosition( caret );
    const int lineStart = PositionFromLine( line );
    return caret - lineStart;
}

qwr::u8string CScriptEditorCtrl::GetCurrentLine()
{
    qwr::u8string buf;

    buf.resize( GetCurLine( nullptr, 0 ) + 1 );
    GetCurLine( buf.data(), buf.size() );
    buf.resize( strlen( buf.c_str() ) );

    return buf;
}

CScriptEditorCtrl::IndentationStatus CScriptEditorCtrl::GetIndentState( int line )
{ // C like language indentation defined by braces and keywords
    constexpr int styles[] = { SCE_C_OPERATOR, SCE_C_WORD };

    const auto styledParts = GetStyledParts( line, styles, 20 );
    if ( styledParts.empty() )
    {
        return IndentationStatus::isNone;
    }

    const auto reverseParts = ranges::views::reverse( styledParts );
    const auto itBraces = ranges::find_if( reverseParts, []( const auto& elem ) {
        const auto& [part, style] = elem;
        return ( style == SCE_C_OPERATOR && ( part == "{" || part == "}" ) );
    } );
    if ( itBraces != ranges::cend( reverseParts ) )
    { // braces have priority
        const auto& [part, style] = *itBraces;
        if ( part == "{" )
        {
            return IndentationStatus::isBlockStart;
        }
        else if ( part == "}" )
        {
            return IndentationStatus::isBlockEnd;
        }
    }
    else
    {
        const auto& [part, style] = reverseParts[0];
        if ( style == SCE_C_OPERATOR && part == ";" )
        {
            return IndentationStatus::isNone;
        }
        else
        {
            constexpr const char* keywords[]{ "case",
                                              "default",
                                              "do",
                                              "else",
                                              "for",
                                              "if",
                                              "while" };
            if ( ranges::find( keywords, part ) != ranges::cend( keywords ) )
            {
                return IndentationStatus::isKeyWordStart;
            }
        }
    }

    return IndentationStatus::isNone;
}

std::vector<CScriptEditorCtrl::StyledPart> CScriptEditorCtrl::GetStyledParts( int line, std::span<const int> styles, size_t maxParts )
{
    std::vector<StyledPart> parts;
    parts.reserve( maxParts );

    qwr::u8string curPart;
    const int thisLineStart = PositionFromLine( line );
    const int nextLineStart = PositionFromLine( line + 1 );
    int lastStyle = 0;

    for ( int pos = thisLineStart; pos < nextLineStart; ++pos )
    {
        const auto curStyle = GetStyleAt( pos );
        if ( ranges::find( styles, curStyle ) != styles.end() )
        {
            if ( curStyle != lastStyle && !curPart.empty() )
            {
                parts.emplace_back( curPart, lastStyle );
                curPart.clear();
            }
            lastStyle = curStyle;

            if ( curStyle == SCE_C_OPERATOR )
            {
                curPart = GetCharAt( pos );
                parts.emplace_back( curPart, curStyle );
                curPart.clear();
            }
            else
            {
                curPart += GetCharAt( pos );
            }
        }

        if ( parts.size() == maxParts )
        {
            curPart.clear();
            break;
        }
    }

    if ( parts.size() != maxParts && !curPart.empty() )
    {
        parts.emplace_back( curPart, lastStyle );
    }

    return parts;
}

bool CScriptEditorCtrl::RangeIsAllWhitespace( int start, int end )
{
    start = std::max( 0, start );
    end = std::min( end, GetLength() );

    for ( int i = start; i < end; ++i )
    {
        const char c = GetCharAt( i );
        if ( ( c != ' ' ) && ( c != '\t' ) )
        {
            return false;
        }
    }

    return true;
}

bool CScriptEditorCtrl::StartCallTip()
{
    m_nCurrentCallTip = 0;
    m_szCurrentCallTipWord.clear();
    qwr::u8string line = GetCurrentLine();
    int current = GetCaretInLine();
    int pos = GetCurrentPos();
    int braces = 0;

    do
    {
        while ( current > 0 && ( braces || line[current - 1] != '(' ) )
        {
            if ( line[current - 1] == '(' )
            {
                braces--;
            }
            else if ( line[current - 1] == ')' )
            {
                braces++;
            }

            current--;
            pos--;
        }

        if ( current > 0 )
        {
            current--;
            pos--;
        }
        else
        {
            break;
        }

        while ( current > 0 && std::isspace( static_cast<unsigned char>( line[current - 1] ) ) )
        {
            current--;
            pos--;
        }
    } while ( current > 0 && !IsCSym( line[current - 1] ) );

    if ( current <= 0 )
    {
        return true;
    }

    m_nStartCalltipWord = current - 1;

    while ( m_nStartCalltipWord > 0 && ( IsCSym( line[m_nStartCalltipWord - 1] ) || ( line[m_nStartCalltipWord - 1] == '.' ) ) )
    {
        --m_nStartCalltipWord;
    }

    line.resize( current );
    m_szCurrentCallTipWord = line.c_str() + m_nStartCalltipWord;
    FillFunctionDefinition( pos );
    return true;
}

void CScriptEditorCtrl::ContinueCallTip()
{
    const qwr::u8string line = GetCurrentLine();
    int current = GetCaretInLine();
    int braces = 0;
    int commas = 0;

    for ( int i = m_nStartCalltipWord; i < current; ++i )
    {
        if ( line[i] == '(' )
        {
            braces++;
        }
        else if ( line[i] == ')' && braces )
        {
            braces--;
        }
        else if ( braces == 1 && line[i] == ',' )
        {
            commas++;
        }
    }

    int startHighlight = 0;

    while ( m_szFunctionDefinition[startHighlight] && m_szFunctionDefinition[startHighlight] != '(' )
    {
        startHighlight++;
    }

    if ( m_szFunctionDefinition[startHighlight] == '(' )
    {
        startHighlight++;
    }

    while ( m_szFunctionDefinition[startHighlight] && commas )
    {
        if ( m_szFunctionDefinition[startHighlight] == ',' )
        {
            commas--;
        }
        // If it reached the end of the argument list it means that the user typed in more
        // arguments than the ones listed in the calltip
        if ( m_szFunctionDefinition[startHighlight] == ')' )
        {
            commas = 0;
        }
        else
        {
            startHighlight++;
        }
    }

    if ( m_szFunctionDefinition[startHighlight] == ',' )
    {
        startHighlight++;
    }

    int endHighlight = startHighlight;

    while ( m_szFunctionDefinition[endHighlight] && m_szFunctionDefinition[endHighlight] != ',' && m_szFunctionDefinition[endHighlight] != ')' )
    {
        endHighlight++;
    }

    CallTipSetHlt( startHighlight, endHighlight );
}

void CScriptEditorCtrl::FillFunctionDefinition( int pos )
{
    m_szFunctionDefinition.clear();

    if ( pos )
    {
        m_nLastPosCallTip = pos;
    }

    if ( m_apis.empty() )
    {
        return;
    }

    auto definitionRet = GetFullDefinitionForWord( m_szCurrentCallTipWord );
    if ( !definitionRet )
    {
        return;
    }

    m_szFunctionDefinition = qwr::u8string{ definitionRet->data(), definitionRet->size() };

    CallTipShow( m_nLastPosCallTip - m_szCurrentCallTipWord.length(), m_szFunctionDefinition.c_str() );
    ContinueCallTip();
}

bool CScriptEditorCtrl::StartAutoComplete()
{
    if ( m_apis.empty() )
    {
        return false;
    }

    const qwr::u8string line = GetCurrentLine();
    const auto curPos = static_cast<size_t>( GetCaretInLine() );

    const qwr::u8string_view word = [&line, curPos] {
        qwr::u8string_view wordBuffer{ line.c_str(), curPos };

        const auto it = std::find_if( wordBuffer.crbegin(), wordBuffer.crend(), []( char ch ) {
            return ( !IsCSym( ch ) && ch != '.' );
        } );
        if ( it != wordBuffer.crend() )
        {
            wordBuffer.remove_prefix( std::distance( it, wordBuffer.crend() ) );
        }

        return wordBuffer;
    }();

    const auto acWordsRet = GetNearestWords( word, '(' );
    if ( !acWordsRet )
    {
        return false;
    }

    const auto& acWordsStr = JoinWithSpace( *acWordsRet );
    AutoCShow( word.length(), acWordsStr.c_str() );

    return true;
}

int CScriptEditorCtrl::IndentOfBlock( int line )
{
    if ( line < 0 )
    {
        return 0;
    }

    int indentSize = GetIndent();
    int indentBlock = GetLineIndentation( line );
    int backLine = line;
    IndentationStatus indentState = IndentationStatus::isNone;

    const int lineLimit = std::max( 0, line - m_nStatementLookback );

    while ( ( backLine >= lineLimit ) && ( indentState == IndentationStatus::isNone ) )
    {
        indentState = GetIndentState( backLine );
        if ( IndentationStatus::isNone != indentState )
        {
            indentBlock = GetLineIndentation( backLine );
        }

        switch ( indentState )
        {
        case IndentationStatus::isBlockStart:
        {
            indentBlock += indentSize;
            break;
        }
        case IndentationStatus::isBlockEnd:
        {
            indentBlock = std::max( 0, indentBlock );
            break;
        }
        case IndentationStatus::isKeyWordStart:
        {
            if ( backLine == line )
            {
                indentBlock += indentSize;
            }
            break;
        }
        default:
        {
            break;
        }
        }

        backLine--;
    }

    return indentBlock;
}

void CScriptEditorCtrl::AutomaticIndentation( char ch )
{
    const Sci_CharacterRange crange = GetSelection();
    const int selStart = crange.cpMin;
    const int curLine = LineFromPosition( GetCurrentPos() );
    const int thisLineStart = PositionFromLine( curLine );
    const int indentSize = GetIndent();
    const int indentBlock = IndentOfBlock( curLine - 1 );

    if ( ch == '}' )
    {
        // Dedent maybe
        if ( RangeIsAllWhitespace( thisLineStart, selStart - 1 ) )
        {
            SetIndentation( curLine, indentBlock - indentSize );
        }
    }
    else if ( ch == '{' )
    {
        // Dedent maybe if first on line and previous line was starting keyword
        if ( ( GetIndentState( curLine - 1 ) == IndentationStatus::isKeyWordStart ) )
        {
            if ( RangeIsAllWhitespace( thisLineStart, selStart - 1 ) )
            {
                SetIndentation( curLine, indentBlock - indentSize );
            }
        }
    }
    else if ( ( ch == '\r' || ch == '\n' ) && ( selStart == thisLineStart ) )
    {
        if ( ( GetIndentState( curLine - 1 ) == IndentationStatus::isKeyWordStart ) )
        {
            const auto line = GetCurrentLine();
            const auto it = ranges::find_if( line, []( const auto& ch ) { return !std::isspace( static_cast<unsigned char>( ch ) ); } );
            if ( it != line.cend() && *it == '{' )
            { // dedent open brace on keyword
                SetIndentation( curLine, indentBlock - indentSize );
            }
            else
            {
                SetIndentation( curLine, indentBlock );
            }
        }
        else
        {
            SetIndentation( curLine, indentBlock );
        }
    }
}

CScriptEditorCtrl::BracePosition CScriptEditorCtrl::FindBraceMatchPos()
{
    const int caretPos = GetCurrentPos();
    int braceAtCaret = -1;
    int braceOpposite = -1;
    char charBefore = '\0';
    const int lengthDoc = GetLength();

    if ( lengthDoc > 0 && caretPos > 0 )
    {
        // Check to ensure not matching brace that is part of a multibyte character
        int posBefore = PositionBefore( caretPos );
        if ( posBefore == ( caretPos - 1 ) )
        {
            charBefore = GetCharAt( posBefore );
        }
    }

    if ( charBefore && IsBraceChar( charBefore ) )
    { // Priority goes to character before caret
        braceAtCaret = caretPos - 1;
    }
    if ( lengthDoc > 0 && braceAtCaret < 0 && caretPos < lengthDoc )
    {
        // No brace found so check other side
        // Check to ensure not matching brace that is part of a multibyte character
        const char charAfter = GetCharAt( caretPos );
        if ( charAfter == ( caretPos + 1 ) )
        {
            if ( charAfter && IsBraceChar( charAfter ) )
            {
                braceAtCaret = caretPos;
            }
        }
    }
    if ( braceAtCaret >= 0 )
    {
        braceOpposite = BraceMatch( braceAtCaret );
    }

    BracePosition position;
    if ( braceAtCaret >= 0 )
    {
        position.current = braceAtCaret;
        if ( braceOpposite >= 0 )
        {
            position.matching = braceOpposite;
        }
    }

    return position;
}

std::optional<std::vector<qwr::u8string_view>> CScriptEditorCtrl::GetNearestWords( qwr::u8string_view wordPart, std::optional<char> separator )
{
    if ( m_apis.empty() )
    {
        return std::nullopt;
    }

    const auto startsWithPart = [&wordPart]( qwr::u8string_view word ) -> bool {
        return StartsWith_CaseInsensitive( word, wordPart );
    };

    std::vector<qwr::u8string_view> words;
    for ( auto it = ranges::find_if( m_apis, startsWithPart ); it != m_apis.end() && startsWithPart( *it ); ++it )
    {
        const auto& word = *it;
        qwr::u8string_view wordToPlace;
        if ( separator )
        {
            const auto charIt = ranges::find( word, *separator );
            if ( word.cend() != charIt )
            {
                wordToPlace = qwr::u8string_view( word.c_str(), static_cast<size_t>( std::distance( word.cbegin(), charIt ) ) );
            }
        }
        if ( wordToPlace.empty() )
        {
            wordToPlace = word;
        }
        words.emplace_back( wordToPlace );
    }

    if ( words.empty() )
    {
        return std::nullopt;
    }

    return words;
}

std::optional<qwr::u8string_view> CScriptEditorCtrl::GetFullDefinitionForWord( qwr::u8string_view word )
{
    if ( m_apis.empty() )
    {
        return std::nullopt;
    }

    const qwr::u8string wordWithBrace = qwr::u8string{ word.data(), word.size() } + '(';
    const auto startsWithPart = [&wordWithBrace]( qwr::u8string_view word ) -> bool {
        return StartsWith_CaseInsensitive( word, wordWithBrace );
    };

    auto it = ranges::find_if( m_apis, startsWithPart );
    if ( it == m_apis.cend() )
    {
        return std::nullopt;
    }

    return *it;
}

std::optional<DWORD> CScriptEditorCtrl::GetPropertyColor( const char* key )
{
    const auto propvalRet = GetPropertyExpanded_Opt( key );
    if ( !propvalRet )
    {
        return std::nullopt;
    }

    return GetColourFromHex( *propvalRet );
}

void CScriptEditorCtrl::Init()
{
    // Reset to default p_style
    StyleResetDefault();

    // Common
    SetCodePage( SC_CP_UTF8 );
    SetEOLMode( SC_EOL_CRLF );
    SetModEventMask( SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO );
    UsePopUp( true );

    // Disable Ctrl + some char
    constexpr auto ctrlcode = std::to_array<int>( { 'Q', 'W', 'E', 'R', 'I', 'O', 'P', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'B', 'N', 'M', 186, 187, 226 } );

    for ( auto code: ctrlcode )
    {
        ClearCmdKey( MAKELONG( code, SCMOD_CTRL ) );
    }

    // Disable Ctrl+Shift+some char
    for ( auto code: ranges::views::indices( 48, 122 ) )
    {
        ClearCmdKey( MAKELONG( code, SCMOD_CTRL | SCMOD_SHIFT ) );
    }

    // Shortcut keys
    AssignCmdKey( MAKELONG( SCK_NEXT, SCMOD_CTRL ), SCI_PARADOWN );
    AssignCmdKey( MAKELONG( SCK_PRIOR, SCMOD_CTRL ), SCI_PARAUP );
    AssignCmdKey( MAKELONG( SCK_NEXT, ( SCMOD_CTRL | SCMOD_SHIFT ) ), SCI_PARADOWNEXTEND );
    AssignCmdKey( MAKELONG( SCK_PRIOR, ( SCMOD_CTRL | SCMOD_SHIFT ) ), SCI_PARAUPEXTEND );
    AssignCmdKey( MAKELONG( SCK_HOME, SCMOD_NORM ), SCI_VCHOMEWRAP );
    AssignCmdKey( MAKELONG( SCK_END, SCMOD_NORM ), SCI_LINEENDWRAP );
    AssignCmdKey( MAKELONG( SCK_HOME, SCMOD_SHIFT ), SCI_VCHOMEWRAPEXTEND );
    AssignCmdKey( MAKELONG( SCK_END, SCMOD_SHIFT ), SCI_LINEENDWRAPEXTEND );

    // Tabs and indentation
    SetUseTabs( false );
    SetTabIndents( false );
    SetBackSpaceUnIndents( true );
    SetTabWidth( 4 );
    SetIndent( 4 );

    // Scroll
    SetEndAtLastLine( true );
    SetScrollWidthTracking( true );
    SetScrollWidth( 1 );

    // Auto complete
    AutoCSetIgnoreCase( true );

    // Set embedded properties
    SetProperty( "dir.base", ( qwr::path::Foobar2000() / "" ).u8string().c_str() );
    SetProperty( "dir.component", ( qwr::path::Component() / "" ).u8string().c_str() );
    SetProperty( "dir.profile", ( qwr::path::Profile() / "" ).u8string().c_str() );

    // Load properties
    ReloadScintillaSettings();
}

void CScriptEditorCtrl::RestoreDefaultStyle()
{
    // Clear and reset all styles
    ClearDocumentStyle();
    StyleResetDefault();

    // enable line numbering
    SetMarginWidthN( 1, 0 );
    SetMarginWidthN( 2, 0 );
    SetMarginWidthN( 3, 0 );
    SetMarginWidthN( 4, 0 );
    SetMarginTypeN( 0, SC_MARGIN_NUMBER );

    // Additional styles
    auto colorRet = GetPropertyColor( "style.selection.fore" );
    SetSelFore( colorRet.has_value(), colorRet.value_or( 0 ) );
    const bool hasForegroundColour = colorRet.has_value();

    colorRet = GetPropertyColor( "style.selection.back" );
    if ( colorRet )
    {
        SetSelBack( true, *colorRet );
    }
    else
    {
        if ( hasForegroundColour )
        {
            SetSelBack( true, RGB( 0xc0, 0xc0, 0xc0 ) );
        }
    }

    SetSelAlpha( GetPropertyInt( "style.selection.alpha", SC_ALPHA_NOALPHA ) );
    SetCaretFore( GetPropertyColor( "style.caret.fore" ).value_or( 0 ) );
    SetCaretWidth( GetPropertyInt( "style.caret.width", 1 ) );

    colorRet = GetPropertyColor( "style.caret.line.back" );
    if ( colorRet )
    {
        SetCaretLineVisible( true );
        SetCaretLineBack( *colorRet );
    }
    else
    {
        SetCaretLineVisible( false );
    }

    SetCaretLineBackAlpha( GetPropertyInt( "style.caret.line.back.alpha", SC_ALPHA_NOALPHA ) );
}

void CScriptEditorCtrl::TrackWidth()
{
    int max_width = 1;

    for ( auto lineIdx: ranges::views::indices( GetLineCount() ) )
    {
        // Get max width
        int pos = GetLineEndPosition( lineIdx );
        int width = PointXFromPosition( pos );

        max_width = std::max( width, max_width );
    }

    SetScrollWidth( max_width );
}

void CScriptEditorCtrl::LoadStyleFromProperties()
{
    qwr::u8string propval;
    for ( const auto [style_num, propname]: js_style_table )
    {
        const auto propvalRet = GetPropertyExpanded_Opt( propname );
        if ( propvalRet )
        {
            const ScintillaStyle style = ParseStyle( *propvalRet );

            if ( style.flags & ESF_FONT )
            {
                StyleSetFont( style_num, style.font.c_str() );
            }

            if ( style.flags & ESF_SIZE )
            {
                StyleSetSize( style_num, style.size );
            }
            if ( style.flags & ESF_FORE )
            {
                StyleSetFore( style_num, style.fore );
            }

            if ( style.flags & ESF_BACK )
            {
                StyleSetBack( style_num, style.back );
            }

            if ( style.flags & ESF_ITALICS )
            {
                StyleSetItalic( style_num, style.italics );
            }

            if ( style.flags & ESF_BOLD )
            {
                StyleSetBold( style_num, style.bold );
            }

            if ( style.flags & ESF_UNDERLINED )
            {
                StyleSetUnderline( style_num, style.underlined );
            }

            if ( style.flags & ESF_CASEFORCE )
            {
                StyleSetCase( style_num, style.case_force );
            }
        }

        if ( style_num == STYLE_DEFAULT )
        {
            StyleClearAll();
        }
    }
}

void CScriptEditorCtrl::AutoMarginWidth()
{
    // Auto margin width
    int linenumwidth = 1;
    int linecount = GetLineCount();

    while ( linecount >= 10 )
    {
        linecount /= 10;
        ++linenumwidth;
    }

    const int oldmarginwidth = GetMarginWidthN( 0 );
    const int marginwidth = 4 + linenumwidth * ( TextWidth( STYLE_LINENUMBER, "9" ) );
    if ( oldmarginwidth != marginwidth )
    {
        SetMarginWidthN( 0, marginwidth );
    }
}

void CScriptEditorCtrl::SetIndentation( int line, int indent )
{
    if ( indent < 0 )
    {
        return;
    }

    Sci_CharacterRange crange = GetSelection();
    int posBefore = GetLineIndentPosition( line );
    SetLineIndentation( line, indent );
    int posAfter = GetLineIndentPosition( line );
    int posDifference = posAfter - posBefore;
    if ( posAfter > posBefore )
    {
        // Move selection on
        if ( crange.cpMin >= posBefore )
        {
            crange.cpMin += posDifference;
        }

        if ( crange.cpMax >= posBefore )
        {
            crange.cpMax += posDifference;
        }
    }
    else if ( posAfter < posBefore )
    {
        // Move selection back
        if ( crange.cpMin >= posAfter )
        {
            if ( crange.cpMin >= posBefore )
            {
                crange.cpMin += posDifference;
            }
            else
            {
                crange.cpMin = posAfter;
            }
        }

        if ( crange.cpMax >= posAfter )
        {
            if ( crange.cpMax >= posBefore )
            {
                crange.cpMax += posDifference;
            }
            else
            {
                crange.cpMax = posAfter;
            }
        }
    }

    SetSel( crange.cpMin, crange.cpMax );
}

std::optional<qwr::u8string> CScriptEditorCtrl::GetPropertyExpanded_Opt( const char* key )
{
    int len = GetPropertyExpanded( key, nullptr );
    if ( !len )
    {
        return std::nullopt;
    }

    qwr::u8string propval;
    propval.resize( len + 1 );
    GetPropertyExpanded( key, propval.data() );
    propval.resize( strlen( propval.c_str() ) );

    return propval;
}

} // namespace smp::ui::sci
