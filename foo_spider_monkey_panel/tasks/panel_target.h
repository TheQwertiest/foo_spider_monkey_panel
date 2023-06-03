#pragma once

namespace smp::panel
{

class PanelWindow;

}

namespace smp
{

class PanelTarget final
{
public:
    [[nodiscard]] PanelTarget( panel::PanelWindow& panel );

    [[nodiscard]] HWND GetHwnd();

    [[nodiscard]] panel::PanelWindow* GetPanel();
    void UnlinkPanel();

private:
    panel::PanelWindow* pPanel_ = nullptr;
    HWND hWnd_ = nullptr;
};

} // namespace smp
