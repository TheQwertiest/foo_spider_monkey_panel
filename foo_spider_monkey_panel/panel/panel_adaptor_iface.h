#pragma once

namespace smp::panel
{

enum class PanelType : uint8_t
{
    CUI = 0,
    DUI = 1
};

class IPanelAdaptor
{
public:
    virtual ~IPanelAdaptor(){};

    [[nodiscard]] virtual PanelType GetPanelType() const = 0;

    [[nodiscard]] virtual DWORD GetColour( unsigned type, const GUID& guid ) = 0;
    [[nodiscard]] virtual HFONT GetFont( unsigned type, const GUID& guid ) = 0;
    virtual void OnSizeLimitChanged( LPARAM lp ) = 0;
    virtual LRESULT OnMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) = 0;
};

} // namespace smp::panel
