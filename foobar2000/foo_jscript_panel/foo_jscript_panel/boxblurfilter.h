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
#pragma once

class box_blur_filter
{
public:
	box_blur_filter() : m_radius(1), m_iterations(1)
	{
	}

	inline void set_op(int p_radius = 1, int p_iterations = 1) throw()
	{
		m_radius = p_radius;
		m_iterations = p_iterations;
	}

	void filter(Gdiplus::Bitmap & p_img) throw();

	static void blur(const int * in, int * out, int width, int height, int radius) throw();

public:
	static inline BYTE get_color_alpha(DWORD color)
	{
		return (color >> ALPHA_SHIFT) & 0xff;
	}

	static inline BYTE get_color_red(DWORD color)
	{
		return (color >> RED_SHIFT) & 0xff;
	}

	static inline BYTE get_color_green(DWORD color)
	{
		return (color >> GREEN_SHIFT) & 0xff;
	}

	static inline BYTE get_color_blue(DWORD color)
	{
		return (color >> BLUE_SHIFT) & 0xff;
	}

	template <typename T>
	static inline T clamp(T x, T l, T r)
	{
		return (x < l) ? l : ((x > r) ? r : x);
	}

	static inline DWORD make_argb(BYTE a, BYTE r, BYTE g, BYTE b)
	{
		return (a << ALPHA_SHIFT) | (b <<  BLUE_SHIFT) | (g << GREEN_SHIFT) | (r << RED_SHIFT);
	}

private:
	int m_radius;
	int m_iterations;
};
