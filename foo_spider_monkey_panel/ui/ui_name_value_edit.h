#pragma once

#include "resource.h"

namespace smp::ui
{

class CNameValueEdit : public CDialogImpl<CNameValueEdit>
{
public:
    CNameValueEdit( const char* p_name, const char* p_value );

    BEGIN_MSG_MAP( CNameValueEdit )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_COMMAND( OnCommand )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_NAME_VALUE
    };

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnCommand( UINT codeNotify, int id, HWND hwndCtl );

    std::u8string GetValue();

private:
    std::u8string m_name;
    std::u8string m_value;
};

} // namespace smp::ui
