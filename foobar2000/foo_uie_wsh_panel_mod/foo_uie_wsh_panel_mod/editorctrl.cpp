#include "stdafx.h"
#include "editorctrl.h"
#include "scintilla_prop_sets.h"
#include "helpers.h"


const t_style_to_key_table default_style_table[] = 
{
	// Default
	{STYLE_DEFAULT, "style.default"},
	// Line number
	{STYLE_LINENUMBER, "style.linenumber"},
	// Bracelight
	{STYLE_BRACELIGHT, "style.bracelight"},
	// Bracebad
	{STYLE_BRACEBAD, "style.bracebad"},
	{-1, NULL},
};

const t_style_to_key_table js_style_table[] =
{
	// Comments
	{SCE_C_COMMENT, "style.comment"},
	{SCE_C_COMMENTLINE, "style.comment"},
	{SCE_C_COMMENTDOC, "style.comment"},
	{SCE_C_COMMENTLINEDOC, "style.comment"},
	{SCE_C_COMMENTDOCKEYWORD, "style.comment"},
	{SCE_C_COMMENTDOCKEYWORDERROR, "style.comment"},
	// Keywords
	{SCE_C_WORD, "style.keyword"},
	// Indentifier
	{SCE_C_IDENTIFIER, "style.indentifier"},
	// Numbers
	{SCE_C_NUMBER, "style.number"},
	// String/Chars
	{SCE_C_STRING, "style.string"},
	{SCE_C_CHARACTER, "style.string"},
	// Operators
	{SCE_C_OPERATOR, "style.operator"},
	//
	{-1, NULL},
};

const t_style_to_key_table vbs_style_table[] =
{
	// Comments
	{SCE_B_COMMENT, "style.comment"},
	// Keywords
	{SCE_B_KEYWORD, "style.keyword"},
	// Indentifier
	{SCE_B_IDENTIFIER, "style.indentifier"},
	// Numbers
	{SCE_B_NUMBER, "style.number"},
	// String/Chars
	{SCE_B_STRING, "style.string"},
	// Operators
	{SCE_B_OPERATOR, "style.operator"},
	//
	{-1, NULL},
};

static bool IsSymIncludes(const StyleAndWords & symbols, const SString value) 
{
	if (symbols.IsEmpty()) 
	{
		return false;
	} 
	else if (isalpha(symbols.words[0])) 
	{
		// Set of symbols separated by spaces
		t_size lenVal = value.length();
		const char *symbol = symbols.words.c_str();

		while (symbol) 
		{
			const char *symbolEnd = strchr(symbol, ' ');
			t_size lenSymbol = strlen(symbol);

			if (symbolEnd)
				lenSymbol = symbolEnd - symbol;

			if (lenSymbol == lenVal) 
			{
				if (strncmp(symbol, value.c_str(), lenSymbol) == 0) 
				{
					return true;
				}
			}

			symbol = symbolEnd;

			if (symbol)
				symbol++;
		}
	} 
	else
	{
		// Set of individual characters. Only one character allowed for now
		char ch = symbols.words[0];
		return strchr(value.c_str(), ch) != 0;
	}

	return false;
}

