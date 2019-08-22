#include <stdafx.h>
#include "editorctrl.h"
#include "scintilla_prop_sets.h"

#include <utils/file_helpers.h>
#include <utils/string_helpers.h>
#include <component_paths.h>

#include <optional>
#include <charconv>

namespace
{

constexpr std::array<t_style_to_key, 16> js_style_table = {
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

bool IsSymIncludes( const StyleAndWords& symbols, std::u8string value )
{
    if ( symbols.IsEmpty() )
    {
        return false;
    }
    else if ( std::isalpha( symbols.words[0] ) )
    {
        const auto words = smp::string::Split( static_cast<std::u8string_view>( symbols.words ), ' ' );
        return ( words.cend() != ranges::find( words, value ) );
    }
    else
    {
        // Set of individual characters. Only one character allowed for now
        char ch = symbols.words[0];
        return strchr( value.c_str(), ch ) != 0;
    }

    return false;
}

bool IsBraceChar( int ch )
{
    return ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '{' || ch == '}';
}

bool IsCSym( int ch )
{
    return __iswcsym( static_cast<wint_t>( ch ) );
}

int int_from_hex_digit( int ch )
{
    if ( ( ch >= '0' ) && ( ch <= '9' ) )
    {
        return ch - '0';
    }
    else if ( ch >= 'A' && ch <= 'F' )
    {
        return ch - 'A' + 10;
    }
    else if ( ch >= 'a' && ch <= 'f' )
    {
        return ch - 'a' + 10;
    }
    else
    {
        return 0;
    }
}

int int_from_hex_byte( const char* hex_byte )
{
    return ( int_from_hex_digit( hex_byte[0] ) << 4 ) | ( int_from_hex_digit( hex_byte[1] ) );
}

t_size LengthWord( const char* word, char otherSeparator )
{
    const char* endWord = 0;
    // Find an otherSeparator

    if ( otherSeparator )
        endWord = strchr( word, otherSeparator );

    // Find a '('. If that fails go to the end of the string.
    if ( !endWord )
        endWord = strchr( word, '(' );

    if ( !endWord )
        endWord = word + strlen( word );

    // Last case always succeeds so endWord != 0

    // Drop any space characters.
    if ( endWord > word )
    {
        endWord--;
        // Back from the '(', otherSeparator, or '\0'
        // Move backwards over any spaces

        while ( ( endWord > word ) && ( std::isspace( static_cast<unsigned char>( *endWord ) ) ) )
        {
            endWord--;
        }
    }

    return endWord - word;
}

DWORD ParseHex( const char* hex )
{
    // len('#000000') = 7
    if ( pfc::strlen_max( hex, 8 ) == 8 )
        return 0;

    int r = int_from_hex_byte( hex + 1 );
    int g = int_from_hex_byte( hex + 3 );
    int b = int_from_hex_byte( hex + 5 );

    return RGB( r, g, b );
}

DWORD ParseHex( std::u8string_view hex )
{
    return ParseHex( std::u8string{ hex.data(), hex.size() }.c_str() );
}

t_sci_editor_style ParseStyle( std::u8string_view p_definition )
{
    t_sci_editor_style p_style;

    const auto attributes = smp::string::Split( p_definition, ',' );
    for ( const auto& attribute: attributes )
    {
        if ( attribute.empty() )
        {
            continue;
        }

        const auto values = smp::string::Split( attribute, ':' );
        if ( values.empty() )
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
                p_style.font.set_string_nc( values[1].data(), values[1].size() );
            }
        }
        else if ( name == "fore" )
        {
            if ( values.size() >= 2 )
            {
                p_style.flags |= ESF_FORE;
                p_style.fore = ParseHex( values[1] );
            }
        }
        else if ( name == "back" )
        {
            if ( values.size() >= 2 )
            {
                p_style.flags |= ESF_BACK;
                p_style.back = ParseHex( values[1] );
            }
        }
        else if ( name == "size" )
        {
            if ( values.size() >= 2 )
            {
                auto intRet = smp::string::GetNumber<unsigned>( values[1] );
                if ( intRet )
                {
                    p_style.flags |= ESF_SIZE;
                    p_style.size = intRet.value();
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

            if ( values.size() >= 2 && values[1].size() >= 1 )
            {
                const char8_t ch = values[1][0];
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

std::vector<std::u8string_view> SplitStringLines( std::u8string_view str )
{
    std::vector<std::u8string_view> lines;
    for ( std::u8string_view curScope = str; !curScope.empty(); )
    {
        if ( size_t pos = curScope.find_first_of( "\r\n" );
             std::string::npos != pos )
        {
            if ( pos )
            {
                lines.emplace_back( curScope.data(), pos );
                curScope.remove_prefix( pos );
            }

            while ( !curScope.empty() && ( curScope[0] == '\r' || curScope[0] == '\n' ) )
            {
                curScope.remove_prefix( 1 );
            }
        }
        else
        {
            lines.emplace_back( curScope.data(), curScope.size() );
            curScope = std::u8string_view{};
        }
    }

    return lines;
}

template <typename T>
std::u8string JoinWithSpace( const T& cont )
{
    static_assert( std::is_same_v<typename T::value_type, std::u8string> || std::is_same_v<typename T::value_type, std::u8string_view> || std::is_same_v<typename T::value_type, const char8_t*> );

    std::u8string words_str;
    words_str.reserve( cont.size() * 6 );
    for ( const auto& word: cont )
    {
        if constexpr ( std::is_constructible_v<std::u8string, typename T::value_type> )
        {
            words_str += word;
        }
        else
        {
            words_str += std::u8string{ word.data(), word.size() };
        }
        words_str += ' ';
    }
    if ( !words_str.empty() )
    {
        words_str.resize( words_str.size() - 1 );
    }
    return words_str;
}

bool StartsWithInsensitive( const std::u8string_view& a, const std::u8string_view& b )
{
    return ( ( a.size() >= b.size() ) && !pfc::stricmp_ascii_ex( a.data(), b.size(), b.data(), b.size() ) );
}

bool StartsWith( const std::u8string_view& a, const std::u8string_view& b )
{
    return ( ( a.size() >= b.size() ) && !a.compare( 0, b.size(), b ) );
}

} // namespace

bool CScriptEditorCtrl::KeyWordComparator::operator()( const std::u8string& a, const std::u8string& b )
{
    int result = _stricmp( a.c_str(), b.c_str() );
    if ( !result && !a.empty() && !b.empty() && std::isalpha( a[0] ) && ( a[0] != b[0] ) )
    {
        return std::isupper( static_cast<unsigned char>( a[0] ) );
    }
    else
    {
        return result < 0;
    }
}

LRESULT CScriptEditorCtrl::OnKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = FALSE;
    ::PostMessage( ::GetAncestor( m_hWnd, GA_PARENT ), static_cast<UINT>( smp::MiscMessage::key_down ), wParam, lParam );
    return TRUE;
}

LRESULT CScriptEditorCtrl::OnUpdateUI( LPNMHDR pnmn )
{
    // Match Brace
    int braceAtCaret = -1;
    int braceOpposite = -1;

    FindBraceMatchPos( braceAtCaret, braceOpposite );

    if ( braceAtCaret != -1 && braceOpposite == -1 )
    {
        BraceBadLight( braceAtCaret );
        SetHighlightGuide( 0 );
    }
    else
    {
        char chBrace = GetCharAt( braceAtCaret );

        BraceHighlight( braceAtCaret, braceOpposite );

        int columnAtCaret = GetColumn( braceAtCaret );
        int columnOpposite = GetColumn( braceOpposite );

        SetHighlightGuide( std::min( columnAtCaret, columnOpposite ) );
    }

    return 0;
}

LRESULT CScriptEditorCtrl::OnCharAdded( LPNMHDR pnmh )
{
    SCNotification* notification = (SCNotification*)pnmh;
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
                    CallTipCancel();
                else
                    StartCallTip();
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
                    StartAutoComplete();
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
                    StartAutoComplete();
            }
        }
    }

    return 0;
}

LRESULT CScriptEditorCtrl::OnZoom( LPNMHDR pnmn )
{
    AutoMarginWidth();

    return 0;
}

LRESULT CScriptEditorCtrl::OnChange( UINT uNotifyCode, int nID, HWND wndCtl )
{
    AutoMarginWidth();

    return 0;
}

void CScriptEditorCtrl::ReadAPI()
{
    m_apis.clear();

    constexpr char8_t propname[] = "api.jscript";

    const auto propvalRet = GetPropertyExpanded_Opt( propname );
    if ( !propvalRet )
    {
        return;
    }

    const auto files = smp::string::Split<char8_t>( propvalRet.value(), ';' );
    std::u8string content;
    for ( const auto& file: files )
    {
        try
        {
            const auto content = smp::file::ReadFile( std::u8string{ file.data(), file.size() }, CP_UTF8 );
            for ( const auto& line: SplitStringLines( content ) )
            {
                if ( !line.empty() && IsCSym( line[0] ) )
                {
                    m_apis.emplace( line.data(), line.size() );
                }
            }
        }
        catch ( const smp::SmpException& e )
        {
            FB2K_console_formatter() << "Warning: " SMP_NAME_WITH_VERSION ": Could not load file " << std::u8string{ file.data(), file.size() }.c_str();
            FB2K_console_formatter() << e.what();
        }
    }
}

void CScriptEditorCtrl::SetContent( const char* text, bool clear_undo_buffer )
{
    SetText( text );
    ConvertEOLs( SC_EOL_CRLF );

    if ( clear_undo_buffer )
        EmptyUndoBuffer();

    Colourise( 0, std::numeric_limits<unsigned int>::max() );
    GrabFocus();
    TrackWidth();
}

void CScriptEditorCtrl::SetJScript()
{
    // clang-format off

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

    // Set lexer
    SetLexer( SCLEX_CPP );
    // Set keywords
    SetKeyWords( 0, keywords_str.c_str() );
    // Set styles
    SetAllStylesFromTable( js_style_table );
    // Colorise
    Colourise( 0, std::numeric_limits<unsigned int>::max() );
}

void CScriptEditorCtrl::SetScintillaSettings()
{
    auto getIntFromProp = [&]( const std::string& propName ) -> std::optional<int> {
        const auto propvalRet = GetPropertyExpanded_Opt( propName.c_str() );
        if ( !propvalRet )
        {
            return std::nullopt;
        }

        return smp::string::GetNumber<int>( static_cast<std::string_view>( propvalRet.value() ) );
    };

    auto retVal = getIntFromProp( "style.wrap.mode" );
    if ( retVal )
    {
        SetWrapMode( retVal.value() );
    }

    retVal = getIntFromProp( "style.wrap.visualflags" );
    if ( retVal )
    {
        SetWrapVisualFlags( retVal.value() );
    }

    retVal = getIntFromProp( "style.wrap.visualflags.location" );
    if ( retVal )
    {
        SetWrapVisualFlagsLocation( retVal.value() );
    }

    retVal = getIntFromProp( "style.wrap.indentmode" );
    if ( retVal )
    {
        SetWrapIndentMode( retVal.value() );
    }
}

BOOL CScriptEditorCtrl::SubclassWindow( HWND hWnd )
{
    BOOL bRet = CScintillaCtrl::SubclassWindow( hWnd );

    if ( bRet )
        Init();

    return bRet;
}

Sci_CharacterRange CScriptEditorCtrl::GetSelection()
{
    Sci_CharacterRange crange;
    crange.cpMin = GetSelectionStart();
    crange.cpMax = GetSelectionEnd();
    return crange;
}

int CScriptEditorCtrl::GetCaretInLine()
{
    int caret = GetCurrentPos();
    int line = LineFromPosition( caret );
    int lineStart = PositionFromLine( line );
    return caret - lineStart;
}

std::u8string CScriptEditorCtrl::GetCurrentLine()
{
    std::u8string buf;

    buf.resize( GetCurLine( nullptr, 0 ) + 1 );
    GetCurLine( buf.data(), buf.size() );
    buf.resize( strlen( buf.c_str() ) );

    return buf;
}

IndentationStatus CScriptEditorCtrl::GetIndentState( int line )
{
    // C like language indentation defined by braces and keywords
    IndentationStatus indentState = IndentationStatus::isNone;

    {
        std::u8string controlWords[20];
        size_t parts = GetLinePartsInStyle( line, m_StatementIndent.styleNumber, -1, controlWords );

        for ( const auto& controlWord: controlWords )
        {
            if ( IsSymIncludes( m_StatementIndent, controlWord ) )
                indentState = IndentationStatus::isKeyWordStart;
        }

        parts = GetLinePartsInStyle( line, m_StatementEnd.styleNumber, -1, controlWords );

        for ( const auto& controlWord: controlWords )
        {
            if ( IsSymIncludes( m_StatementEnd, controlWord ) )
                indentState = IndentationStatus::isNone;
        }
    }

    {
        // Braces override keywords
        std::u8string controlStrings[20];
        size_t parts = GetLinePartsInStyle( line, m_BlockEnd.styleNumber, -1, controlStrings );

        for ( const auto& controlString: controlStrings )
        {
            if ( IsSymIncludes( m_BlockEnd, controlString ) )
                indentState = IndentationStatus::isBlockEnd;

            if ( IsSymIncludes( m_BlockStart, controlString ) )
                indentState = IndentationStatus::isBlockStart;
        }
    }

    return indentState;
}

size_t CScriptEditorCtrl::GetLinePartsInStyle( int line, int style1, int style2, nonstd::span<std::u8string> parts )
{
    for ( auto& part: parts )
    {
        part.clear();
    }

    std::u8string curPart;
    size_t partIdx = 0;
    int thisLineStart = PositionFromLine( line );
    int nextLineStart = PositionFromLine( line + 1 );

    for ( int pos = thisLineStart; pos < nextLineStart; ++pos )
    {
        if ( ( GetStyleAt( pos ) == style1 ) || ( GetStyleAt( pos ) == style2 ) )
        {
            curPart += GetCharAt( pos );
        }
        else if ( !curPart.empty() )
        {
            parts[partIdx++] = curPart;
            curPart.clear();
        }

        if ( partIdx >= parts.size() )
        {
            curPart.clear();
            break;
        }
    }

    if ( !curPart.empty() && ( partIdx < parts.size() ) )
    {
        parts[partIdx++] = curPart;
    }

    return partIdx;
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
    m_szCurrentCallTipWord = "";
    std::u8string line = GetCurrentLine();
    int current = GetCaretInLine();
    int pos = GetCurrentPos();
    int braces = 0;

    do
    {
        while ( current > 0 && ( braces || line[current - 1] != '(' ) )
        {
            if ( line[current - 1] == '(' )
                braces--;
            else if ( line[current - 1] == ')' )
                braces++;

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
        return true;

    m_nStartCalltipWord = current - 1;

    while ( m_nStartCalltipWord > 0 && ( IsCSym( line[m_nStartCalltipWord - 1] ) || ( line[m_nStartCalltipWord - 1] == '.' ) ) )
    {
        --m_nStartCalltipWord;
    }

    line.resize( current );
    m_szCurrentCallTipWord = line.c_str() + m_nStartCalltipWord;
    m_szFunctionDefinition = "";
    FillFunctionDefinition( pos );
    return true;
}

void CScriptEditorCtrl::ContinueCallTip()
{
    const std::u8string line = GetCurrentLine();
    int current = GetCaretInLine();
    int braces = 0;
    int commas = 0;

    for ( int i = m_nStartCalltipWord; i < current; ++i )
    {
        if ( line[i] == '(' )
            braces++;
        else if ( line[i] == ')' && braces )
            braces--;
        else if ( braces == 1 && line[i] == ',' )
            commas++;
    }

    int startHighlight = 0;

    while ( m_szFunctionDefinition[startHighlight] && m_szFunctionDefinition[startHighlight] != '(' )
        startHighlight++;

    if ( m_szFunctionDefinition[startHighlight] == '(' )
        startHighlight++;

    while ( m_szFunctionDefinition[startHighlight] && commas )
    {
        if ( m_szFunctionDefinition[startHighlight] == ',' )
            commas--;
        // If it reached the end of the argument list it means that the user typed in more
        // arguments than the ones listed in the calltip
        if ( m_szFunctionDefinition[startHighlight] == ')' )
            commas = 0;
        else
            startHighlight++;
    }

    if ( m_szFunctionDefinition[startHighlight] == ',' )
        startHighlight++;

    int endHighlight = startHighlight;

    while ( m_szFunctionDefinition[endHighlight] && m_szFunctionDefinition[endHighlight] != ',' && m_szFunctionDefinition[endHighlight] != ')' )
    {
        endHighlight++;
    }

    CallTipSetHlt( startHighlight, endHighlight );
}

void CScriptEditorCtrl::FillFunctionDefinition( int pos )
{
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

    m_szFunctionDefinition = std::u8string{ definitionRet.value().data(), definitionRet.value().size() };

    CallTipShow( m_nLastPosCallTip - m_szCurrentCallTipWord.length(), m_szFunctionDefinition.c_str() );
    ContinueCallTip();
}

bool CScriptEditorCtrl::StartAutoComplete()
{
    if ( m_apis.empty() )
    {
        return false;
    }

    const std::u8string line = GetCurrentLine();
    const int current = GetCaretInLine();

    int startword = current;

    while ( ( startword > 0 ) && ( IsCSym( line[startword - 1] ) || line[startword - 1] == '.' ) )
    {
        startword--;
    }

    const std::u8string root{ line.c_str() + startword, line.c_str() + current };
    const auto words_ret = GetNearestWords( root, '(' );
    if ( !words_ret )
    {
        return false;
    }

    const auto& wordsStr = JoinWithSpace( words_ret.value() );
    AutoCShow( root.length(), wordsStr.c_str() );

    return true;
}

int CScriptEditorCtrl::IndentOfBlock( int line )
{
    if ( line < 0 )
        return 0;

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
        const auto identState = GetIndentState( curLine - 1 );
        if ( ( GetIndentState( curLine - 1 ) == IndentationStatus::isKeyWordStart ) )
        {
            const auto line = GetCurrentLine();
            const auto it = ranges::find_if( line, []( const auto& ch ) { return std::isspace( static_cast<unsigned char>( ch ) ); } );
            if ( it != line.cend() && *it == '{' )
            { // dedent open brace on keyword
                SetIndentation( curLine, indentBlock - indentSize );
            }
        }
        else
        {
            SetIndentation( curLine, indentBlock );
        }
    }
}

bool CScriptEditorCtrl::FindBraceMatchPos( int& braceAtCaret, int& braceOpposite )
{
    bool isInside = false;
    /*
    const int mainSel = win.Call( SCI_GETMAINSELECTION, 0, 0 );
    if ( win.Call( SCI_GETSELECTIONNCARETVIRTUALSPACE, mainSel, 0 ) > 0 )
        return false;
    */
    int caretPos = GetCurrentPos();
    braceAtCaret = -1;
    braceOpposite = -1;
    char charBefore = '\0';
    const int lengthDoc = GetLength();

    if ( ( lengthDoc > 0 ) && ( caretPos > 0 ) )
    {
        // Check to ensure not matching brace that is part of a multibyte character
        int posBefore = PositionBefore( caretPos );
        if ( posBefore == ( caretPos - 1 ) )
        {
            charBefore = GetCharAt( posBefore );
        }
    }
    // Priority goes to character before caret
    if ( charBefore && IsBraceChar( charBefore ) )
    {
        braceAtCaret = caretPos - 1;
    }
    bool colonMode = false;
    bool isAfter = true;
    if ( lengthDoc > 0 && ( braceAtCaret < 0 ) && ( caretPos < lengthDoc ) )
    {
        // No brace found so check other side
        // Check to ensure not matching brace that is part of a multibyte character
        const char charAfter = GetCharAt( caretPos );
        if ( charAfter == ( caretPos + 1 ) )
        {
            if ( charAfter && IsBraceChar( charAfter ) )
            {
                braceAtCaret = caretPos;
                isAfter = false;
            }
        }
    }
    if ( braceAtCaret >= 0 )
    {
        if ( colonMode )
        {
            const int lineStart = LineFromPosition( braceAtCaret );
            const int lineMaxSubord = GetLastChild( lineStart, -1 );
            braceOpposite = GetLineEndPosition( lineMaxSubord );
        }
        else
        {
            braceOpposite = BraceMatch( braceAtCaret );
        }
        if ( braceOpposite > braceAtCaret )
        {
            isInside = isAfter;
        }
        else
        {
            isInside = !isAfter;
        }
    }
    return isInside;
}

std::optional<std::vector<std::u8string_view>> CScriptEditorCtrl::GetNearestWords( std::u8string_view wordPart, std::optional<char8_t> separator )
{
    if ( m_apis.empty() )
    {
        return std::nullopt;
    }

    const auto startsWithPart = [&wordPart]( std::u8string_view word ) -> bool {
        return StartsWithInsensitive( word, wordPart );
    };

    std::vector<std::u8string_view> words;
    for ( auto it = ranges::find_if( m_apis, startsWithPart ); it != m_apis.end() && startsWithPart( *it ); ++it )
    {
        const auto& word = *it;
        std::u8string_view wordToPlace;
        if ( separator )
        {
            const auto charIt = ranges::find( word, separator.value() );
            if ( word.cend() != charIt )
            {
                wordToPlace = std::u8string_view( word.c_str(), (size_t)std::distance( word.cbegin(), charIt ) );
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

std::optional<std::u8string_view> CScriptEditorCtrl::GetFullDefinitionForWord( std::u8string_view word )
{
    if ( m_apis.empty() )
    {
        return std::nullopt;
    }

    const std::u8string wordWithBrace = std::u8string{ word.data(), word.size() } + '(';
    const auto startsWithPart = [&wordWithBrace]( std::u8string_view word ) -> bool {
        return StartsWithInsensitive( word, wordWithBrace );
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

    return ParseHex( propvalRet.value() );
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
    const int ctrlcode[22] = { 'Q', 'W', 'E', 'R', 'I', 'O', 'P', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'B', 'N', 'M', 186, 187, 226 };

    for ( int i = 0; i < _countof( ctrlcode ); ++i )
    {
        ClearCmdKey( MAKELONG( ctrlcode[i], SCMOD_CTRL ) );
    }

    // Disable Ctrl+Shift+some char
    for ( int i = 48; i < 122; ++i )
    {
        ClearCmdKey( MAKELONG( i, SCMOD_CTRL | SCMOD_SHIFT ) );
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
    SetProperty( "dir.base", smp::get_fb2k_path().c_str() );
    SetProperty( "dir.component", smp::get_fb2k_component_path().c_str() );
    SetProperty( "dir.profile", smp::get_profile_path().c_str() );

    // Load properties
    LoadProperties( g_sci_prop_sets.val() );
}

void CScriptEditorCtrl::LoadProperties( const std::vector<t_sci_prop_set>& data )
{
    for ( t_size i = 0; i < data.size(); ++i )
    {
        SetProperty( data[i].key.get_ptr(), data[i].val.get_ptr() );
    }
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
        SetSelBack( true, colorRet.value() );
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
        SetCaretLineBack( colorRet.value() );
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

    for ( int i = 0; i < GetLineCount(); ++i )
    {
        // Get max width
        int pos = GetLineEndPosition( i );
        int width = PointXFromPosition( pos );

        if ( width > max_width )
            max_width = width;
    }

    SetScrollWidth( max_width );
}

void CScriptEditorCtrl::SetAllStylesFromTable( nonstd::span<const t_style_to_key> table )
{
    std::u8string propval;
    for ( const auto [style_num, key]: table )
    {
        const auto propvalRet = GetPropertyExpanded_Opt( key );
        if ( propvalRet )
        {
            const t_sci_editor_style style = ParseStyle( propvalRet.value() );

            if ( style.flags & ESF_FONT )
                StyleSetFont( style_num, style.font.get_ptr() );

            if ( style.flags & ESF_SIZE )
                StyleSetSize( style_num, style.size );

            if ( style.flags & ESF_FORE )
                StyleSetFore( style_num, style.fore );

            if ( style.flags & ESF_BACK )
                StyleSetBack( style_num, style.back );

            if ( style.flags & ESF_ITALICS )
                StyleSetItalic( style_num, style.italics );

            if ( style.flags & ESF_BOLD )
                StyleSetBold( style_num, style.bold );

            if ( style.flags & ESF_UNDERLINED )
                StyleSetUnderline( style_num, style.underlined );

            if ( style.flags & ESF_CASEFORCE )
                StyleSetCase( style_num, style.case_force );
        }

        if ( style_num == STYLE_DEFAULT )
            StyleClearAll();
    }
}

void CScriptEditorCtrl::AutoMarginWidth()
{
    // Auto margin width
    int linenumwidth = 1;
    int marginwidth, oldmarginwidth;
    int linecount;

    linecount = GetLineCount();

    while ( linecount >= 10 )
    {
        linecount /= 10;
        ++linenumwidth;
    }

    oldmarginwidth = GetMarginWidthN( 0 );
    marginwidth = 4 + linenumwidth * ( TextWidth( STYLE_LINENUMBER, "9" ) );

    if ( oldmarginwidth != marginwidth )
        SetMarginWidthN( 0, marginwidth );
}

void CScriptEditorCtrl::SetIndentation( int line, int indent )
{
    if ( indent < 0 )
        return;

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
                crange.cpMin += posDifference;
            else
                crange.cpMin = posAfter;
        }

        if ( crange.cpMax >= posAfter )
        {
            if ( crange.cpMax >= posBefore )
                crange.cpMax += posDifference;
            else
                crange.cpMax = posAfter;
        }
    }

    SetSel( crange.cpMin, crange.cpMax );
}

std::optional<std::u8string> CScriptEditorCtrl::GetPropertyExpanded_Opt( const char8_t* key )
{
    int len = GetPropertyExpanded( key, 0 );
    if ( !len )
    {
        return std::nullopt;
    }

    std::u8string propval;
    propval.resize( len );
    GetPropertyExpanded( key, propval.data() );
    propval.resize( strlen( propval.c_str() ) );

    return propval;
}
