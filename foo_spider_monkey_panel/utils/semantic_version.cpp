#include <stdafx.h>
#include "semantic_version.h"

#include <cctype>

// TODO: replace stol with from_chars

namespace smp::version
{

SemVer::SemVer( const std::string& strVer ) noexcept( false )
{
    const auto ret = ParseString( strVer );
    if ( !ret )
    {
        throw std::runtime_error( "Parsing failed" );
    }
    *this = ret.value();
}

std::optional<SemVer> SemVer::ParseString( const std::string& strVer )
{
    SemVer semVer;

    std::string_view curScope( strVer );
    if ( size_t pos = curScope.find_first_of( '+' ); std::string::npos != pos )
    {
        semVer.metadata = curScope.substr( pos + 1, curScope.size() - pos );
        curScope.remove_suffix( curScope.size() - pos );
    }

    if ( size_t pos = curScope.find_first_of( '-' ); std::string::npos != pos )
    {
        semVer.prerelease = curScope.substr( pos + 1, curScope.size() - pos );
        curScope.remove_suffix( curScope.size() - pos );
    }
    if ( !curScope.size() )
    {
        return std::nullopt;
    }

    auto extractNumber = []( std::string_view& numberStrView ) -> std::optional<uint8_t> {
        try
        {
            size_t idx = 0;
            const int8_t number = std::stoi( std::string{ numberStrView.data(), numberStrView.size() }, &idx );
            if ( !idx )
            {
                return std::nullopt;
            }

            numberStrView.remove_prefix( idx );
            return number;
        }
        catch ( const std::invalid_argument& )
        {
            return std::nullopt;
        }
        catch ( const std::out_of_range& )
        {
            return std::nullopt;
        }
    };

    {
        const auto majorNumber = extractNumber( curScope );
        if ( !majorNumber || ( !curScope.empty() && curScope[0] != '.' ) )
        {
            return std::nullopt;
        }

        semVer.major = majorNumber.value();
        if ( curScope.empty() )
        {
            return semVer;
        }
    }

    {
        curScope.remove_prefix( 1 );
        const auto minorNumber = extractNumber( curScope );
        if ( !minorNumber || ( !curScope.empty() && curScope[0] != '.' ) )
        {
            return std::nullopt;
        }

        semVer.minor = minorNumber.value();
        if ( curScope.empty() )
        {
            return semVer;
        }
    }

    {
        curScope.remove_prefix( 1 );
        const auto patchNumber = extractNumber( curScope );
        if ( !patchNumber || !curScope.empty() )
        {
            return std::nullopt;
        }

        semVer.patch = patchNumber.value();
    }

    return semVer;
}

bool SemVer::operator==( const SemVer& other ) const
{ // metadata is ignored during comparison
    return ( major == other.major
             && minor == other.minor
             && patch == other.patch
             && prerelease == other.prerelease );
}
bool SemVer::operator!=( const SemVer& other ) const
{
    return ( !( *this == other ) );
}
bool SemVer::operator<( const SemVer& other ) const
{
    if ( major != other.major )
    {
        return ( major < other.major );
    }
    if ( minor != other.minor )
    {
        return ( minor < other.minor );
    }
    if ( patch != other.patch )
    {
        return ( patch < other.patch );
    }

    // metadata is ignored during comparison
    return IsPreleaseNewer( other.prerelease, prerelease );
}
bool SemVer::operator>( const SemVer& other ) const
{
    return ( other < *this );
}
bool SemVer::operator<=( const SemVer& other ) const
{
    return ( !( other < *this ) );
}
bool SemVer::operator>=( const SemVer& other ) const
{
    return ( !( *this < other ) );
}

bool SemVer::IsPreleaseNewer( std::string_view a, std::string_view b )
{
    if ( a == b )
    {
        return false;
    }

    if ( a.empty() || b.empty() )
    { // Pre-release versions have a lower precedence than the associated normal version
        return a.empty();
    }

    auto isNumber = []( std::string_view str ) {
        return ( str.cend() == ranges::find_if_not( str, []( char c ) { return std::isdigit( c ); } ) );
    };

    auto extractToken = []( std::string_view str ) -> std::string_view {
        assert( !str.empty() );

        std::string_view token( str );
        if ( size_t pos = token.find_first_of( '.' ); std::string::npos != pos )
        {
            token = token.substr( 0, pos );
            str.remove_prefix( pos + 1 );
        }
        else
        {
            str = std::string_view{};
        }

        return token;
    };

    while ( !a.empty() && !b.empty() )
    {
        const std::string_view a_Token = extractToken( a );
        const std::string_view b_Token = extractToken( b );

        const bool a_isNumber = isNumber( a_Token );
        const bool b_isNumber = isNumber( b_Token );
        if ( a_isNumber != b_isNumber )
        { // Numeric identifiers always have lower precedence than non-numeric identifiers
            return !a_isNumber;
        }

        if ( a_isNumber && b_isNumber )
        {
            size_t dummy = 0; // should be valid
            const int8_t aNum = std::stoi( std::string{ a_Token.data(), a_Token.size() }, &dummy );
            assert( dummy );

            const int8_t bNum = std::stoi( std::string{ b_Token.data(), b_Token.size() }, &dummy );
            assert( dummy );

            if ( aNum != bNum )
            {
                return aNum > bNum;
            }
        }
        if ( a_Token != b_Token )
        {
            return a_Token > b_Token;
        }

        if ( a.empty() != b.empty() )
        { // A larger set of pre-release fields has a higher precedence than a smaller set
            return !a.empty();
        }
    }

    // They are equal (should not reach here)
    assert( 0 );
    return false;
}

} // namespace smp::version
