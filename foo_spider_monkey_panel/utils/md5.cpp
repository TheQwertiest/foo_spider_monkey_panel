#include <stdafx.h>

#include "md5.h"

/*

MD5
 converted to C++ class by Frank Thilo (thilo@unix-ag.org)
 for bzflag (http://www.bzflag.org)
 
   based on:
 
   md5.h and md5.c
   reference implemantion of RFC 1321
 
   Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.
 
License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.
 
License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.
 
RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.
 
These notices must be retained in any copies of any part of this
documentation and/or software.
 
*/

namespace
{

// Constants for MD5Transform routine.
constexpr uint32_t kS11 = 7;
constexpr uint32_t kS12 = 12;
constexpr uint32_t kS13 = 17;
constexpr uint32_t kS14 = 22;
constexpr uint32_t kS21 = 5;
constexpr uint32_t kS22 = 9;
constexpr uint32_t kS23 = 14;
constexpr uint32_t kS24 = 20;
constexpr uint32_t kS31 = 4;
constexpr uint32_t kS32 = 11;
constexpr uint32_t kS33 = 16;
constexpr uint32_t kS34 = 23;
constexpr uint32_t kS41 = 6;
constexpr uint32_t kS42 = 10;
constexpr uint32_t kS43 = 15;
constexpr uint32_t kS44 = 21;

} // namespace

namespace
{

// F, G, H and I are basic MD5 functions.
uint32_t F( uint32_t x, uint32_t y, uint32_t z )
{
    return ( x & y ) | ( ~x & z );
}

uint32_t G( uint32_t x, uint32_t y, uint32_t z )
{
    return ( x & z ) | ( y & ~z );
}

uint32_t H( uint32_t x, uint32_t y, uint32_t z )
{
    return x ^ y ^ z;
}

uint32_t I( uint32_t x, uint32_t y, uint32_t z )
{
    return y ^ ( x | ~z );
}

// rotate_left rotates x left n bits.
uint32_t rotate_left( uint32_t x, int n )
{
    return ( x << n ) | ( x >> ( 32 - n ) );
}

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
void FF( uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac )
{
    a = rotate_left( a + F( b, c, d ) + x + ac, s ) + b;
}

void GG( uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac )
{
    a = rotate_left( a + G( b, c, d ) + x + ac, s ) + b;
}

void HH( uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac )
{
    a = rotate_left( a + H( b, c, d ) + x + ac, s ) + b;
}

void II( uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac )
{
    a = rotate_left( a + I( b, c, d ) + x + ac, s ) + b;
}

} // namespace

namespace smp
{

MD5::MD5()
{
    Init();
}

MD5::MD5( std::span<const uint8_t> input )
{
    Init();
    Update( input );
    Finalize();
}

void MD5::Init()
{
    finalized = false;

    count[0] = 0;
    count[1] = 0;

    // load magic initialization constants.
    state[0] = 0x67452301;
    state[1] = 0xefcdab89;
    state[2] = 0x98badcfe;
    state[3] = 0x10325476;
}

// decodes input (uint8_t) into output (uint32_t). Assumes len is a multiple of 4.
void MD5::Decode( uint32_t output[], const uint8_t input[], size_t len )
{
    for ( size_t i = 0, j = 0; j < len; i++, j += 4 )
    {
        output[i] = ( (uint32_t)input[j] ) | ( ( (uint32_t)input[j + 1] ) << 8 ) | ( ( (uint32_t)input[j + 2] ) << 16 ) | ( ( (uint32_t)input[j + 3] ) << 24 );
    }
}

// encodes input (uint32_t) into output (uint8_t). Assumes len is
// a multiple of 4.
void MD5::Encode( uint8_t output[], const uint32_t input[], size_t len )
{
    for ( size_t i = 0, j = 0; j < len; i++, j += 4 )
    {
        output[j] = input[i] & 0xff;
        output[j + 1] = ( input[i] >> 8 ) & 0xff;
        output[j + 2] = ( input[i] >> 16 ) & 0xff;
        output[j + 3] = ( input[i] >> 24 ) & 0xff;
    }
}

// apply MD5 algo on a block
void MD5::Transform( const uint8_t block[kBlocksize] )
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    Decode( x, block, kBlocksize );

