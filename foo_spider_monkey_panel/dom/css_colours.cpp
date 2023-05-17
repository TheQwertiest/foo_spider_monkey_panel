#include <stdafx.h>

#include "css_colours.h"

namespace
{

constexpr const int32_t kMaxIntDigits = 4;
constexpr const int32_t kMaxFractionDigits = 4;

} // namespace

namespace
{

bool Contains( std::string_view sv, std::string_view str )
{ // TODO: replace with `.contains()` in C++23
    return ( sv.find( str ) != std::string_view::npos );
}

uint8_t PercentToColour( float value )
{
    return static_cast<uint8_t>( std::round( std::clamp<float>( value * 255 / 100, 0.0, 255.0 ) ) );
}

uint8_t FractionToColour( float value )
{
    return static_cast<uint8_t>( std::round( std::clamp<float>( value * 255, 0.0, 255.0 ) ) );
}

uint8_t NumberToColour( float value )
{
    return static_cast<uint8_t>( std::round( std::clamp<float>( value, 0.0, 255.0 ) ) );
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

bool StripLeftSeparator( std::string_view& sv, char separator )
{
    if ( separator != ' ' )
    {
        LeftTrimWhitespace( sv );
    }
    if ( !sv.starts_with( separator ) )
    {
        return false;
    }

    sv.remove_prefix( 1 );
    LeftTrimWhitespace( sv );

    return true;
}

std::optional<Gdiplus::Color> ParseRgb( std::string_view sv )
{
    if ( sv.starts_with( "rgb(" ) )
    {
        sv.remove_prefix( std::size( "rgb(" ) - 1 );
    }
    else if ( sv.starts_with( "rgba(" ) )
    {
        sv.remove_prefix( std::size( "rgba(" ) - 1 );
    }
    else
    {
        return std::nullopt;
    }

    if ( !sv.ends_with( ")" ) )
    {
        return std::nullopt;
    }
    sv.remove_suffix( 1 );
    TrimWhitespace( sv );

    const auto isCommaSeparated = Contains( sv, "," );

    int percentState = -1;
    std::array<uint8_t, 3> components{};
    for ( size_t i = 0; i < components.size(); ++i )
    {
        const auto numberOpt = ExtractBoundNumber( sv );
        if ( !numberOpt )
        {
            return std::nullopt;
        }

        switch ( percentState )
        {
        case -1:
            percentState = sv.starts_with( "%" );
        case 0:
        case 1:
            if ( !!percentState != sv.starts_with( "%" ) )
            {
                return std::nullopt;
            }
            sv.remove_prefix( percentState );
        }

        if ( i != components.size() - 1 && !StripLeftSeparator( sv, isCommaSeparated ? ',' : ' ' ) )
        {
            return std::nullopt;
        }

        components[i] = ( percentState == 1 ? PercentToColour( *numberOpt ) : NumberToColour( *numberOpt ) );
    }
    LeftTrimWhitespace( sv );

    if ( sv.empty() )
    {
        return Gdiplus::Color{ components[0], components[1], components[2] };
    }

    if ( !StripLeftSeparator( sv, isCommaSeparated ? ',' : '/' ) )
    {
        return std::nullopt;
    }
    LeftTrimWhitespace( sv );

    const auto alphaNumberOpt = ExtractBoundNumber( sv );
    if ( !alphaNumberOpt )
    {
        return std::nullopt;
    }

    const auto isAlphaPercent = Contains( sv, "%" );
    if ( isAlphaPercent )
    {
        sv.remove_prefix( 1 );
    }
    if ( !sv.empty() )
    {
        return std::nullopt;
    }

    const auto alphaColour = ( isAlphaPercent ? PercentToColour( *alphaNumberOpt ) : FractionToColour( *alphaNumberOpt ) );
    return Gdiplus::Color{ alphaColour, components[0], components[1], components[2] };
}

std::optional<uint8_t> HexCharToNumber( char ch )
{
    if ( ch >= '0' && ch <= '9' )
    {
        return ch - '0';
    }
    if ( ch >= 'A' && ch <= 'F' )
    {
        return 10 + ch - 'A';
    }
    if ( ch >= 'a' && ch <= 'f' )
    {
        return 10 + ch - 'a';
    }

    return std::nullopt;
}

std::optional<Gdiplus::Color> ParseHex( std::string_view sv )
{
    if ( !sv.starts_with( "#" ) )
    {
        return std::nullopt;
    }
    sv.remove_prefix( 1 );

    switch ( sv.size() )
    {
    case 3:
    case 4:
    {
        uint32_t colour = 0;
        for ( auto ch: sv )
        {
            auto numOpt = HexCharToNumber( ch );
            if ( !numOpt )
            {
                return std::nullopt;
            }

            colour <<= 8;
            colour |= *numOpt << 4 | *numOpt;
        }
        return ( sv.size() == 3
                     ? Gdiplus::Color{ colour | 0xFF000000 }
                     : Gdiplus::Color{ ( colour & 0xFF ) << 8 * 3
                                       | colour >> 8 } );
    }
    case 6:
    case 8:
    {
        uint32_t colour = 0;
        for ( auto ch: sv )
        {
            auto numOpt = HexCharToNumber( ch );
            if ( !numOpt )
            {
                return std::nullopt;
            }

            colour <<= 4;
            colour |= *numOpt;
        }
        return ( sv.size() == 6
                     ? Gdiplus::Color{ colour | 0xFF000000 }
                     : Gdiplus::Color{ ( colour & 0xFF ) << 8 * 3
                                       | colour >> 8 } );
    }
    default:
        return std::nullopt;
    }
}

char CharToLower( char ch )
{
    return ( ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch );
}

} // namespace

namespace smp::dom
{

std::optional<Gdiplus::Color> FromCssColour( const std::string& cssColour )
{
    const auto cssColourLower = cssColour | ranges::views::transform( CharToLower ) | ranges::to<std::string>;
    std::string_view sv{ cssColourLower };
    TrimWhitespace( sv );

    if ( sv.starts_with( "rgb" ) )
    {
        return ParseRgb( sv );
    }

    if ( sv.starts_with( "#" ) )
    {
        return ParseHex( sv );
    }

    return std::nullopt;
}

std::string ToCssColour( const Gdiplus::Color& colour )
{
    return ( colour.GetA() == 0xFF
                 ? fmt::format( "#{:02x}{:02x}{:02x}",
                                static_cast<uint8_t>( colour.GetR() ),
                                static_cast<uint8_t>( colour.GetG() ),
                                static_cast<uint8_t>( colour.GetB() ) )
                 : fmt::format( "rgb({}, {}, {}, {:.3g})",
                                static_cast<uint8_t>( colour.GetR() ),
                                static_cast<uint8_t>( colour.GetG() ),
                                static_cast<uint8_t>( colour.GetB() ),
                                colour.GetA() / 255.0 ) );
}

} // namespace smp::dom
