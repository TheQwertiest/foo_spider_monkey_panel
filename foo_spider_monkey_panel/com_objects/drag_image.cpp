// Copied from https://github.com/reupen/ui_helpers
// Copyright (c) Reupen Shah 2003-2017
// All rights reserved.

#include <stdafx.h>
#include "drag_image.h"

#include <com_objects/handle.h>

namespace uih
{
// Ripped from win32_helpers.cpp

SIZE get_system_dpi()
{
    HDC dc = GetDC( nullptr );
    SIZE ret = { GetDeviceCaps( dc, LOGPIXELSX ), GetDeviceCaps( dc, LOGPIXELSY ) };
    ReleaseDC( nullptr, dc );
    return ret;
}

SIZE get_system_dpi_cached()
{
    static const SIZE size = get_system_dpi();
    return size;
}

} // namespace uih

namespace uih
{

// Only used in non-themed mode – if theming is active, the shell draws the background for us
void draw_drag_image_background( HWND wnd, bool isThemed, HTHEME theme, HDC dc, COLORREF selectionBackgroundColour, const RECT& rc )
{
    int themeState = 0;

    bool useTheming = theme && isThemed && IsThemePartDefined( theme, DD_IMAGEBG, themeState );

    if ( useTheming )
    {
        {
            if ( IsThemeBackgroundPartiallyTransparent( theme, DD_IMAGEBG, themeState ) )
                DrawThemeParentBackground( wnd, dc, &rc );
            DrawThemeBackground( theme, dc, DD_IMAGEBG, themeState, &rc, nullptr );
        }
    }
    else
    {
        FillRect( dc, &rc, gdi_object_t<HBRUSH>::ptr_t( CreateSolidBrush( selectionBackgroundColour ) ) );
    }
}
void draw_drag_image_label( HWND wnd, bool isThemed, HTHEME theme, HDC dc, const RECT& rc, COLORREF selectionTextColour, const char* text )
{
    int theme_state = 0;
    bool useTheming = theme && isThemed && IsThemePartDefined( theme, DD_TEXTBG, theme_state );
    pfc::stringcvt::string_os_from_utf8 wtext( text );
    DWORD text_flags = DT_CENTER | DT_WORDBREAK;
    RECT rc_text = { 0 };

    if ( useTheming )
    {
        GetThemeTextExtent( theme, dc, DD_TEXTBG, theme_state, wtext, -1, text_flags, &rc, &rc_text );
    }
    else
    {
        rc_text = rc;
        DrawText( dc, wtext, -1, &rc_text, text_flags | DT_CALCRECT );
    }

    auto x_offset = ( RECT_CX( rc ) - RECT_CX( rc_text ) ) / 2;
    auto y_offset = ( RECT_CY( rc ) - RECT_CY( rc_text ) ) / 2;
    rc_text.left += x_offset;
    rc_text.right += x_offset;
    rc_text.top += y_offset;
    rc_text.bottom += y_offset;

    if ( useTheming )
    {
        MARGINS margins = { 0 };
        GetThemeMargins( theme, dc, DD_TEXTBG, theme_state, TMT_CONTENTMARGINS, &rc_text, &margins );

        RECT background_rect = rc_text;
        background_rect.left -= margins.cxLeftWidth;
        background_rect.top -= margins.cyTopHeight;
        background_rect.bottom += margins.cyBottomHeight;
        background_rect.right += margins.cxRightWidth;

        if ( IsThemeBackgroundPartiallyTransparent( theme, DD_TEXTBG, 0 ) )
            DrawThemeParentBackground( wnd, dc, &background_rect );
        DrawThemeBackground( theme, dc, DD_TEXTBG, theme_state, &background_rect, nullptr );
        DrawThemeText( theme, dc, DD_TEXTBG, theme_state, wtext, -1, text_flags, 0, &rc_text );
    }
    else
    {
        auto previousColour = GetTextColor( dc );
        auto previousBackgroundMode = GetBkMode( dc );
        SetTextColor( dc, selectionTextColour );
        SetBkMode( dc, TRANSPARENT );
        DrawText( dc, wtext, -1, &rc_text, text_flags );
        SetTextColor( dc, previousColour );
        SetBkMode( dc, previousBackgroundMode );
    }
}
void draw_drag_image_icon( HDC dc, const RECT& rc, HICON icon )
{
    // We may want to use better scaling.
    DrawIconEx( dc, 0, 0, icon, RECT_CX( rc ), RECT_CY( rc ), NULL, nullptr, DI_NORMAL );
}
SIZE GetDragImageContentSize( HDC dc, bool isThemed, HTHEME theme )
{
    auto sz = uih::get_system_dpi_cached();
    auto margins = MARGINS{ 0 };
    auto themeState = 0;
    auto hr = HRESULT{ S_OK };

    auto useTheming = theme && isThemed && IsThemePartDefined( theme, DD_IMAGEBG, themeState );
    if ( useTheming )
    {
        hr = GetThemePartSize( theme, dc, DD_IMAGEBG, themeState, nullptr, TS_DRAW, &sz );
        if ( SUCCEEDED( hr ) )
        {
            hr = GetThemeMargins( theme, dc, DD_IMAGEBG, themeState, TMT_CONTENTMARGINS, nullptr, &margins );
        }
        if ( SUCCEEDED( hr ) )
        {
            sz.cx -= margins.cxLeftWidth + margins.cxRightWidth;
            sz.cy -= margins.cyBottomHeight + margins.cyTopHeight;
        }
    }

    return sz;
}
BOOL create_drag_image( HWND wnd, bool isThemed, HTHEME theme, COLORREF selectionBackgroundColour,
                        COLORREF selectionTextColour, HICON icon, const LPLOGFONT font, bool showText, const char* text,
                        LPSHDRAGIMAGE lpsdi )
{
    HDC dc = GetDC( wnd );
    HDC dc_mem = CreateCompatibleDC( dc );

    SIZE size = GetDragImageContentSize( dc, isThemed, theme );
    RECT rc = { 0, 0, size.cx, size.cy };

    HBITMAP bm_mem = CreateCompatibleBitmap( dc, size.cx, size.cy ); // Not deleted - the shell takes ownership.
    HBITMAP bm_old = SelectBitmap( dc_mem, bm_mem );

    pfc::string8 drag_text;

    LOGFONT lf = *font;

    lf.lfWeight = FW_BOLD;
    // lf.lfQuality = NONANTIALIASED_QUALITY;
    HFONT fnt = CreateFontIndirect( &lf );

    HFONT font_old = SelectFont( dc_mem, fnt );

    if ( !isThemed || !theme )
    {
        draw_drag_image_background( wnd, isThemed, theme, dc_mem, selectionBackgroundColour, rc );
    }

    if ( icon )
        uih::draw_drag_image_icon( dc_mem, rc, icon );

    // Draw label
    if ( showText )
        uih::draw_drag_image_label( wnd, isThemed, theme, dc_mem, rc, selectionTextColour, text );

    SelectFont( dc_mem, font_old );
    DeleteFont( fnt );

    SelectObject( dc_mem, bm_old );
    DeleteDC( dc_mem );
    ReleaseDC( wnd, dc );

    lpsdi->sizeDragImage.cx = size.cx;
    lpsdi->sizeDragImage.cy = size.cy;
    lpsdi->ptOffset.x = size.cx / 2;
    lpsdi->ptOffset.y = size.cy - size.cy / 10;
    lpsdi->hbmpDragImage = bm_mem;
    lpsdi->crColorKey = 0xffffffff;

    return TRUE;
}

} // namespace uih
