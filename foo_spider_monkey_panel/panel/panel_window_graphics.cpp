#include <stdafx.h>

#include "panel_window_graphics.h"

#include <graphics/gdi/object_selector.h>
#include <panel/panel_window.h>

namespace smp::panel
{

PanelWindowGraphics::PanelWindowGraphics( const PanelWindow& parent )
    : parent_( parent )
    , wnd_( parent.GetHWND() )
    , cDc_( parent.GetHWND() )
{
}

PanelWindowGraphics::~PanelWindowGraphics()
{
    ::ReleaseDC( wnd_, cDc_ );
}

void PanelWindowGraphics::Reinitialize()
{
    CRect rc;
    wnd_.GetClientRect( &rc );
    width_ = rc.Width();
    height_ = rc.Height();

    Finalize();

    bmp_.CreateCompatibleBitmap( cDc_, width_, height_ );

    if ( parent_.GetPanelConfig().panelSettings.isPseudoTransparent )
    {
        bmpBg_.CreateCompatibleBitmap( cDc_, width_, height_ );
    }
}

void PanelWindowGraphics::Finalize()
{
    if ( bmp_ )
    {
        bmp_.DeleteObject();
    }

    if ( bmpBg_ )
    {
        bmpBg_.DeleteObject();
    }
}

void PanelWindowGraphics::PaintWithCallback( const std::function<void( Gdiplus::Graphics& )>& callback )
{
    CPaintDC paintDc{ wnd_ };
    if ( !paintDc || !bmp_ )
    {
        return;
    }

    const CRect updateRc{ paintDc.m_ps.rcPaint };

    CDC memDc{ CreateCompatibleDC( paintDc ) };
    GdiObjectSelector autoBmp( memDc, bmp_.m_hBitmap );

    PaintBackground( paintDc, memDc );

    {
        Gdiplus::Graphics gr( memDc );

        // SetClip() may improve performance slightly
        gr.SetClip( Gdiplus::Rect{ updateRc.left,
                                   updateRc.top,
                                   updateRc.Width(),
                                   updateRc.Height() } );

        isPainting_ = true;
        callback( gr );
        isPainting_ = false;
    }

    BitBlt( paintDc, 0, 0, width_, height_, memDc, 0, 0, SRCCOPY );
}

void PanelWindowGraphics::PaintFallback()
{
    CPaintDC paintDc{ wnd_ };
    if ( !paintDc || !bmp_ )
    {
        return;
    }

    const CRect updateRc{ paintDc.m_ps.rcPaint };

    CDC memDc{ CreateCompatibleDC( paintDc ) };
    GdiObjectSelector autoBmp( memDc, bmp_.m_hBitmap );

    PaintBackground( paintDc, memDc );

    BitBlt( paintDc, 0, 0, width_, height_, memDc, 0, 0, SRCCOPY );
}

void PanelWindowGraphics::PaintPseudoTransparentBackground()
{
    CWindow parentWnd = GetAncestor( wnd_, GA_PARENT );

    // HACK: for Tab control
    // Find siblings
    HWND hWnd = nullptr;
    while ( ( hWnd = FindWindowEx( parentWnd, hWnd, nullptr, nullptr ) ) )
    {
        if ( hWnd == wnd_ )
        {
            continue;
        }
        std::array<wchar_t, 64> buff;
        GetClassName( hWnd, buff.data(), buff.size() );
        if ( wcsstr( buff.data(), L"SysTabControl32" ) )
        {
            parentWnd = hWnd;
            break;
        }
    }

    CRect updateRc;
    wnd_.GetUpdateRect( &updateRc, FALSE );

    CRect childRc{ 0, 0, static_cast<int>( width_ ), static_cast<int>( height_ ) };
    CRgn childRng{ ::CreateRectRgnIndirect( &childRc ) };
    {
        CRgn rgn{ ::CreateRectRgnIndirect( &updateRc ) };
        childRng.CombineRgn( rgn, RGN_DIFF );
    }

    CPoint pt{ 0, 0 };
    wnd_.ClientToScreen( &pt );
    parentWnd.ScreenToClient( &pt );

    CRect parentRc{ childRc };
    wnd_.ClientToScreen( &parentRc );
    parentWnd.ScreenToClient( &parentRc );

    // Force parent repaint
    wnd_.SetWindowRgn( childRng, FALSE );
    parentWnd.RedrawWindow( &parentRc, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW );

    {
        // Background bitmap
        CClientDC parentDc{ parentWnd };
        CDC bgDc{ ::CreateCompatibleDC( parentDc ) };
        GdiObjectSelector autoBmp( bgDc, bmpBg_.m_hBitmap );

        // Paint background
        bgDc.BitBlt( childRc.left, childRc.top, childRc.Width(), childRc.Height(), parentDc, pt.x, pt.y, SRCCOPY );
    }

    wnd_.SetWindowRgn( nullptr, FALSE );
}

void PanelWindowGraphics::PaintErrorSplash()
{
    CPaintDC paintDc{ wnd_ };
    if ( !paintDc || !bmp_ )
    {
        return;
    }

    const CRect updateRc{ paintDc.m_ps.rcPaint };

    CDC memDc{ CreateCompatibleDC( paintDc ) };
    GdiObjectSelector autoBmp( memDc, bmp_.m_hBitmap );

    CDCHandle cdc{ memDc.m_hDC };
    CFont font;
    font.CreateFont( 20,
                     0,
                     0,
                     0,
                     FW_BOLD,
                     FALSE,
                     FALSE,
                     FALSE,
                     DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE,
                     L"Tahoma" );
    GdiObjectSelector autoFontSelector( cdc, font.m_hFont );

    LOGBRUSH lbBack = { BS_SOLID, RGB( 225, 60, 45 ), 0 };
    CBrush brush;
    brush.CreateBrushIndirect( &lbBack );

    CRect rc{ 0, 0, static_cast<int>( width_ ), static_cast<int>( height_ ) };
    cdc.FillRect( &rc, brush );
    cdc.SetBkMode( TRANSPARENT );

    cdc.SetTextColor( RGB( 255, 255, 255 ) );
    cdc.DrawText( L"Aw, crashed :(", -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );

    BitBlt( paintDc, 0, 0, width_, height_, memDc, 0, 0, SRCCOPY );
}

HDC PanelWindowGraphics::GetHDC() const
{
    return cDc_;
}

uint32_t PanelWindowGraphics::GetHeight() const
{
    return height_;
}

uint32_t PanelWindowGraphics::GetWidth() const
{
    return width_;
}

void PanelWindowGraphics::PaintBackground( CPaintDC& paintDc, CDC& memDc )
{
    const CRect updateRc{ paintDc.m_ps.rcPaint };

    if ( parent_.GetPanelConfig().panelSettings.isPseudoTransparent )
    {
        CDC bgDc{ CreateCompatibleDC( paintDc ) };
        GdiObjectSelector autoBgBmp( bgDc, bmpBg_.m_hBitmap );

        memDc.BitBlt( updateRc.left,
                      updateRc.top,
                      updateRc.Width(),
                      updateRc.Height(),
                      bgDc,
                      updateRc.left,
                      updateRc.top,
                      SRCCOPY );
    }
    else
    {
        CRect rc{ 0, 0, static_cast<int>( width_ ), static_cast<int>( height_ ) };
        memDc.FillRect( &rc, (HBRUSH)( COLOR_WINDOW + 1 ) );
    }
}

bool PanelWindowGraphics::IsPaintInProgress() const
{
    return isPainting_;
}

} // namespace smp::panel
