#include <stdafx.h>

#include "string_utils.h"

namespace
{

constexpr const int32_t kMaxIntDigits = 4;
constexpr const int32_t kMaxFractionDigits = 4;

} // namespace

namespace
{

template <typename T>
bool Contains( T str, T substr )
{ // TODO: replace with `.contains()` in C++23
    return ( str.find( substr ) != T::npos );
}

template <typename T>
void LeftTrimWhitespace( T& sv )
{
    auto pos = sv.find_first_not_of( ' ' );
    if ( pos == T::npos )
    {
        sv.remove_prefix( sv.size() );
        return;
    }

    sv = sv.substr( pos );
}

template <typename T>
void RightTrimWhitespace( T& sv )
{
    auto pos = sv.find_last_not_of( ' ' );
    if ( pos == T::npos )
    {
        sv.remove_prefix( sv.size() );
        return;
    }

    sv = sv.substr( 0, pos + 1 );
}

template <typename T>
void TrimWhitespace( T& sv )
{
    LeftTrimWhitespace( sv );
    RightTrimWhitespace( sv );
}

} // namespace

namespace smp::utils::string
{

bool Contains( std::string_view str, std::string_view substr )
{
    return ::Contains( str, substr );
}

bool Contains( std::wstring_view str, std::wstring_view substr )
{
    return ::Contains( str, substr );
}

std::optional<float> ExtractBoundNumber( std::string_view& sv )
{ // TODO: deduplicate
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

std::optional<float> ExtractBoundNumber( std::wstring_view& sv )
{
    if ( sv.empty() )
    {
        return std::nullopt;
    }

    const auto chToDigit = []( auto ch ) {
        auto v = ch - L'0';
        return ( v > 9 ? -1 : v );
    };

    uint32_t iRet = 0;
    int32_t parsedCharsCnt = 0;
    int8_t fixedPointCnt = -1;
    bool hasMinus = sv.starts_with( L'-' );
    bool hasPlus = sv.starts_with( L'+' );
    auto sv_stripped = sv;
    if ( hasPlus || hasMinus )
    {
        sv_stripped = sv.substr( 1 );
    }

    for ( auto ch: sv_stripped )
    {
        if ( ch == L'.' )
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
    ::LeftTrimWhitespace( sv );
}

void LeftTrimWhitespace( std::wstring_view& sv )
{
    ::LeftTrimWhitespace( sv );
}

void RightTrimWhitespace( std::string_view& sv )
{
    ::RightTrimWhitespace( sv );
}

void RightTrimWhitespace( std::wstring_view& sv )
{
    ::RightTrimWhitespace( sv );
}

void TrimWhitespace( std::string_view& sv )
{
    ::TrimWhitespace( sv );
}

void TrimWhitespace( std::wstring_view& sv )
{
    ::TrimWhitespace( sv );
}

} // namespace smp::utils::string