static inline bool IsASpace(unsigned int ch)
{
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

static bool IsBraceChar(int ch)
{
	return ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '{' || ch == '}';
}

static bool IsIdentifierChar(int ch)
{
	return __iswcsym(ch);
}

struct StringComparePartialNC
{
	StringComparePartialNC(t_size len_) : len(len_)
	{
	}

	int operator()(const char * s1, const char * s2) const
	{
		t_size len1 = pfc::strlen_max_t(s1, len);
		t_size len2 = pfc::strlen_max_t(s2, len);

		return pfc::stricmp_ascii_ex(s1, len1, s2, len2);
	}

	t_size len;
};

struct StringCompareSpecial
{
	int operator()(const char * s1, const char * s2) const
	{
		int result = _stricmp(s1, s2);

		if (result == 0)
		{
			if (isalpha(*s1) && (*s1 != *s2))
			{
				return islower(*s1) ? -1 : 1;
			}
		}

		return result;
	}
};

static t_size LengthWord(const char * word, char otherSeparator)
{
	const char *endWord = 0;
	// Find an otherSeparator

	if (otherSeparator)
		endWord = strchr(word, otherSeparator);

	// Find a '('. If that fails go to the end of the string.
	if (!endWord)
		endWord = strchr(word, '(');

	if (!endWord)
		endWord = word + strlen(word);

	// Last case always succeeds so endWord != 0

	// Drop any space characters.
	if (endWord > word)
	{
		endWord--;	// Back from the '(', otherSeparator, or '\0'
		// Move backwards over any spaces

		while ((endWord > word) && (IsASpace(*endWord)))
		{
			endWord--;
		}
	}

	return endWord - word;
}

static DWORD ParseHex(const char * hex)
{
	// len('#000000') = 7
	if (pfc::strlen_max(hex, 8) == 8)
		return 0;

	int r = helpers::int_from_hex_byte(hex + 1);
	int g = helpers::int_from_hex_byte(hex + 3);
	int b = helpers::int_from_hex_byte(hex + 5);

	return RGB(r, g, b);
}

static bool ParseStyle(const char * p_definition, t_sci_editor_style & p_style)
{
	if (p_definition == 0 || !*p_definition)
		return false;

	char * val = _strdup(p_definition);
	char * opt = val;

	while (opt)
	{
		// Find attribute separator
		char * cpComma = strchr(opt, ',');

		if (cpComma) 
		{
			// If found, we terminate the current attribute (opt) string
			*cpComma = 0;
		}

		// Find attribute name/value separator
		char * colon = strchr(opt, ':');

		if (colon)
		{
			// If found, we terminate the current attribute name and point on the value
			*colon++ = '\0';
		}

		if (0 == strcmp(opt, "italics"))
		{
			p_style.flags |= ESF_ITALICS;
			p_style.italics = true;
		}
		else if (0 == strcmp(opt, "notitalics"))
		{
			p_style.flags |= ESF_ITALICS;
			p_style.italics = false;
		}
		else if (0 == strcmp(opt, "bold")) 
		{
			p_style.flags |= ESF_BOLD;
			p_style.bold = true;
		}
		else if (0 == strcmp(opt, "notbold"))
		{
			p_style.flags |= ESF_BOLD;
			p_style.bold = false;
		}
		else if (0 == strcmp(opt, "font"))
		{
			p_style.flags |= ESF_FONT;
			p_style.font = colon;
		}
		else if (0 == strcmp(opt, "fore")) 
		{
			p_style.flags |= ESF_FORE;
			p_style.fore = ParseHex(colon);
		}
		else if (0 == strcmp(opt, "back")) 
		{
			p_style.flags |= ESF_BACK;
			p_style.back = ParseHex(colon);
		}
		else if (0 == strcmp(opt, "size"))
		{
			p_style.flags |= ESF_SIZE;
			p_style.size = atoi(colon);
		}
		else if (0 == strcmp(opt, "underlined"))
		{
			p_style.flags |= ESF_UNDERLINED;
			p_style.underlined = true;
		}
		else if (0 == strcmp(opt, "notunderlined"))
		{
			p_style.flags |= ESF_UNDERLINED;
			p_style.underlined = false;
		}
		else if (0 == strcmp(opt, "case"))
		{
			p_style.flags |= ESF_CASEFORCE;
			p_style.case_force = SC_CASE_MIXED;

			if (colon)
			{
				if (*colon == 'u')
					p_style.case_force = SC_CASE_UPPER;
				else if (*colon == 'l')
					p_style.case_force = SC_CASE_LOWER;
			}
		}

		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}

	free(val);
	return true;
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
	int line = LineFromPosition(caret);
	int lineStart = PositionFromLine(line);
	return caret - lineStart;
}

pfc::string8 CScriptEditorCtrl::GetCurrentLine()
{
	pfc::string8 buf;
	int len = 0;

	len = GetCurLine(0, 0);
	GetCurLine(buf.lock_buffer(len), len);
	buf.unlock_buffer();

	return buf;
}

IndentationStatus CScriptEditorCtrl::GetIndentState(int line)
{
	// C like language indentation defined by braces and keywords
	IndentationStatus indentState = isNone;
	SString controlWords[20];
	unsigned int parts = GetLinePartsInStyle(line, m_StatementIndent.styleNumber,
		-1, controlWords, _countof(controlWords));
	unsigned int i;

	for (i = 0; i < parts; i++) 
	{
		if (IsSymIncludes(m_StatementIndent, controlWords[i]))
			indentState = isKeyWordStart;
	}

	parts = GetLinePartsInStyle(line, m_StatementEnd.styleNumber,
		-1, controlWords, _countof(controlWords));

	for (i = 0; i < parts; i++) 
	{
		if (IsSymIncludes(m_StatementEnd, controlWords[i]))
			indentState = isNone;
	}
	// Braces override keywords
	SString controlStrings[20];

	parts = GetLinePartsInStyle(line, m_BlockEnd.styleNumber,
		-1, controlStrings, _countof(controlStrings));

	for (unsigned int j = 0; j < parts; j++)
	{
		if (IsSymIncludes(m_BlockEnd, controlStrings[j]))
			indentState = isBlockEnd;

		if (IsSymIncludes(m_BlockStart, controlStrings[j]))
			indentState = isBlockStart;
	}

	return indentState;
}

unsigned int CScriptEditorCtrl::GetLinePartsInStyle(int line, int style1, int style2, SString sv[], int len)
{
	for (int i = 0; i < len; i++)
		sv[i] = "";

	SString s;
	int part = 0;
	int thisLineStart = PositionFromLine(line);
	int nextLineStart = PositionFromLine(line + 1);

	for (int pos = thisLineStart; pos < nextLineStart; pos++) 
	{
		if ((GetStyleAt(pos) == style1) || (GetStyleAt(pos) == style2)) 
		{
			char c[2];
			c[0] = GetCharAt(pos);
			c[1] = '\0';
			s += c;
		} 
		else if (s.length() > 0)
		{
			if (part < len)
			{
				sv[part++] = s;
			}
			s = "";
		}
	}

	if ((s.length() > 0) && (part < len)) 
	{
		sv[part++] = s;
	}

	return part;
}

bool CScriptEditorCtrl::RangeIsAllWhitespace(int start, int end)
{
	if (start < 0)
		start = 0;

	end = min(end, GetLength());

	for (int i = start; i < end; i++)
	{
		char c = GetCharAt(i);

		if ((c != ' ') && (c != '\t'))
			return false;
	}

	return true;
}

bool CScriptEditorCtrl::StartCallTip()
{
	m_nCurrentCallTip = 0;
	m_szCurrentCallTipWord = "";
	SString line = GetCurrentLine();
	int current = GetCaretInLine();
	int pos = GetCurrentPos();
	int braces = 0;

	do 
	{
		while (current > 0 && (braces || line[current - 1] != '(')) 
		{
			if (line[current - 1] == '(')
				braces--;
			else if (line[current - 1] == ')')
				braces++;

			current--;
			pos--;
		}

		if (current > 0)
		{
			current--;
			pos--;
		} 
		else
		{
			break;
		}

		while (current > 0 && isspace(line[current - 1]))
		{
			current--;
			pos--;
		}
	} 
	while (current > 0 && !IsIdentifierChar(line[current - 1]));

	if (current <= 0)
		return true;

	m_nStartCalltipWord = current - 1;

	while (m_nStartCalltipWord > 0 && 
		(IsIdentifierChar(line[m_nStartCalltipWord - 1]) ||
		(line[m_nStartCalltipWord - 1] == '.')))
	{
		--m_nStartCalltipWord;
	}

	line.change(current, 0);
	m_szCurrentCallTipWord = line.c_str() + m_nStartCalltipWord;
	m_szFunctionDefinition = "";
	FillFunctionDefinition(pos);
	return true;
}

void CScriptEditorCtrl::ContinueCallTip()
{
	pfc::string8 line = GetCurrentLine();
	int current = GetCaretInLine();
	int braces = 0;
	int commas = 0;

	for (int i = m_nStartCalltipWord; i < current; i++) 
	{
		if (line[i] == '(')
			braces++;
		else if (line[i] == ')' && braces > 0)
			braces--;
		else if (braces == 1 && line[i] == ',')
			commas++;
	}

	int startHighlight = 0;

	while (m_szFunctionDefinition[startHighlight] && m_szFunctionDefinition[startHighlight] != '(')
		startHighlight++;

	if (m_szFunctionDefinition[startHighlight] == '(')
		startHighlight++;

	while (m_szFunctionDefinition[startHighlight] && commas > 0)
	{
		if (m_szFunctionDefinition[startHighlight] == ',')
			commas--;
		// If it reached the end of the argument list it means that the user typed in more
		// arguments than the ones listed in the calltip
		if (m_szFunctionDefinition[startHighlight] == ')')
			commas = 0;
		else
			startHighlight++;
	}

	if (m_szFunctionDefinition[startHighlight] == ',')
		startHighlight++;

	int endHighlight = startHighlight;

	while (m_szFunctionDefinition[endHighlight] && 
		m_szFunctionDefinition[endHighlight] != ',' && 
		m_szFunctionDefinition[endHighlight] != ')')
	{
		endHighlight++;
	}

	CallTipSetHlt(startHighlight, endHighlight);
}

void CScriptEditorCtrl::FillFunctionDefinition(int pos /*= -1*/)
{
	if (pos > 0) 
	{
		m_nLastPosCallTip = pos;
	}

	if (m_apis.get_count())
	{
		pfc::string8_fast words;

		if (!GetNearestWords(words, m_szCurrentCallTipWord.get_ptr(), 
			m_szCurrentCallTipWord.get_length(), "("))
		{
			t_size calltip_pos = m_szCurrentCallTipWord.find_first(".");

			if (calltip_pos == pfc_infinite)
				return;

			m_szCurrentCallTipWord.remove_chars(0, calltip_pos + 1);

			if (!GetNearestWords(words, m_szCurrentCallTipWord.get_ptr(), 
				m_szCurrentCallTipWord.get_length(), "("))
			{
				return;
			}
		}

		// Should get current api definition
		const char * word = GetNearestWord(m_szCurrentCallTipWord.get_ptr(), 
			m_szCurrentCallTipWord.get_length(), 
			"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", m_nCurrentCallTip);

		if (word)
		{
			m_szFunctionDefinition = word;

			CallTipShow(m_nLastPosCallTip - m_szCurrentCallTipWord.get_length(), m_szFunctionDefinition.get_ptr());
			ContinueCallTip();
		}
	}
}

bool CScriptEditorCtrl::StartAutoComplete()
{
	pfc::string8 line = GetCurrentLine();
	int current = GetCaretInLine();

	int startword = current;

	while ((startword > 0) && (IsIdentifierChar(line[startword - 1]) || line[startword - 1] == '.'))
	{
		startword--;
	}

	pfc::string8 root;
	root.set_string(line.get_ptr() + startword, current - startword);

	if (m_apis.get_count() == 0)
		return false;

	pfc::string8_fast words;

	if (!GetNearestWords(words, root.get_ptr(), root.length(), "("))
		return false;

	AutoCShow(root.length(), words);

	return true;
}

int CScriptEditorCtrl::IndentOfBlock(int line)
{
	if (line < 0)
		return 0;

	int indentSize = GetIndent();
	int indentBlock = GetLineIndentation(line);
	int backLine = line;
	IndentationStatus indentState = isNone;

	int lineLimit = line - m_nStatementLookback;

	if (lineLimit < 0)
		lineLimit = 0;

	while ((backLine >= lineLimit) && (indentState == 0)) 
	{
		indentState = GetIndentState(backLine);

		if (indentState != 0) 
		{
			indentBlock = GetLineIndentation(backLine);

			if (indentState == isBlockStart) 
			{
				indentBlock += indentSize;
			}

			if (indentState == isBlockEnd)
			{
				if (indentBlock < 0)
					indentBlock = 0;
			}

			if ((indentState == isKeyWordStart) && (backLine == line))
				indentBlock += indentSize;
		}

		backLine--;
	}

	return indentBlock;
}

void CScriptEditorCtrl::AutomaticIndentation(char ch)
{
	Sci_CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	int curLine = LineFromPosition(GetCurrentPos());
	int thisLineStart = PositionFromLine(curLine);
	int indentSize = GetIndent();
	int indentBlock = IndentOfBlock(curLine - 1);

	if (curLine > 0)
	{
		// Smart indent?
		pfc::array_t<char> linebuf;
		bool foundBrace = false;
		int prevLineLength = LineLength(curLine - 1);
		int len = GetCurLine(0, 0);
		int slen = 0;

		linebuf.set_size(prevLineLength + 2);
		GetLine(curLine - 1, linebuf.get_ptr());
		linebuf[prevLineLength] = 0;
		slen = strlen(linebuf.get_ptr());

		for (int pos = slen - 1; pos >= 0 && linebuf[pos]; --pos)
		{
			char c = linebuf[pos];

			if (c == '\t' || c == ' ' || c == '\r' || c == '\n')
			{
				continue;
			}
			else if (c == '{' || c == '[' || c == '(')
			{
				foundBrace = true;
			}

			break;
		}

		if (foundBrace)
			indentBlock += indentSize;
	}

	if (ch == '}') 
	{	
		// Dedent maybe
		if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) 
		{
			SetIndentation(curLine, indentBlock - indentSize);
		}
	} 
	else if (ch == '{') 
	{
		// Dedent maybe if first on line and previous line was starting keyword
		if ((GetIndentState(curLine - 1) == isKeyWordStart))
		{
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) 
			{
				SetIndentation(curLine, indentBlock - indentSize);
			}
		}
	} 
	else if ((ch == '\r' || ch == '\n') && (selStart == thisLineStart))
	{	
		// Dedent previous line maybe
		SString controlWords[1];

		if (GetLinePartsInStyle(curLine - 1, m_BlockEnd.styleNumber, -1, controlWords, _countof(controlWords))) 
		{
			if (IsSymIncludes(m_BlockEnd, controlWords[0])) 
			{
				// Check if first keyword on line is an ender
				SetIndentation(curLine - 1, IndentOfBlock(curLine - 2) - indentSize);
				// Recalculate as may have changed previous line
				indentBlock = IndentOfBlock(curLine - 1);
			}
		}

		SetIndentation(curLine, indentBlock);
	}
}

