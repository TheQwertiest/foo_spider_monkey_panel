#pragma once

namespace smp::utils
{

/*
The Stack Blur Algorithm was invented by Mario Klingemann,
mario@quasimondo.com and described here:
http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

This is C++ RGBA (32 bit color) multi-threaded version
by Victor Laskin (victor.laskin@gmail.com)
More details: http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp
*/

void stack_blur_filter( Gdiplus::Bitmap& img, int radius );

} // namespace smp::utils
