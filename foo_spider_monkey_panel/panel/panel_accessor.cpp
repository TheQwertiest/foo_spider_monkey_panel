#include <stdafx.h>

#include "panel_accessor.h"

#include <panel/panel_window.h>

namespace smp::panel
{

PanelAccessor::PanelAccessor( panel::PanelWindow& panel )
    : pPanel_( &panel )
    , hWnd_( panel.GetHWND() )
{
}

HWND PanelAccessor::GetHwnd()
{
    return hWnd_;
}

panel::PanelWindow* PanelAccessor::GetPanel()
{
    assert( core_api::is_main_thread() );
    return pPanel_;
}

void PanelAccessor::UnlinkPanel()
{
    pPanel_ = nullptr;
    hWnd_ = nullptr;
}

} // namespace smp::panel
