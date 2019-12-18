// The Stack Blur Algorithm was invented by Mario Klingemann,
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

// This is C++ RGBA (32 bit color) multi-threaded version
// by Victor Laskin (victor.laskin@gmail.com)
// More details: http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp

#include <stdafx.h>

#include "stackblur.h"

namespace
{

constexpr uint32_t kColourCount = 4; // 0 - r, 1 - g, 2 - b, 3 - a
using ColourArray = std::array<uint32_t, kColourCount>;

// clang-format off
// protect array value format style
constexpr uint16_t stackblur_mul[255] =
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

constexpr uint8_t stackblur_shr[255] =
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
// clang-format on

void stackblur_by_segment( uint8_t* src,       ///< input image data
                           uint32_t w,         ///< image width
                           uint32_t h,         ///< image height
                           uint32_t radius,    ///< blur intensity (should be in 2..254 range)
                           bool isLineSegment, ///< false, means column segment
                           uint32_t segmentStart,
                           uint32_t segmentEnd,
                           uint8_t* stack ///< stack buffer
)
{
    // if line segment: coord_1 = x; coord_2 = y;
    const uint32_t coord_1_shift = isLineSegment ? kColourCount : ( w * kColourCount );
    const uint32_t coord_2_shift = isLineSegment ? ( w * kColourCount ) : kColourCount;

    const uint32_t axis_1_size = isLineSegment ? w : h;
    const uint32_t coord_1_limit = axis_1_size - 1;

    const uint32_t div = ( radius * 2 ) + 1;
    const uint32_t mul_sum = stackblur_mul[radius];
    const uint8_t shr_sum = stackblur_shr[radius];

    for ( uint32_t coord_2 = segmentStart; coord_2 < segmentEnd; ++coord_2 )
    { // iterate through segment elements (i.e. lines or columns)
        ColourArray sum_colour = { 0 };
        ColourArray sum_in_colour = { 0 };
        ColourArray sum_out_colour = { 0 };

        {
            // preload the kernel(stack)
            // pixels before the left edge of the image are
            // samples of [0] (max()).  min() handles
            // images which are smaller than the kernel.
            const uint8_t* src_ptr = src + coord_2_shift * coord_2;
            for ( int32_t i = -static_cast<int32_t>( radius ); i <= static_cast<int32_t>( radius ); ++i )
            {
                // calculate address of source pixel
                const size_t srcOffset = [coord_1_limit, coord_1_shift, i]() -> size_t {
                    if ( i <= 0 )
                    {
                        return 0;
                    }
                    return coord_1_shift * std::min( static_cast<uint32_t>( i ), coord_1_limit );
                }();
                const uint8_t* srcCur = src_ptr + srcOffset;

                // put pixel in the stack
                memcpy( &stack[kColourCount * ( i + radius )], srcCur, kColourCount );

                // rbs is a weight from (1)...(radius+1)...(1)
                const uint32_t rbs = ( radius + 1 ) - abs( i );

                // add pixel to accumulators
                for ( uint32_t j = 0; j < kColourCount; ++j )
                {
                    sum_colour[j] += srcCur[j] * rbs;
                }
                if ( i <= 0 )
                {
                    for ( uint32_t j = 0; j < kColourCount; ++j )
                    {
                        sum_out_colour[j] += srcCur[j];
                    }
                }
                else
                {
                    for ( uint32_t j = 0; j < kColourCount; ++j )
                    {
                        sum_in_colour[j] += srcCur[j];
                    }
                }
            }
        }

        // now that the kernel is preloaded
        // stackpointer is the index of the center of the kernel
        uint32_t stackPointer = radius;

        uint32_t coord_1_p = std::min( radius, coord_1_limit );
        const uint32_t src_ptr_shift = isLineSegment ? ( coord_1_p + coord_2 * w ) : ( coord_2 + coord_1_p * w );

        const uint8_t* src_ptr = src + kColourCount * src_ptr_shift;
        uint8_t* dst_ptr = src + coord_2 * coord_2_shift;
        for ( uint32_t coord_1 = 0; coord_1 < axis_1_size; ++coord_1 )
        { // iterate through pixels inside segment element
            // output a pixel
            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                dst_ptr[j] = static_cast<uint8_t>( ( sum_colour[j] * mul_sum ) >> shr_sum );
            }
            dst_ptr += coord_1_shift;

            // remove "past" pixels from the sum
            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                sum_colour[j] -= sum_out_colour[j];
            }

            // remove "left" side of stack from outsum
            uint32_t stack_start = stackPointer + div - radius;
            if ( stack_start >= div )
            {
                stack_start -= div;
            }
            uint8_t* stack_ptr = &stack[kColourCount * stack_start];
            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                sum_out_colour[j] -= stack_ptr[j];
            }