bool CScriptEditorCtrl::FindBraceMatchPos(int &braceAtCaret, int &braceOpposite)
{
	bool isInside = false;
	int mask = (1 << GetStyleBits()) - 1;
	int caretPos = GetCurrentPos();

	braceAtCaret = -1;
	braceOpposite = -1;

	char charBefore = 0;
	char styleBefore = 0;
	int lengthDoc = GetLength();

	if ((lengthDoc > 0) && (caretPos > 0))
	{
		// Check to ensure not matching brace that is part of a multibyte character
		int posBefore = PositionBefore(caretPos);

		if (posBefore == (caretPos - 1))
		{
			charBefore = GetCharAt(posBefore);
			styleBefore = GetStyleAt(posBefore) & mask;
		}
	}

	// Priority goes to character before caret
	if (charBefore && IsBraceChar(charBefore))
	{
		braceAtCaret = caretPos - 1;
	}

	bool colonMode = false;
	long lexLanguage = GetLexer();

	bool isAfter = true;

	if (lengthDoc > 0 && (braceAtCaret < 0) && (caretPos < lengthDoc))
	{
		// No brace found so check other side
		// Check to ensure not matching brace that is part of a multibyte character
		char charAfter = GetCharAt(caretPos);
		char styleAfter =GetStyleAt(caretPos) & mask;

		if (charAfter && IsBraceChar(charAfter))
		{
			braceAtCaret = caretPos;
			isAfter = false;
		}
	}

	if (braceAtCaret >= 0)
	{
		if (colonMode)
		{
			int lineStart = LineFromPosition(braceAtCaret);
			int lineMaxSubord = GetLastChild(lineStart, -1);

			braceOpposite = GetLineEndPosition(lineMaxSubord);
		}
		else
		{
			braceOpposite = BraceMatch(braceAtCaret);
		}

		if (braceOpposite > braceAtCaret)
			isInside = isAfter;
		else
			isInside = !isAfter;
	}

	return isInside;
}