    /* Round 1 */
    FF( a, b, c, d, x[0], kS11, 0xd76aa478 );  /* 1 */
    FF( d, a, b, c, x[1], kS12, 0xe8c7b756 );  /* 2 */
    FF( c, d, a, b, x[2], kS13, 0x242070db );  /* 3 */
    FF( b, c, d, a, x[3], kS14, 0xc1bdceee );  /* 4 */
    FF( a, b, c, d, x[4], kS11, 0xf57c0faf );  /* 5 */
    FF( d, a, b, c, x[5], kS12, 0x4787c62a );  /* 6 */
    FF( c, d, a, b, x[6], kS13, 0xa8304613 );  /* 7 */
    FF( b, c, d, a, x[7], kS14, 0xfd469501 );  /* 8 */
    FF( a, b, c, d, x[8], kS11, 0x698098d8 );  /* 9 */
    FF( d, a, b, c, x[9], kS12, 0x8b44f7af );  /* 10 */
    FF( c, d, a, b, x[10], kS13, 0xffff5bb1 ); /* 11 */
    FF( b, c, d, a, x[11], kS14, 0x895cd7be ); /* 12 */
    FF( a, b, c, d, x[12], kS11, 0x6b901122 ); /* 13 */
    FF( d, a, b, c, x[13], kS12, 0xfd987193 ); /* 14 */
    FF( c, d, a, b, x[14], kS13, 0xa679438e ); /* 15 */
    FF( b, c, d, a, x[15], kS14, 0x49b40821 ); /* 16 */

    /* Round 2 */
    GG( a, b, c, d, x[1], kS21, 0xf61e2562 );  /* 17 */
    GG( d, a, b, c, x[6], kS22, 0xc040b340 );  /* 18 */
    GG( c, d, a, b, x[11], kS23, 0x265e5a51 ); /* 19 */
    GG( b, c, d, a, x[0], kS24, 0xe9b6c7aa );  /* 20 */
    GG( a, b, c, d, x[5], kS21, 0xd62f105d );  /* 21 */
    GG( d, a, b, c, x[10], kS22, 0x2441453 );  /* 22 */
    GG( c, d, a, b, x[15], kS23, 0xd8a1e681 ); /* 23 */
    GG( b, c, d, a, x[4], kS24, 0xe7d3fbc8 );  /* 24 */
    GG( a, b, c, d, x[9], kS21, 0x21e1cde6 );  /* 25 */
    GG( d, a, b, c, x[14], kS22, 0xc33707d6 ); /* 26 */
    GG( c, d, a, b, x[3], kS23, 0xf4d50d87 );  /* 27 */
    GG( b, c, d, a, x[8], kS24, 0x455a14ed );  /* 28 */
    GG( a, b, c, d, x[13], kS21, 0xa9e3e905 ); /* 29 */
    GG( d, a, b, c, x[2], kS22, 0xfcefa3f8 );  /* 30 */
    GG( c, d, a, b, x[7], kS23, 0x676f02d9 );  /* 31 */
    GG( b, c, d, a, x[12], kS24, 0x8d2a4c8a ); /* 32 */

    /* Round 3 */
    HH( a, b, c, d, x[5], kS31, 0xfffa3942 );  /* 33 */
    HH( d, a, b, c, x[8], kS32, 0x8771f681 );  /* 34 */
    HH( c, d, a, b, x[11], kS33, 0x6d9d6122 ); /* 35 */
    HH( b, c, d, a, x[14], kS34, 0xfde5380c ); /* 36 */
    HH( a, b, c, d, x[1], kS31, 0xa4beea44 );  /* 37 */
    HH( d, a, b, c, x[4], kS32, 0x4bdecfa9 );  /* 38 */
    HH( c, d, a, b, x[7], kS33, 0xf6bb4b60 );  /* 39 */
    HH( b, c, d, a, x[10], kS34, 0xbebfbc70 ); /* 40 */
    HH( a, b, c, d, x[13], kS31, 0x289b7ec6 ); /* 41 */
    HH( d, a, b, c, x[0], kS32, 0xeaa127fa );  /* 42 */
    HH( c, d, a, b, x[3], kS33, 0xd4ef3085 );  /* 43 */
    HH( b, c, d, a, x[6], kS34, 0x4881d05 );   /* 44 */
    HH( a, b, c, d, x[9], kS31, 0xd9d4d039 );  /* 45 */
    HH( d, a, b, c, x[12], kS32, 0xe6db99e5 ); /* 46 */
    HH( c, d, a, b, x[15], kS33, 0x1fa27cf8 ); /* 47 */
    HH( b, c, d, a, x[2], kS34, 0xc4ac5665 );  /* 48 */

