#include "stdafx.h"
#include "gdiplus_helpers.h"

namespace gdiplus_helpers
{
	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap * bitmap_ptr)
	{
		BITMAP bm;
		Gdiplus::Rect rect;
		Gdiplus::BitmapData bmpdata;
		HBITMAP hBitmap;

		rect.X = rect.Y = 0;
		rect.Width = bitmap_ptr->GetWidth();
		rect.Height = bitmap_ptr->GetHeight();

		if (bitmap_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata) != Gdiplus::Ok)
		{
			// Error
			return NULL;
		}

		bm.bmType       = 0;
		bm.bmWidth      = bmpdata.Width;
		bm.bmHeight     = bmpdata.Height;
		bm.bmWidthBytes = bmpdata.Stride;
		bm.bmPlanes     = 1;
		bm.bmBitsPixel  = 32;
		bm.bmBits       = bmpdata.Scan0;

		hBitmap = CreateBitmapIndirect(&bm);
		bitmap_ptr->UnlockBits(&bmpdata);
		return hBitmap;
	}
}