const char * CScriptEditorCtrl::GetNearestWord(const char *wordStart, int searchLen, SString wordCharacters /*= NULL*/, int wordIndex /*= -1*/)
{
	if (m_apis.get_count() == 0)
		return false;

	t_size index;

	if (m_apis.bsearch_t(StringComparePartialNC(searchLen), wordStart, index))
	{
		// Find first match
		t_size start = index;

		while ((start > 0) && 
			(StringComparePartialNC(searchLen)(m_apis[start - 1], wordStart) == 0))
		{
			--start;
		}

		// Find last match
		t_size end = index;

		while ((end < m_apis.get_count() - 1) &&
			(StringComparePartialNC(searchLen)(m_apis[end + 1], wordStart) == 0))
		{
			++end;
		}

		// Finds first word in a series of equal words
		for (t_size i = start; i <= end; ++i) 
		{
			const char * word = m_apis[i];

			if (!wordCharacters.contains(word[searchLen])) 
			{
				if (wordIndex <= 0) // Checks if a specific index was requested
					return word; // result must not be freed with free()

				--wordIndex;
			}
		}

		return NULL;
	}

	return NULL;
}

bool CScriptEditorCtrl::GetNearestWords(pfc::string_base & out, const char * wordStart, int searchLen, const char *separators)
{
	out.reset();

	if (m_apis.get_count() == 0)
		return false;

	bool status = false;

	while (!status && *separators) 
	{
		char otherSeparator = *separators;
		t_size index;

		if (m_apis.bsearch_t(StringComparePartialNC(searchLen), wordStart, index))
		{
			t_size pivot = index;
			status = true;

			// Find first match
			while ((pivot > 0) && 
				(StringComparePartialNC(searchLen)(m_apis[pivot - 1], wordStart) == 0))
			{
				--pivot;
			}

			// Grab each match
			while (pivot <= m_apis.get_count() - 1)
			{
				if (StringComparePartialNC(searchLen)(m_apis[pivot], wordStart) != 0)
					break;

				t_size wordlen = LengthWord(m_apis[pivot], otherSeparator) + 1;

				out.add_string(m_apis[pivot], wordlen);
				out.add_char(' ');

				++pivot;
			}

		}

		separators++;
	}

	return status;
}

