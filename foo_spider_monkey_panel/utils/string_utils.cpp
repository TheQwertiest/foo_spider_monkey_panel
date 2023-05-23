#include <stdafx.h>

#include "string_utils.h"

namespace
{

constexpr const int32_t kMaxIntDigits = 4;
constexpr const int32_t kMaxFractionDigits = 4;

} // namespace

namespace smp::utils::string
{

bool Contains( std::string_view sv, std::string_view str )
{ // TODO: replace with `.contains()` in C++23
    return ( sv.find( str ) != std::string_view::npos );
}

std::optional<float> ExtractBoundNumber( std::string_view& sv )
{
    if ( sv.empty() )
    {
        return std::nullopt;
    }

    const auto chToDigit = []( auto ch ) {
        auto v = ch - '0';
        return ( v > 9 ? -1 : v );
    };

    uint32_t iRet = 0;
    int32_t parsedCharsCnt = 0;
    int8_t fixedPointCnt = -1;
    bool hasMinus = sv.starts_with( "-" );
    bool hasPlus = sv.starts_with( "+" );
    auto sv_stripped = sv;
    if ( hasPlus || hasMinus )
    {
        sv_stripped = sv.substr( 1 );
    }

    for ( auto ch: sv_stripped )
    {
        if ( ch == '.' )
        {
            if ( fixedPointCnt >= 0 )
            {
                break;
            }

            ++fixedPointCnt;

            ++parsedCharsCnt;
            continue;
        }

        const auto digit = chToDigit( ch );
        if ( digit < 0 )
        {
            break;
        }

        if ( fixedPointCnt < 0 && parsedCharsCnt > kMaxIntDigits )
        {
            iRet = 999;
            ++parsedCharsCnt;
            continue;
        }

        if ( fixedPointCnt >= 0 )
        {
            if ( fixedPointCnt == kMaxFractionDigits )
            {
                ++parsedCharsCnt;
                continue;
            }

            ++fixedPointCnt;
        }

        iRet *= 10;
        iRet += digit;

        ++parsedCharsCnt;
    }

    if ( !parsedCharsCnt || !fixedPointCnt )
    {
        return std::nullopt;
    }
    sv_stripped.remove_prefix( parsedCharsCnt );

    sv = sv_stripped;

    float fRet = ( fixedPointCnt > 0 ? iRet / powf( 10, fixedPointCnt ) : iRet );
    return ( hasMinus ? -fRet : fRet );
}

void LeftTrimWhitespace( std::string_view& sv )
{
    auto pos = sv.find_first_not_of( ' ' );
    if ( std::string::npos == pos )
    {
        sv.remove_prefix( sv.size() );
        return;
    }

    sv = sv.substr( pos );
}

void RightTrimWhitespace( std::string_view& sv )
{
    auto pos = sv.find_last_not_of( ' ' );
    if ( std::string::npos == pos )
    {
        sv.remove_prefix( sv.size() );
        return;
    }

    sv = sv.substr( 0, pos + 1 );
}

void TrimWhitespace( std::string_view& sv )
{
    LeftTrimWhitespace( sv );
    RightTrimWhitespace( sv );
}

} // namespace smp::utils::string
