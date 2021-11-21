#pragma once

#include <utils/gdi_helpers.h>
#include <utils/hash_combine.h>

// convert negative fontSize to fontHeight in LOGFONT-s, and avoid duplicate cache entries
#define FONT_CACHE_ABSOLUTE_HEIGHT 1

// wether to combine size + weight + italic + underlune + striketrough into one hash or not
// or: hash_combine once, or once for each property
#define HASH_PACKED_PROPS 1

namespace smp::fontcache
{
using unique_hfont = smp::gdi::unique_gdi_ptr<HFONT>;
using shared_hfont = std::shared_ptr<unique_hfont::element_type>;
using weak_hfont = shared_hfont::weak_type;

shared_hfont Cache( const LOGFONTW& logfont ) noexcept;
size_t Purge( bool force = false ) noexcept;

void ForEach( std::function<void( const std::pair<LOGFONTW, weak_hfont>& )> callback );

} // namespace smp::fontcache

namespace smp::logfont
{ // FIXME: move into own cpp/h?

void Make
(
    const std::wstring& _In_ fontName,
    const int32_t _In_ fontSize,
    const uint32_t _In_ fontStyle,
    LOGFONTW& _Out_ logfont
);

void Normalize
(
    const HDC _In_ dc,
    LOGFONTW& _Out_ logfont
);

} // namespace smp::logfont

namespace std
{

// specialized std::hash for LOGFONTW
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
            ( ( logfont.lfWeight    & 0x3ff ) <<  3 ) |  // 10 bits = weight (0-1000),
            ( ( logfont.lfItalic    ? 1 : 0 ) <<  2 ) |  //  1 bit for the italic flag
            ( ( logfont.lfUnderline ? 1 : 0 ) <<  1 ) |  //  1 bit for the underline flag
            ( ( logfont.lfStrikeOut ? 1 : 0 )       )    //  1 but for the strikeout flag
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

// specialized std::equal_to for LOGFONTW-s, based on their hash
template <>
struct equal_to<LOGFONTW>
{
    inline bool operator()( const LOGFONTW& lhs, const LOGFONTW& rhs ) const noexcept
    {
        return ( hash<LOGFONTW>{}( lhs ) == hash<LOGFONTW>{}( rhs ) );
    }
};

} // namespace std