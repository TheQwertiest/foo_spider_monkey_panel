#pragma once

namespace smp
{

class HeartbeatWindow
{
public:
    ~HeartbeatWindow();
    HeartbeatWindow( HeartbeatWindow& ) = delete;
    HeartbeatWindow& operator=( HeartbeatWindow& ) = delete;

    /// @throw qwr::QwrException
    static [[nodiscard]] std::unique_ptr<HeartbeatWindow> Create();

    [[nodiscard]] HWND GetHwnd() const;

private:
    HeartbeatWindow( HWND hWnd );

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

private:
    HWND hWnd_ = nullptr;
};

} // namespace smp
