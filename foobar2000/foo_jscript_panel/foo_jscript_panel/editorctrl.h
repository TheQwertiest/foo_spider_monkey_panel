// Stolen from SciTE
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
#pragma once

#include "wtlscintilla.h"
#include "SString.h"
#include "user_message.h"


enum IndentationStatus 
{
	isNone,		// no effect on indentation
	isBlockStart,	// indentation block begin such as "{" or VB "function"
	isBlockEnd,	// indentation end indicator such as "}" or VB "end"
	isKeyWordStart	// Keywords that cause indentation
};

struct StyleAndWords
{
	int styleNumber;
	SString words;
	bool IsEmpty() const { return words.length() == 0; }
	bool IsSingleChar() const { return words.length() == 1; }
};

// forward declaration
struct t_sci_prop_set;

struct t_style_to_key_table
{
	int style_num;
	const char * key;
};

class CScriptEditorCtrl : public CScintillaCtrl
{
public:
	CScriptEditorCtrl()
	{
		m_nBraceCount = 0;
		m_nCurrentCallTip = 0;
		m_nStartCalltipWord = 0;
		m_nLastPosCallTip = 0;
		m_nStatementLookback = 10;
	}

	// Operations and Implementation
	Sci_CharacterRange GetSelection();
	int GetCaretInLine();
	pfc::string8 GetCurrentLine();
	IndentationStatus GetIndentState(int line);
	unsigned int GetLinePartsInStyle(int line, int style1, int style2, SString sv[], int len);
	bool RangeIsAllWhitespace(int start, int end);
	DWORD GetPropertyColor(const char * key, bool * key_exist = NULL);
	void Init();
	void LoadProperties(const pfc::list_t<t_sci_prop_set> & data);
	void SetContent(const char * text, bool clear_undo_buffer = false);
	void RestoreDefaultStyle();
	void SetLanguage(const char * lang);
	void SetJScript();
	void SetVBScript();	
	void TrackWidth();
	void SetAllStylesFromTable(const t_style_to_key_table table[]);
	void AutoMarginWidth();
	void ReadAPI();
	BOOL SubclassWindow(HWND hWnd);

	bool StartCallTip();
	void ContinueCallTip();
	void FillFunctionDefinition(int pos = -1);
	bool StartAutoComplete();
	int IndentOfBlock(int line);
	void AutomaticIndentation(char ch);
	bool FindBraceMatchPos(int &braceAtCaret, int &braceOpposite);
	const char * GetNearestWord(const char *wordStart, int searchLen, SString wordCharacters = NULL, int wordIndex = -1);
	bool GetNearestWords(pfc::string_base & out, const char * wordStart, int searchLen, const char *separators);
	void SetIndentation(int line, int indent);

	// Message map and handlers
	BEGIN_MSG_MAP(CScriptEditorCtrl)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(SCN_UPDATEUI, OnUpdateUI)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(SCN_CHARADDED, OnCharAdded)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(SCN_ZOOM, OnZoom)
		REFLECTED_COMMAND_CODE_HANDLER_EX(SCEN_CHANGE, OnChange)
	END_MSG_MAP()

	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnUpdateUI(LPNMHDR pnmn);
	LRESULT OnCharAdded(LPNMHDR pnmh);
	LRESULT OnZoom(LPNMHDR pnmn);
	LRESULT OnChange(UINT uNotifyCode, int nID, HWND wndCtl);

private:
	int m_nBraceCount;
	int m_nCurrentCallTip;
	int m_nStartCalltipWord;
	int m_nLastPosCallTip;
	int m_nStatementLookback;

	StyleAndWords m_BlockStart;
	StyleAndWords m_BlockEnd;
	StyleAndWords m_StatementEnd;
	StyleAndWords m_StatementIndent;

	pfc::string8 m_szCurrentCallTipWord;
	pfc::string8 m_szFunctionDefinition;

	pfc::list_t<pfc::string_simple> m_apis;
};
