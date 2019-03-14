#pragma once

namespace smp
{

class HeartbeatWindow
{
public:
    ~HeartbeatWindow();
    HeartbeatWindow( HeartbeatWindow& ) = delete;
    HeartbeatWindow& operator=( HeartbeatWindow& ) = delete;

    /// @throw smp::SmpException
    static std::unique_ptr<HeartbeatWindow> Create();

    HWND GetHwnd() const;

private:
    HeartbeatWindow( HWND hWnd );

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

private:
    HWND hWnd_ = nullptr;
};

} // namespace smp