DWORD CScriptEditorCtrl::GetPropertyColor(const char * key, bool * key_exist /*= NULL*/)
{
	pfc::array_t<char> buff;
	int len = GetPropertyExpanded(key, 0);  // Get property len

	if (key_exist)
		*key_exist = (len != 0);

	if (len == 0)
		return 0;

	// Allocate buffer
	buff.set_size(len + 1);
	buff[len] = 0;
	// Get property
	GetPropertyExpanded(key, buff.get_ptr());

	return ParseHex(buff.get_ptr());
}

void CScriptEditorCtrl::Init()
{
	// Reset to default p_style
	StyleResetDefault();

	// Common
	SetCodePage(SC_CP_UTF8);
	SetEOLMode(SC_EOL_CRLF);
	SetModEventMask(SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);
	UsePopUp(true);

	// Disable Ctrl + some char
	const int ctrlcode[22] =  {'Q', 'W', 'E', 'R', 'I', 'O', 'P', 'S', 'D', 'F',
		'G', 'H', 'J', 'K', 'L', 'B', 'N', 'M', 186, 187, 226
	};

	for (int i = 0; i < _countof(ctrlcode); ++i)
	{
		ClearCmdKey(MAKELONG(ctrlcode[i], SCMOD_CTRL));
	}

	// Disable Ctrl+Shift+some char
	for (int i = 48; i < 122; ++i)
	{
		ClearCmdKey(MAKELONG(i, SCMOD_CTRL|SCMOD_SHIFT));
	}

	// Shortcut keys
	AssignCmdKey(MAKELONG(SCK_NEXT, SCMOD_CTRL), SCI_PARADOWN);
	AssignCmdKey(MAKELONG(SCK_PRIOR, SCMOD_CTRL), SCI_PARAUP);
	AssignCmdKey(MAKELONG(SCK_NEXT, (SCMOD_CTRL | SCMOD_SHIFT)), SCI_PARADOWNEXTEND);
	AssignCmdKey(MAKELONG(SCK_PRIOR, (SCMOD_CTRL | SCMOD_SHIFT)), SCI_PARAUPEXTEND);
	AssignCmdKey(MAKELONG(SCK_HOME, SCMOD_NORM), SCI_VCHOMEWRAP);
	AssignCmdKey(MAKELONG(SCK_END, SCMOD_NORM), SCI_LINEENDWRAP);
	AssignCmdKey(MAKELONG(SCK_HOME, SCMOD_SHIFT), SCI_VCHOMEWRAPEXTEND);
	AssignCmdKey(MAKELONG(SCK_END, SCMOD_SHIFT), SCI_LINEENDWRAPEXTEND);

	// Tabs and indentation
	SetUseTabs(false); 
	SetTabIndents(false);
	SetBackSpaceUnIndents(true);
	SetTabWidth(4);
	SetIndent(4);

	// Scroll
	SetEndAtLastLine(true);
	SetScrollWidthTracking(true);
	SetScrollWidth(1);

	// Auto complete
	AutoCSetIgnoreCase(true);

	// Set embeded properties
	SetProperty("dir.base", helpers::get_fb2k_path());
	SetProperty("dir.component", helpers::get_fb2k_component_path());
	SetProperty("dir.profile", helpers::get_profile_path());

	// Load properties
	LoadProperties(g_sci_prop_sets.val());
}

