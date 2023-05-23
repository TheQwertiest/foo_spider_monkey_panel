#include <stdafx.h>

#include "css_fonts.h"

#include <utils/string_utils.h>

#include <qwr/utility.h>

namespace str_utils = smp::utils::string;

namespace
{

enum class ComponentName
{
    style,
    weight,
    size,
    family
};

}

namespace
{

std::optional<smp::dom::FontStyle> ParseStyle( std::string_view sv )
{
    if ( sv == "regular" )
    {
        return smp::dom::FontStyle::regular;
    }
    else if ( sv == "italic" )
    {
        return smp::dom::FontStyle::italic;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<smp::dom::FontWeight> ParseWeight( std::string_view sv )
{
    if ( sv == "normal" )
    {
        return smp::dom::FontWeight::regular;
    }
    else if ( sv == "bold" )
    {
        return smp::dom::FontWeight::bold;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<std::pair<double, smp::dom::FontSizeUnit>> ParseSize( std::string_view sv )
{
    auto sizeUnit = smp::dom::FontSizeUnit::px;
    if ( sv.ends_with( "px" ) )
    {
        sizeUnit = smp::dom::FontSizeUnit::px;
        sv.remove_suffix( std::size( "px" ) - 1 );
    }
    else
    {
        return std::nullopt;
    }

    if ( sv.empty() )
    {
        return std::nullopt;
    }

    auto sizeOpt = str_utils::ExtractBoundNumber( sv );
    if ( !sizeOpt || !sv.empty() )
    {
        return std::nullopt;
    }

    return std::make_pair( *sizeOpt, sizeUnit );
}

std::optional<std::string> ParseFamily( std::string_view sv )
{
    if ( !( sv.starts_with( '\'' ) && sv.ends_with( '\'' ) )
         && !( sv.starts_with( '"' ) && sv.ends_with( '"' ) ) )
    {
        return std::nullopt;
    }

    sv.remove_suffix( 1 );
    sv.remove_prefix( 1 );

    return std::string{ sv.data(), sv.size() };
}

} // namespace

namespace smp::dom
{

std::optional<FontDescription> FromCssFont( const std::string& cssFont )
{
    // TODO: add support for numerical weights
    std::string_view sv{ cssFont };
    str_utils::TrimWhitespace( sv );

    FontDescription fontDescription;
    std::string cleanCssFont;

    // TODO: simplify
    // TODO: add sorting (style then weight)
    // TODO: replace single quote with double quote in cleanCssFont
    std::unordered_set<ComponentName> parsedComponents;
    while ( !sv.empty() )
    {
        if ( sv.starts_with( '\'' ) || sv.starts_with( '"' ) )
        {
            if ( parsedComponents.contains( ComponentName::family ) )
            {
                return std::nullopt;
            }

            auto pos = sv.find_first_of( sv[0], 1 );
            auto component = sv.substr( 0, pos == std::string_view::npos ? pos : pos + 1 );
            sv.remove_prefix( component.size() );
            str_utils::LeftTrimWhitespace( sv );

            auto familyOpt = ParseFamily( component );
            if ( !familyOpt )
            {
                return std::nullopt;
            }

            fontDescription.family = *familyOpt;
            cleanCssFont += component;
            parsedComponents.emplace( ComponentName::family );

            break;
        }

        auto pos = sv.find_first_of( ' ' );
        auto component = sv.substr( 0, pos );
        sv.remove_prefix( component.size() );
        str_utils::LeftTrimWhitespace( sv );

        if ( parsedComponents.contains( ComponentName::size ) )
        {
            return std::nullopt;
        }

        if ( auto styleOpt = ParseStyle( component ) )
        {
            if ( parsedComponents.contains( ComponentName::style ) )
            {
                return std::nullopt;
            }

            fontDescription.style = *styleOpt;
            cleanCssFont += component;
            cleanCssFont += " ";
            parsedComponents.emplace( ComponentName::style );
        }
        else if ( auto weightOpt = ParseWeight( component ) )
        {
            if ( parsedComponents.contains( ComponentName::weight ) )
            {
                return std::nullopt;
            }

            fontDescription.weight = qwr::to_underlying( *weightOpt );
            cleanCssFont += component;
            cleanCssFont += " ";
            parsedComponents.emplace( ComponentName::weight );
        }
        else if ( auto sizeDataOpt = ParseSize( component ) )
        {
            if ( parsedComponents.contains( ComponentName::size ) )
            {
                return std::nullopt;
            }

            const auto [size, sizeUnit] = *sizeDataOpt;
            if ( size <= 0 )
            {
                return std::nullopt;
            }

            fontDescription.size = size;
            fontDescription.sizeUnit = sizeUnit;
            cleanCssFont += component;
            cleanCssFont += " ";
            parsedComponents.emplace( ComponentName::size );
        }
        else
        {
            return std::nullopt;
        }
    }

    if ( !sv.empty() )
    {
        return std::nullopt;
    }

    if ( !parsedComponents.contains( ComponentName::size ) || !parsedComponents.contains( ComponentName::family ) )
    {
        return std::nullopt;
    }

    fontDescription.cssFont = cleanCssFont;

    return fontDescription;
}

} // namespace smp::dom
