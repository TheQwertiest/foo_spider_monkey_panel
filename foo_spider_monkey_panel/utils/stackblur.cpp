// The Stack Blur Algorithm was invented by Mario Klingemann,
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

// This is C++ RGBA (32 bit color) multi-threaded version
// by Victor Laskin (victor.laskin@gmail.com)
// More details: http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp

#include "stdafx.h"
#include "stackblur.h"

namespace
{

constexpr unsigned int kColourCount = 4; // 0 - r, 1 - g, 2 - b, 3 - a
using ColourArray = std::array<unsigned long, kColourCount>;


static unsigned short const stackblur_mul[255] =
{
    512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
    454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
    482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
    437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
    497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
    320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
    446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
    329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
    505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
    399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
    324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
    268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
    451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
    385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
    332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
    289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
};

static unsigned char const stackblur_shr[255] =
{
     9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
    17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
};

void stackblur_by_segment( unsigned char* src,  ///< input image data
                           unsigned int w,      ///< image width
                           unsigned int h,      ///< image height
                           unsigned int radius, ///< blur intensity (should be in 2..254 range)
                           bool isLineSegment,  ///< false, means column segment
                           unsigned int segmentStart,
                           unsigned int segmentEnd,
                           unsigned char* stack ///< stack buffer
)
{
    // if line segment: coord_1 = x; coord_2 = y;
    const unsigned int coord_1_shift = isLineSegment ? kColourCount : ( w * kColourCount );
    const unsigned int coord_2_shift = isLineSegment ? ( w * kColourCount ) : kColourCount;

    const unsigned int axis_1_size = isLineSegment ? w : h;
    const unsigned int coord_1_limit = axis_1_size - 1;

    const unsigned int div = ( radius * 2 ) + 1;
    const unsigned int mul_sum = stackblur_mul[radius];
    const unsigned char shr_sum = stackblur_shr[radius];

    for ( unsigned int coord_2 = segmentStart; coord_2 < segmentEnd; ++coord_2 )
    {// iterate through segment elements (i.e. lines or columns)
        ColourArray sum_colour = { 0 };
        ColourArray sum_in_colour = { 0 };
        ColourArray sum_out_colour = { 0 };

        unsigned char* src_ptr = src + coord_2_shift * coord_2;

        for ( unsigned int i = 0; i <= radius; ++i )
        {
            memcpy( &stack[kColourCount * i], src_ptr, kColourCount );

            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_colour[j] += src_ptr[j] * ( i + 1 );
            }
            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_out_colour[j] += src_ptr[j];
            }
        }

        for ( unsigned int i = 1; i <= radius; ++i )
        {
            if ( i <= coord_1_limit )
            {
                src_ptr += coord_1_shift;
            }

            memcpy( &stack[kColourCount * ( i + radius )], src_ptr, kColourCount );

            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_colour[j] += src_ptr[j] * ( radius + 1 - i );
            }
            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_in_colour[j] += src_ptr[j];
            }
        }

        unsigned int sp = radius;
        unsigned int coord_p = std::min( radius, coord_1_limit );
        unsigned int src_ptr_shift = isLineSegment ? ( coord_p + coord_2 * w ) : ( coord_2 + coord_p * w );

        src_ptr = src + kColourCount * src_ptr_shift;
        unsigned char* dst_ptr = src + coord_2 * coord_2_shift;
        for ( unsigned int coord_1 = 0; coord_1 < axis_1_size; ++coord_1 )
        {// iterate through pixels inside segment element
            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                dst_ptr[j] = static_cast<uint8_t>( ( sum_colour[j] * mul_sum ) >> shr_sum );
            }
            dst_ptr += coord_1_shift;

            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_colour[j] -= sum_out_colour[j];
            }

            unsigned int stack_start = sp + div - radius;
            if ( stack_start >= div )
            {
                stack_start -= div;
            }
            unsigned char* stack_ptr = &stack[kColourCount * stack_start];

            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_out_colour[j] -= stack_ptr[j];
            }

            if ( coord_p < coord_1_limit )
            {
                src_ptr += coord_1_shift;
                ++coord_p;
            }

            memcpy( stack_ptr, src_ptr, kColourCount );

            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_in_colour[j] += src_ptr[j];
            }
            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_colour[j] += sum_in_colour[j];
            }

            ++sp;
            if ( sp >= div )
            {
                sp = 0;
            }
            stack_ptr = &stack[sp * kColourCount];

            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_out_colour[j] += stack_ptr[j];
            }
            for ( unsigned int j = 0; j < kColourCount; ++j )
            {
                sum_in_colour[j] -= stack_ptr[j];
            }
        }
    }
}

class stack_blur_task : public pfc::thread
{
public:
    stack_blur_task( unsigned char* src, unsigned int w, unsigned int h, unsigned int radius, int cores, int core, unsigned char* stack )
        : src( src )
        , w( w )
        , h( h )
        , radius( radius )
        , cores( cores )
        , core( core )
        , stack( stack )
    {
    }

    void threadProc() override
    {
        const unsigned int axisSize = iterateByLines ? h : w;
        const unsigned int segmentStart = ( core * axisSize / cores );
        const unsigned int segmentEnd = ( core + 1 ) * axisSize / cores;

        stackblur_by_segment( src, w, h, radius, iterateByLines, segmentStart, segmentEnd, stack );
    }

    void changeAxis()
    {
        iterateByLines = false;
    }

private:
    unsigned char* src;

    const unsigned int w;
    const unsigned int h;
    const unsigned int radius;
    const int cores;
    const int core;

    bool iterateByLines = true;
    unsigned char* stack;
};

void stackblur( unsigned char* src, ///< input image data
                unsigned int w,     ///< image width
                unsigned int h,     ///< image height
                unsigned int radius ///< blur intensity (should be in 2..254 range)
)
{
    assert( radius <= 254 && radius >= 2 );

    const size_t cores = std::max( static_cast<t_size>( 1 ), pfc::getOptimalWorkerThreadCount() );
    const unsigned int div = ( radius * 2 ) + 1;
    std::vector<unsigned char> stack( div * 4 * cores );

    if ( cores == 1 )
    { // no multithreading
        stackblur_by_segment( src, w, h, radius, true, 0, h, stack.data() );
        stackblur_by_segment( src, w, h, radius, false, 0, w, stack.data() );
    }
    else
    {
        std::vector<std::unique_ptr<stack_blur_task>> workers;
        workers.reserve( cores );

        for ( size_t i = 0; i < cores; ++i )
        {
            auto& worker = workers.emplace_back(
                std::make_unique<stack_blur_task>( src, w, h, radius, cores, i, &stack[div * 4 * i] ) );
            worker->start();
        }

        for ( auto& worker : workers )
        {
            worker->waitTillDone();
        }

        for ( auto& worker : workers )
        {
            worker->changeAxis();
            worker->start();
        }

        for ( auto& worker : workers )
        {
            worker->waitTillDone();
        }
    }
}

} // namespace

namespace smp::utils
{

void stack_blur_filter( Gdiplus::Bitmap& img, int radius ) throw()
{
    int width = img.GetWidth();
    int height = img.GetHeight();

    Gdiplus::BitmapData bmpdata;
    Gdiplus::Rect rect;

    rect.X = rect.Y = 0;
    rect.Width = width;
    rect.Height = height;

    if ( img.LockBits( &rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bmpdata ) == Gdiplus::Ok )
    {
        radius = std::clamp( radius, 2, 254 );
        stackblur( (unsigned char*)bmpdata.Scan0, width, height, radius );
        img.UnlockBits( &bmpdata );
    }
}

} // namespace smp::utils