void CScriptEditorCtrl::LoadProperties(const pfc::list_t<t_sci_prop_set> & data)
{
	for (t_size i = 0; i < data.get_count(); ++i)
	{
		SetProperty(data[i].key.get_ptr(), data[i].val.get_ptr());
	}
}

void CScriptEditorCtrl::SetContent(const char * text, bool clear_undo_buffer /*= false*/)
{
	SetText(text);
	ConvertEOLs(SC_EOL_CRLF);

	if (clear_undo_buffer)
		EmptyUndoBuffer();

	Colourise(0, -1);
	GrabFocus();
	TrackWidth();
}

void CScriptEditorCtrl::RestoreDefaultStyle()
{
	// Clear and reset all styles
	ClearDocumentStyle();
	StyleResetDefault();

	// enable line numbering
	SetMarginWidthN(1, 0);
	SetMarginWidthN(2, 0);
	SetMarginWidthN(3, 0);
	SetMarginWidthN(4, 0);
	SetMarginTypeN(0, SC_MARGIN_NUMBER);

	// Default styles
	SetAllStylesFromTable(default_style_table);

	// Additional styles
	// 
	bool sel_fore, sel_back, line_back;
	DWORD color;

	color = GetPropertyColor("style.selection.fore", &sel_fore);

	if (sel_fore)
		SetSelFore(true, color);
	else
		SetSelFore(false, 0);

	color = GetPropertyColor("style.selection.back", &sel_back);

	if (sel_back)
	{
		SetSelBack(true, color);
	}
	else
	{
		if (sel_fore)
			SetSelBack(true, RGB(0xc0, 0xc0, 0xc0));
	}

	SetSelAlpha(GetPropertyInt("style.selection.alpha", SC_ALPHA_NOALPHA));
	SetCaretFore(GetPropertyColor("style.caret.fore"));
	SetCaretWidth(GetPropertyInt("style.caret.width", 1));

	color = GetPropertyColor("style.caret.line.back", &line_back);

	if (line_back)
	{
		SetCaretLineVisible(true);
		SetCaretLineBack(color);
	}
	else
	{
		SetCaretLineVisible(false);
	}

	SetCaretLineBackAlpha(GetPropertyInt("style.caret.line.back.alpha", SC_ALPHA_NOALPHA));
}

void CScriptEditorCtrl::SetLanguage(const char * lang)
{
	if (strcmp(lang, "JScript") == 0 || strcmp(lang, "JScript9") == 0) 
	{
		SetJScript();
	}
	else if (strcmp(lang, "VBScript") == 0)
	{
		SetVBScript();
	}

	ReadAPI();
}

