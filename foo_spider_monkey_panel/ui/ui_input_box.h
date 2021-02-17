#pragma once

#include <resources/resource.h>

namespace smp::ui
{

class CInputBox : public CDialogImpl<CInputBox>
{
public:
    CInputBox( const char* p_prompt, const char* p_caption, const char* p_value = "" );

    BEGIN_MSG_MAP( CInputBox )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_COMMAND( OnCommand )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_INPUT
    };

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnCommand( UINT codeNotify, int id, HWND hwndCtl );
    qwr::u8string GetValue();

private:
    qwr::u8string m_prompt;
    qwr::u8string m_caption;
    qwr::u8string m_value;
};

} // namespace smp::ui