    /* Round 4 */
    II( a, b, c, d, x[0], kS41, 0xf4292244 );  /* 49 */
    II( d, a, b, c, x[7], kS42, 0x432aff97 );  /* 50 */
    II( c, d, a, b, x[14], kS43, 0xab9423a7 ); /* 51 */
    II( b, c, d, a, x[5], kS44, 0xfc93a039 );  /* 52 */
    II( a, b, c, d, x[12], kS41, 0x655b59c3 ); /* 53 */
    II( d, a, b, c, x[3], kS42, 0x8f0ccc92 );  /* 54 */
    II( c, d, a, b, x[10], kS43, 0xffeff47d ); /* 55 */
    II( b, c, d, a, x[1], kS44, 0x85845dd1 );  /* 56 */
    II( a, b, c, d, x[8], kS41, 0x6fa87e4f );  /* 57 */
    II( d, a, b, c, x[15], kS42, 0xfe2ce6e0 ); /* 58 */
    II( c, d, a, b, x[6], kS43, 0xa3014314 );  /* 59 */
    II( b, c, d, a, x[13], kS44, 0x4e0811a1 ); /* 60 */
    II( a, b, c, d, x[4], kS41, 0xf7537e82 );  /* 61 */
    II( d, a, b, c, x[11], kS42, 0xbd3af235 ); /* 62 */
    II( c, d, a, b, x[2], kS43, 0x2ad7d2bb );  /* 63 */
    II( b, c, d, a, x[9], kS44, 0xeb86d391 );  /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    // Zeroize sensitive information.
    memset( x, 0, sizeof x );
}

// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block
void MD5::Update( std::span<const uint8_t> input )
{
    const auto length = input.size();

    // compute number of bytes mod 64
    size_t index = count[0] / 8 % kBlocksize;

    // Update number of bits
    if ( ( count[0] += ( length << 3 ) ) < ( length << 3 ) )
    {
        count[1]++;
    }
    count[1] += ( length >> 29 );

    // number of bytes we need to fill in buffer
    size_t firstpart = 64 - index;

    size_t i;

    // transform as many times as possible.
    if ( length >= firstpart )
    {
        // fill buffer first, transform
        memcpy( &buffer[index], input.data(), firstpart );
        Transform( buffer );

        // transform chunks of kBlocksize (64 bytes)
        for ( i = firstpart; i + kBlocksize <= length; i += kBlocksize )
        {
            Transform( &input[i] );
        }

        index = 0;
    }
    else
    {
        i = 0;
    }

    if ( length != i )
    { // buffer remaining input
        memcpy( &buffer[index], &input[i], length - i );
    }
}

void MD5::Finalize()
{
    // clang-format off
    static constexpr uint8_t kPadding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    // clang-format off

    if ( !finalized )
    {
        // Save number of bits
        uint8_t bits[8];
        Encode( bits, count, 8 );

        // pad out to 56 mod 64.
        size_t index = count[0] / 8 % 64;
        size_t padLen = ( index < 56 ) ? ( 56 - index ) : ( 120 - index );
        Update( {kPadding, padLen} );

        // Append length (before padding)
        Update( {bits, 8} );

        // Store state in digest
        Encode( digest, state, 16 );

        // Zeroize sensitive information.
        memset( buffer, 0, sizeof buffer );
        memset( count, 0, sizeof count );

        finalized = true;
    }
}

std::string MD5::HexDigest() const
{
    if ( !finalized )
    {
        return "";
    }

    return fmt::format( "{:02x}", fmt::join( digest, "" ) );
}

std::string CalculateMd5( std::span<const uint8_t> input )
{
    return MD5( input ).HexDigest();
}

} // namespace smp