void CScriptEditorCtrl::SetJScript()
{
	const char js_keywords[] = "abstract boolean break byte case catch char class const continue"
							   " debugger default delete do double else enum export extends false final"
							   " finally float for function goto if implements import in instanceof int"
							   " interface long native new null package private protected public return"
							   " short static super switch synchronized this throw throws transient true"
							   " try typeof var void while with enum byvalue cast future generic inner"
							   " operator outer rest var Array Math RegExp window fb gdi utils plman";

	RestoreDefaultStyle();

	// Set lexer
	SetLexer(SCLEX_CPP);
	// Set keywords
	SetKeyWords(0, js_keywords);
	// Set styles
	SetAllStylesFromTable(js_style_table);
	// Colorise
	Colourise(0, -1);
}

void CScriptEditorCtrl::SetVBScript()
{
	const char vbs_keywords[] = "addressof alias and as attribute base begin binary boolean byref byte byval call case cdbl"
								" cint clng compare const csng cstr currency date decimal declare defbool defbyte defcur"
								" defdate defdbl defdec defint deflng defobj defsng defstr defvar dim do double each else"
								" elseif empty end enum eqv erase error event exit explicit false for friend function get"
								" global gosub goto if imp implements in input integer is len let lib like load lock long"
								" loop lset me mid midb mod new next not nothing null object on option optional or paramarray"
								" preserve print private property public raiseevent randomize redim rem resume return rset"
								" seek select set single static step stop string sub text then time to true type typeof"
								" unload until variant wend while with withevents xor window fb gdi utils plman";

	RestoreDefaultStyle();

	// Set lexer
	SetLexer(SCLEX_VBSCRIPT);
	// Set keywords
	SetKeyWords(0, vbs_keywords);
	// Set styles
	SetAllStylesFromTable(vbs_style_table);
	// Colorise
	Colourise(0, -1);
}

void CScriptEditorCtrl::TrackWidth()
{
	int max_width = 1;

	for (int i = 0; i < GetLineCount(); ++i)
	{
		// Get max width
		int pos = GetLineEndPosition(i);
		int width = PointXFromPosition(pos);

		if (width > max_width)
			max_width = width;
	}

	SetScrollWidth(max_width);
}

void CScriptEditorCtrl::SetAllStylesFromTable(const t_style_to_key_table table[])
{
	const char * key = NULL;

	for (int i = 0; table[i].style_num != -1; ++i)
	{
		if ((key = table[i].key) != NULL)
		{
			int style_num;
			pfc::array_t<char> definition;
			t_sci_editor_style style;
			int len;

			style_num = table[i].style_num;

			// Get property len
			len = GetPropertyExpanded(key, 0);

			if (len != 0)
			{
				// Alloc buffer
				definition.set_size(len + 1);
				definition[len] = 0;
				// Get property
				GetPropertyExpanded(key, definition.get_ptr());

				if (!ParseStyle(definition.get_ptr(), style))
					continue;

				// Set style
				if (style.flags & ESF_FONT)
					StyleSetFont(style_num, style.font.get_ptr());

				if (style.flags & ESF_SIZE)
					StyleSetSize(style_num, style.size);

				if (style.flags & ESF_FORE)
					StyleSetFore(style_num, style.fore);

				if (style.flags & ESF_BACK)
					StyleSetBack(style_num, style.back);

				if (style.flags & ESF_ITALICS)
					StyleSetItalic(style_num, style.italics);

				if (style.flags & ESF_BOLD)
					StyleSetBold(style_num, style.bold);

				if (style.flags & ESF_UNDERLINED)
					StyleSetUnderline(style_num, style.underlined);

				if (style.flags & ESF_CASEFORCE)
					StyleSetCase(style_num, style.case_force);
			}

			if (style_num == STYLE_DEFAULT)
				StyleClearAll();
		}
	}
}

void CScriptEditorCtrl::AutoMarginWidth()
{
	// Auto margin width
	int linenumwidth = 1;
	int marginwidth, oldmarginwidth;
	int linecount;

	linecount = GetLineCount();

	while (linecount >= 10)
	{
		linecount /= 10;
		++linenumwidth;
	}

	oldmarginwidth = GetMarginWidthN(0);
	marginwidth = 4 + linenumwidth * (TextWidth(STYLE_LINENUMBER, "9"));

	if (oldmarginwidth != marginwidth)
		SetMarginWidthN(0, marginwidth);
}

BOOL CScriptEditorCtrl::SubclassWindow(HWND hWnd)
{
	BOOL bRet = CScintillaCtrl::SubclassWindow(hWnd);

	if (bRet)
		Init();

	return bRet;
}

