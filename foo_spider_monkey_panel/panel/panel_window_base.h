#pragma once

namespace smp::panel
{

class IPanelWindowBase
{
public:
    virtual ~IPanelWindowBase(){};

    virtual DWORD GetColour( unsigned type, const GUID& guid ) = 0;
    virtual HFONT GetFont( unsigned type, const GUID& guid ) = 0;
    virtual LRESULT on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) = 0;
    virtual void notify_size_limit_changed( LPARAM lp ) = 0;
};

} // namespace smp::panel
