#pragma once

#include <utils/gdi_helpers.h>
#include <utils/hash_combine.h>

namespace std
{

// wether to combine size + weight + italic + underlune + striketrough into one hash or not
// or: hash_combine once, or once for each property
#define HASH_PACKED_PROPS 1

// std:hash specialization for LOGFONTW
template <>
struct hash<LOGFONTW>
{
    inline const size_t operator()( const LOGFONTW& logfont ) const noexcept
    {
        return smp::hash::combine
        (
            hash<wstring>{}( wstring( logfont.lfFaceName ) ),
#if HASH_PACKED_PROPS
            ( ( logfont.lfHeight            ) << 13 ) |  // shift height into upper bits
            ( ( logfont.lfWeight    & 0x3ff ) <<  3 ) |  // 10 bits for weight (0-1000),
            ( ( logfont.lfItalic    ? 1 : 0 ) <<  2 ) |  //  1 bit for the italic flag
            ( ( logfont.lfUnderline ? 1 : 0 ) <<  1 ) |  //  1 bit for the underline flag
            ( ( logfont.lfStrikeOut ? 1 : 0 )       )    //  1 bit for the strikeout flag
#else
            logfont.lfHeight,
            logfont.lfWeight,
            logfont.lfItalic,
            logfont.lfUnderline,
            logfont.lfStrikeOut
#endif
        );
    }
};

// std::equal to specialization for (hashed) LOGFONTWs
template <>
struct equal_to<LOGFONTW>
{
    inline bool operator()( const LOGFONTW& lhs, const LOGFONTW& rhs ) const noexcept
    {
        return ( hash<LOGFONTW>{}( lhs ) == hash<LOGFONTW>{}( rhs ) );
    }
};

} // namespace std

namespace smp::gdi
{

class FontCache
{
public:
    using container = std::unordered_map<LOGFONTW, weak_hfont>;
    using enumproc = std::function<void( const container::value_type)>;

public:
    static FontCache& Instance();

public: // cache methods
    shared_hfont CacheFontW( const LOGFONTW& logfont ) noexcept;
    size_t RemoveUnused( bool force = false ) noexcept;
    void Enumerate( enumproc callback ) const;
    void NornalizeLogfontW( const HDC dc, LOGFONTW& logfont );
    void NornalizeLogfontW( const HWND wnd, LOGFONTW& logfont );

private:
    inline const FontCache() = default;

    static container cache;
};

} // namespace smp::gdi

