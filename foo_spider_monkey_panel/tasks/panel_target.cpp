#include <stdafx.h>

#include "panel_target.h"

#include <panel/panel_window.h>

namespace smp
{

PanelTarget::PanelTarget( panel::PanelWindow& panel )
    : pPanel_( &panel )
    , hWnd_( panel.GetHWND() )
{
}

HWND PanelTarget::GetHwnd()
{
    return hWnd_;
}

panel::PanelWindow* PanelTarget::GetPanel()
{
    assert( core_api::is_main_thread() );
    return pPanel_;
}

void PanelTarget::UnlinkPanel()
{
    pPanel_ = nullptr;
    hWnd_ = nullptr;
}

} // namespace smp
