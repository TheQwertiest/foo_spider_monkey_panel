#pragma once

namespace smp::panel
{

class PanelWindow;

}

namespace smp::panel
{

class PanelWindowGraphics
{
public:
    PanelWindowGraphics( const PanelWindow& parent );
    ~PanelWindowGraphics();

public:
    void Reinitialize();
    void Finalize();

    void PaintWithCallback( const std::function<void( Gdiplus::Graphics& )>& callback );
    void PaintFallback();
    void PaintPseudoTransparentBackground();
    void PaintErrorSplash();

    [[nodiscard]] HDC GetHDC() const;
    [[nodiscard]] uint32_t GetWidth() const;
    [[nodiscard]] uint32_t GetHeight() const;

private:
    void PaintBackground( CPaintDC& paintDc, CDC& memDc );

private:
    const PanelWindow& parent_;

    CWindow wnd_;
    CClientDC cDc_;

    CBitmap bmp_;
    CBitmap bmpBg_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

} // namespace smp::panel
