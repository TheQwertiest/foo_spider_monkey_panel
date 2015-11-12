// This code is obtained from Huxtable.com
// Ported by T.P Wang
/*
Copyright 2006 Jerry Huxtable

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "stdafx.h"
#include "boxblurfilter.h"


void box_blur_filter::filter(Gdiplus::Bitmap & p_img)
{
	int width = p_img.GetWidth();
	int height = p_img.GetHeight();

	// Optimized for memory alloc.
	unsigned count = width * height * sizeof(int);
	bool is_big_chunk = count >= (2 << 20);
	int * pixels_in = NULL;
	int * pixels_out = reinterpret_cast<int *>(
		is_big_chunk ?
		VirtualAlloc(NULL, count, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE) : malloc(count));

	Gdiplus::BitmapData bmpdata;
	Gdiplus::Rect rect;

	rect.X = rect.Y = 0;
	rect.Width = width;
	rect.Height = height;

	if (pixels_out && p_img.LockBits(&rect, 
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
		PixelFormat32bppPARGB, &bmpdata) == Gdiplus::Ok)
	{
		pixels_in = reinterpret_cast<int *>(bmpdata.Scan0);

		for (int i = 0; i < m_iterations; ++i)
		{
			blur(pixels_in, pixels_out, width, height, m_radius);
			blur(pixels_out, pixels_in, height, width, m_radius);
		}

		p_img.UnlockBits(&bmpdata);
	}

	if (pixels_out)
		is_big_chunk ? VirtualFree(pixels_out, 0, MEM_RELEASE) : free(pixels_out);
}

void box_blur_filter::blur(const int * in, int * out, int width, int height, int radius)
{
	int width_minus_1 = width - 1;
	int table_size = radius * 2 + 1;
	int count = table_size * 256;
	int * divide = reinterpret_cast<int *>(malloc(count * sizeof(int)));

	if (!divide) return;

	for (int i = 0; i < count; ++i)
	{
		divide[i] = i / table_size;
	}

	int in_idx = 0;

	for (int y = 0; y < height; ++y)
	{
		int out_idx = y;
		int ta = 0, tr = 0, tg = 0, tb = 0;

		for (int i = -radius; i <= radius; ++i)
		{
			int rgb = in[in_idx + clamp(i, 0, width - 1)];

			ta += get_color_alpha(rgb);
			tr += get_color_red(rgb);
			tg += get_color_green(rgb);
			tb += get_color_blue(rgb);
		}

		for (int x = 0; x < width; ++x)
		{
			out[out_idx] = make_argb(divide[ta], divide[tr], divide[tg], divide[tb]);

			int i1 = x + radius + 1;

			if (i1 > width_minus_1)
				i1 = width_minus_1;

			int i2 = x - radius;

			if (i2 < 0)
				i2 = 0;

			int rgb1 = in[in_idx + i1];
			int rgb2 = in[in_idx + i2];

			ta += ((rgb1 >> ALPHA_SHIFT) & 0xff) - ((rgb2 >> ALPHA_SHIFT) & 0xff);
			tr += ((rgb1 & 0xff0000) - (rgb2 & 0xff0000)) >> RED_SHIFT;
			tg += ((rgb1 & 0xff00) - (rgb2 & 0xff00)) >> GREEN_SHIFT;
			tb += (rgb1 & 0xff) - (rgb2 & 0xff);
			out_idx += height;
		}

		in_idx += width;
	}

	free(divide);
}
