#pragma once

#include <resources/resource.h>

namespace smp::ui
{

class CMenuEditWith
    : public CMenu
{
public:
    static BOOL ProcessWindowMessage( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _Inout_ LRESULT& lResult, _In_ DWORD dwMsgMapID = 0 );

public:
    void InitMenu();
    static void OnEditClick( UINT uNotifyCode, int nID, CWindow wndCtl );

private:
    std::vector<CBitmap> currentBmps_;
};

} // namespace smp::ui