void CScriptEditorCtrl::SetIndentation(int line, int indent)
{
	if (indent < 0)
		return;

	Sci_CharacterRange crange = GetSelection();
	int posBefore = GetLineIndentPosition(line);
	SetLineIndentation(line, indent);
	int posAfter = GetLineIndentPosition(line);
	int posDifference = posAfter - posBefore;
	if (posAfter > posBefore) 
	{
		// Move selection on
		if (crange.cpMin >= posBefore) 
		{
			crange.cpMin += posDifference;
		}

		if (crange.cpMax >= posBefore) 
		{
			crange.cpMax += posDifference;
		}
	} 
	else if (posAfter < posBefore)
	{
		// Move selection back
		if (crange.cpMin >= posAfter) 
		{
			if (crange.cpMin >= posBefore)
				crange.cpMin += posDifference;
			else
				crange.cpMin = posAfter;
		}

		if (crange.cpMax >= posAfter) 
		{
			if (crange.cpMax >= posBefore)
				crange.cpMax += posDifference;
			else
				crange.cpMax = posAfter;
		}
	}

	SetSel(crange.cpMin, crange.cpMax);
}

void CScriptEditorCtrl::ReadAPI()
{
	m_apis.remove_all();

	pfc::string8 propname;
	pfc::array_t<char> propval;
	int lexer = GetLexer();

	switch (lexer)
	{
	case SCLEX_CPP:
		propname = "api.jscript";
		break;

	case SCLEX_VBSCRIPT:
		propname = "api.vbscript";
		break;

	default:
		return;
	}

	int len = GetPropertyExpanded(propname, 0);
	if (!len) return;
	propval.set_size(len + 1);
	GetPropertyExpanded(propname, propval.get_ptr());
	propval[len] = 0;

	// Replace ';' to 'zero'
	for (int i = 0; i < len; ++i)
	{
		if (propval[i] == ';')
			propval[i] = 0;
	}

	const char * api_filename = propval.get_ptr();
	const char * api_endfilename = api_filename + len;
	pfc::string8_fast content;

	while (api_filename < api_endfilename)
	{
		if (helpers::read_file(api_filename, content))
		{
			pfc::string_list_impl temp;
			pfc::splitStringByLines(temp, content);

			for (t_size i = 0; i < temp.get_count(); ++i)
			{
				if (IsIdentifierChar(*temp[i]))
					m_apis.add_item(temp[i]);
			}
		}
		else
		{
			console::formatter() << WSPM_NAME ": Warning: Could not load file " << api_filename;
		}

		api_filename += strlen(api_filename) + 1;
	}

	// Sort now
	m_apis.sort_remove_duplicates_t(StringCompareSpecial());
}

LRESULT CScriptEditorCtrl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	::PostMessage(::GetAncestor(m_hWnd, GA_PARENT), UWM_KEYDOWN, wParam, lParam);
	return TRUE;
}

LRESULT CScriptEditorCtrl::OnUpdateUI(LPNMHDR pnmn)
{
	// Match Brace
	int braceAtCaret = -1;
	int braceOpposite = -1;

	FindBraceMatchPos(braceAtCaret, braceOpposite);

	if (braceAtCaret != -1 && braceOpposite == -1)
	{
		BraceBadLight(braceAtCaret);
		SetHighlightGuide(0);
	}
	else
	{
		char chBrace = GetCharAt(braceAtCaret);

		BraceHighlight(braceAtCaret, braceOpposite);

		int columnAtCaret = GetColumn(braceAtCaret);
		int columnOpposite = GetColumn(braceOpposite);

		SetHighlightGuide(min(columnAtCaret, columnOpposite));
	}

	return 0;
}

LRESULT CScriptEditorCtrl::OnCharAdded(LPNMHDR pnmh)
{
	SCNotification * notification = (SCNotification *)pnmh;
	int ch = notification->ch;
	Sci_CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	int selEnd = crange.cpMax;

	if ((selEnd == selStart) && (selStart > 0))
	{
		if (CallTipActive()) 
		{
			switch (ch)
			{
			case ')':
				m_nBraceCount--;
				if (m_nBraceCount < 1)
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
		else if (AutoCActive())
		{
			if (ch == '(') 
			{
				m_nBraceCount++;
				StartCallTip();
			} 
			else if (ch == ')')
			{
				m_nBraceCount--;
			} 
			else if (!IsIdentifierChar(ch))
			{
				AutoCCancel();

				if (ch == '.')
					StartAutoComplete();
			} 
		} 
		else 
		{
			if (ch == '(')
			{
				m_nBraceCount = 1;
				StartCallTip();
			} 
			else 
			{
				AutomaticIndentation(ch);
				
				if (IsIdentifierChar(ch) || ch == '.')
					StartAutoComplete();
			}
		}
	}

	return 0;
}

LRESULT CScriptEditorCtrl::OnZoom(LPNMHDR pnmn)
{
	AutoMarginWidth();

	return 0;
}

LRESULT CScriptEditorCtrl::OnChange(UINT uNotifyCode, int nID, HWND wndCtl)
{
	AutoMarginWidth();

	return 0;
}
