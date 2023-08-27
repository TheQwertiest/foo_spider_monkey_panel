#pragma once

namespace smp::panel
{

class PanelWindow;

}

namespace smp::panel
{

class PanelAccessor final
{
public:
    [[nodiscard]] PanelAccessor( panel::PanelWindow& panel );

    [[nodiscard]] HWND GetHwnd();

    [[nodiscard]] panel::PanelWindow* GetPanel();
    void UnlinkPanel();

private:
    panel::PanelWindow* pPanel_ = nullptr;
    HWND hWnd_ = nullptr;
};

} // namespace smp::panel