            // will repeat last pixel if past the edge
            if ( coord_1_p < coord_1_limit )
            {
                src_ptr += coord_1_shift;
                ++coord_1_p;
            }

            // now this (same) stack entry is the "right" side
            // add new pixel to the stack, and update accumulators
            memcpy( stack_ptr, src_ptr, kColourCount );
            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                sum_in_colour[j] += src_ptr[j];
            }
            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                sum_colour[j] += sum_in_colour[j];
            }

            // slide kernel one pixel ahead (right),
            // update accumulators again
            ++stackPointer;
            if ( stackPointer >= div )
            {
                stackPointer = 0;
            }
            stack_ptr = &stack[stackPointer * kColourCount];

            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                sum_out_colour[j] += stack_ptr[j];
            }
            for ( uint32_t j = 0; j < kColourCount; ++j )
            {
                sum_in_colour[j] -= stack_ptr[j];
            }
        }
    }
}

class stack_blur_task : public pfc::thread
{
public:
    stack_blur_task( uint8_t* src, uint32_t w, uint32_t h, uint32_t radius, size_t cores, size_t core, uint8_t* stack )
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
        const uint32_t axisSize = iterateByLines ? h : w;
        const uint32_t segmentStart = ( core * axisSize / cores );
        const uint32_t segmentEnd = ( core + 1 ) * axisSize / cores;

        stackblur_by_segment( src, w, h, radius, iterateByLines, segmentStart, segmentEnd, stack );
    }

    void changeAxis()
    {
        iterateByLines = false;
    }

private:
    uint8_t* src;

    const uint32_t w;
    const uint32_t h;
    const uint32_t radius;
    const size_t cores;
    const size_t core;

    bool iterateByLines = true;
    uint8_t* stack;
};

void stackblur( uint8_t* src,   ///< input image data
                uint32_t w,     ///< image width
                uint32_t h,     ///< image height
                uint32_t radius ///< blur intensity (should be in 2..254 range)
)
{
    assert( radius <= 254 && radius >= 2 );

    const size_t cores = std::max( static_cast<t_size>( 1 ), pfc::getOptimalWorkerThreadCount() );
    const uint32_t div = ( radius * 2 ) + 1;
    std::vector<uint8_t> stack( div * 4 * cores );

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

        for ( auto& worker: workers )
        {
            worker->waitTillDone();
        }

        for ( auto& worker: workers )
        {
            worker->changeAxis();
            worker->start();
        }

        for ( auto& worker: workers )
        {
            worker->waitTillDone();
        }
    }
}

} // namespace

namespace smp::utils
{

void stack_blur_filter( Gdiplus::Bitmap& img, int radius )
{
    const int width = img.GetWidth();
    const int height = img.GetHeight();

    const Gdiplus::Rect rect{ 0, 0, width, height };
    Gdiplus::BitmapData bmpdata;

    if ( img.LockBits( &rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bmpdata ) == Gdiplus::Ok )
    {
        radius = std::clamp( radius, 2, 254 );
        stackblur( static_cast<uint8_t*>( bmpdata.Scan0 ), width, height, radius );
        img.UnlockBits( &bmpdata );
    }
}

} // namespace smp::utils
