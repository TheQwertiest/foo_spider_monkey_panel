// Copied from https://github.com/reupen/ui_helpers
// Copyright (c) Reupen Shah 2003-2017
// All rights reserved.

#pragma once

namespace uih
{

void draw_drag_image_background( HWND wnd, bool is_themed, HTHEME theme, HDC dc, COLORREF selection_background_colour, const RECT& rc );
void draw_drag_image_label( HWND wnd, bool is_themed, HTHEME theme, HDC dc, const RECT& rc, COLORREF selection_text_colour, const char* text );
void draw_drag_image_icon( HDC dc, const RECT& rc, HICON icon );
bool create_drag_image( HWND wnd, bool is_themed, HTHEME theme, COLORREF selection_background_colour,
                        COLORREF selection_text_colour, HICON icon, LPLOGFONT font, const char* text,
                        Gdiplus::Bitmap* pUserImage, LPSHDRAGIMAGE lpsdi );

} // namespace uih
