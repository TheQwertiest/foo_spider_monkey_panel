#pragma once

#include <limits>
#include <bit>

// wether to use a custom hash combiner (with supposedly less collisions), or one based on boost's
#define HASH_CUSTOM_COMBINER 1

namespace smp::hash
{

#if HASH_CUSTOM_COMBINER
// mangled from:
// https://newbedev.com/c-why-is-boost-hash-combine-the-best-way-to-combine-hash-values
// (tldr: it's not)
inline constexpr size_t h = std::numeric_limits<size_t>::digits / 2; // mulxor value shifted by (Half) bits
inline constexpr size_t a = 0x55555555;                              // Alternating bits of 0101-s
inline constexpr size_t r = 0x2e5bf271;                              // Random uneven int constant
inline constexpr size_t t = std::numeric_limits<size_t>::digits / 3; // rotate seed by (Third) bits

inline const size_t mxs( const size_t m, const size_t& x ) noexcept { return m * ( x ^ ( x >> h ) ); }
inline const size_t dsp( const size_t& z ) noexcept { return mxs( r, mxs( a, z ) ); }

inline const size_t combiner( const size_t& seed, const size_t& hash ) noexcept
{
    return std::rotl( seed, t ) ^ dsp( hash );
}
#else
inline const size_t combiner( const size_t& seed, const size_t& hash ) noexcept
{
    constexpr size_t boost_magic = 0x9e3779b9;
    return ( seed ^ hash ) + boost_magic + ( seed << 6 ) + ( seed >> 2 );
}
#endif

inline constexpr size_t combine( const std::size_t& seed ) noexcept
{
    return seed;
}

template <typename T, typename... Rest>
inline constexpr size_t combine( const size_t& seed, const T& val, Rest... rest ) noexcept
{
    return combine( combiner( seed, std::hash<T>{}( val ) ), rest... );
};

} // namespace