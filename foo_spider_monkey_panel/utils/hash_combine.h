#pragma once

// wether to use a custom hash combiner (with supposedly less collisions), or one based on boost's
#define HASH_CUSTOM_COMBINER 1

namespace smp::hash
{
#if HASH_CUSTOM_COMBINER
// mangled from:
// https://newbedev.com/c-why-is-boost-hash-combine-the-best-way-to-combine-hash-values
// (tldr: its not)
inline constexpr size_t h = std::numeric_limits<size_t>::digits / 2;    // mulxor value shifted by (sizehalf) bits
inline constexpr size_t a = 0x55555555;                                 // alternating bits of 0101-s
inline constexpr size_t r = 0x2e5bf271;                                 // random uneven int constant
inline constexpr size_t t = std::numeric_limits<size_t>::digits / 3;    // rotate seed by (sizethird) bits

inline constexpr size_t rots( const size_t& o, const size_t& s ) noexcept { return std::rotl( o, s ); }
inline constexpr size_t mxxh( const size_t& m, const size_t& x ) noexcept { return m * ( x ^ rots( x, h ) ); }
inline constexpr size_t dist( const size_t& z ) noexcept { return mxxh( r, mxxh( a, z ) ); }

inline constexpr size_t combiner( const size_t& seed, const size_t& grow ) noexcept
{
    return rots( seed, t ) ^ dist( grow );
}
#else
// boost::hash_combine
inline constexpr size_t boost_magic = 0x9e3779b9; // random uneven int constant from boost

inline constexpr size_t combiner( const size_t& seed, const size_t& grow ) noexcept
{
    return ( seed ^ grow ) + boost_magic + ( seed << 6 ) + ( seed >> 2 );
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