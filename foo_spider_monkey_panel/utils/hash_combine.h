#pragma once

#include <limits>

// wether to use a custom hash combiner (with less collisions)
// or a relatively simpler, collision prone one based on boost's
#define HASH_CUSTOM_COMBINER 1

namespace smp::hash
{
#define HASHTYPE const std::size_t
#define HASHEXPR static inline constexpr HASHTYPE
#define HASHFUNC [[nodiscard]] HASHEXPR

#if HASH_CUSTOM_COMBINER
// use custom combiner (32/64bit agnostic)
HASHEXPR d = std::numeric_limits<HASHTYPE>::digits;
HASHEXPR i = std::numeric_limits<HASHTYPE>::digits10;
// (compile time) helper recursive constexprs (pseudo Random Distributed Bits  + ALTernating bits)
HASHFUNC rdb( HASHTYPE n ) noexcept { return 7 + ( n ? rdb( n - 1 ) * 10 : 0 ); }
HASHFUNC alt( HASHTYPE n ) noexcept { return 5 + ( n ? alt( n - 1 ) << 4 : 0 ); }
// custom combiner compositing helpers (Mul+Xor+Shift + ROTate)
HASHFUNC mxs( HASHTYPE m, HASHTYPE x ) noexcept { return m * ( x ^ ( x >> ( d / 2 ) ) ); };
HASHFUNC rot( HASHTYPE s, HASHTYPE b ) noexcept { return s << ( b % d ) | s >> ( d - b % d ); }
// evenly distributing hash combiner
HASHFUNC combiner( HASHTYPE seed, HASHTYPE hash ) noexcept { return rot( seed, d / 3 ) ^ mxs( rdb( i ), mxs( alt( d / 4 ), hash ) ); }
#else
// use boost magic (colliding) hash combiner
HASHEVAL boost_magic = 0x9e3779b9;
HASHFUNC combiner( HASHTYPE seed, HASHTYPE hash ) noexcept { return ( seed ^ hash ) + boost_magic + ( seed << 6 ) + ( seed >> 2 ); }
#endif

// var-args recursion end
HASHFUNC combine( HASHTYPE seed ) noexcept { return seed; }

// var-args template to recursively combine std::hash-es
template <typename T, typename... Rest>
HASHFUNC combine( HASHTYPE seed, const T& val, Rest&&... rest ) noexcept
{
    return hash::combine( hash::combiner( seed, std::hash<T>{}( val ) ), std::forward<Rest>( rest )... );
};

} // namespace