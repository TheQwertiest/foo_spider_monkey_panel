#include <stdafx.h>

#include "css_fonts.h"

#include <utils/string_utils.h>

#include <qwr/algorithm.h>
#include <qwr/utility.h>

#include <cwctype>

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

std::optional<smp::dom::FontStyle> ParseStyle( std::wstring_view sv )
{
    if ( sv == L"regular" )
    {
        return smp::dom::FontStyle::regular;
    }
    else if ( sv == L"italic" )
    {
        return smp::dom::FontStyle::italic;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<smp::dom::FontWeight> ParseWeight( std::wstring_view sv )
{
    if ( sv == L"normal" )
    {
        return smp::dom::FontWeight::regular;
    }
    else if ( sv == L"bold" )
    {
        return smp::dom::FontWeight::bold;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<std::pair<double, smp::dom::FontSizeUnit>> ParseSize( std::wstring_view sv )
{
    auto sizeUnit = smp::dom::FontSizeUnit::px;
    if ( sv.ends_with( L"px" ) )
    {
        sizeUnit = smp::dom::FontSizeUnit::px;
        sv.remove_suffix( std::size( L"px" ) - 1 );
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

std::optional<std::wstring> ParseFamily( std::wstring_view sv )
{
    if ( !( sv.starts_with( L'\'' ) && sv.ends_with( L'\'' ) )
         && !( sv.starts_with( L'"' ) && sv.ends_with( L'"' ) ) )
    {
        return std::nullopt;
    }

    sv.remove_suffix( 1 );
    sv.remove_prefix( 1 );

    return std::wstring{ sv.data(), sv.size() };
}

} // namespace

namespace smp::dom
{

std::optional<FontDescription> FromCssFont( const std::wstring& cssFont )
{
    // TODO: add support for numerical weights
    std::wstring_view sv{ cssFont };
    str_utils::TrimWhitespace( sv );

    FontDescription fontDescription;

    // TODO: simplify
    std::unordered_map<ComponentName, std::wstring> parsedComponents;
    while ( !sv.empty() )
    {
        if ( sv.starts_with( L'\'' ) || sv.starts_with( L'"' ) )
        {
            if ( parsedComponents.contains( ComponentName::family ) )
            {
                return std::nullopt;
            }

            auto pos = sv.find_first_of( sv[0], 1 );
            auto component = sv.substr( 0, pos == std::wstring_view::npos ? pos : pos + 1 );
            sv.remove_prefix( component.size() );
            str_utils::LeftTrimWhitespace( sv );

            auto familyOpt = ParseFamily( component );
            if ( !familyOpt )
            {
                return std::nullopt;
            }

            fontDescription.family = *familyOpt;
            parsedComponents.emplace( ComponentName::family, component );

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
            parsedComponents.emplace( ComponentName::style, component );
        }
        else if ( auto weightOpt = ParseWeight( component ) )
        {
            if ( parsedComponents.contains( ComponentName::weight ) )
            {
                return std::nullopt;
            }

            fontDescription.weight = qwr::to_underlying( *weightOpt );
            parsedComponents.emplace( ComponentName::weight, component );
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
            parsedComponents.emplace( ComponentName::size, component );
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

    // normalize cssFont value
    std::wstring cleanCssFont;
    for ( const auto name: { ComponentName::style, ComponentName::weight, ComponentName::size, ComponentName::family } )
    {
        if ( auto pComponent = qwr::FindAsPointer( parsedComponents, name ) )
        {
            auto cleanComponent = *pComponent;
            if ( name == ComponentName::family )
            {
                std::replace( cleanComponent.begin(), cleanComponent.end(), '\'', '"' );
            }
            else
            {
                ranges::transform( cleanComponent, cleanComponent.begin(), []( wint_t c ) { return std::towlower( c ); } );
                cleanComponent += ' ';
            }
            cleanCssFont += cleanComponent;
        }
    }
    fontDescription.cssFont = cleanCssFont;

    return fontDescription;
}

} // namespace smp::dom
