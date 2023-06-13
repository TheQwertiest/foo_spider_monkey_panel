#include <stdafx.h>

#include "font_manager.h"

#include <dom/css_fonts.h>
#include <graphics/gdiplus/error_handler.h>

#include <qwr/utility.h>

#include <cwctype>

namespace
{

/// @throw qwr::QwrException
auto FetchFont( const smp::dom::FontDescription& fontDescription, bool isUnderlined, bool isStrikeout )
{
    auto pFamily = std::make_unique<Gdiplus::FontFamily>( fontDescription.family.c_str() );
    smp::CheckGdiPlusObject( pFamily );

    auto pFont = std::make_unique<Gdiplus::Font>(
        pFamily.get(),
        static_cast<float>( fontDescription.size ),
        [&] {
            const auto isBold = ( fontDescription.weight == qwr::to_underlying( smp::dom::FontWeight::bold ) );
            int32_t style = 0;
            if ( isUnderlined )
            {
                style |= Gdiplus::FontStyleUnderline;
            }
            if ( isStrikeout )
            {
                style |= Gdiplus::FontStyleStrikeout;
            }
            switch ( fontDescription.style )
            {
            case smp::dom::FontStyle::regular:
                return style | ( isBold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular );
            case smp::dom::FontStyle::italic:
                return style | ( isBold ? Gdiplus::FontStyleBoldItalic : Gdiplus::FontStyleItalic );
            default:
            {
                assert( false );
                return qwr::to_underlying( Gdiplus::FontStyleRegular );
            }
            }
        }(),
        [&] {
            switch ( fontDescription.sizeUnit )
            {
            case smp::dom::FontSizeUnit::px:
                return Gdiplus::UnitPixel;
            default:
            {
                assert( false );
                return Gdiplus::UnitPixel;
            }
            }
        }() );
    smp::CheckGdiPlusObject( pFont );

    // TODO: handle dpi and units here
    const auto fontStyle = pFont->GetStyle();
    const auto lineHeight = fontDescription.size * pFamily->GetLineSpacing( fontStyle ) / pFamily->GetEmHeight( fontStyle );
    const auto ascentHeight = fontDescription.size * pFamily->GetCellAscent( fontStyle ) / pFamily->GetEmHeight( fontStyle );
    const auto descentHeight = fontDescription.size * pFamily->GetCellDescent( fontStyle ) / pFamily->GetEmHeight( fontStyle );

    return smp::make_not_null_shared<const smp::GdiPlusFontData>(
        std::move( pFont ),
        std::move( pFamily ),
        static_cast<float>( ascentHeight ),
        static_cast<float>( descentHeight ),
        static_cast<float>( lineHeight ) );
}

} // namespace

namespace smp
{

GdiPlusFontManager::GdiPlusFontManager()
{
}

GdiPlusFontManager& GdiPlusFontManager::Get()
{
    assert( core_api::is_main_thread() );
    static GdiPlusFontManager cache;
    return cache;
}

not_null_shared<const GdiPlusFontData>
GdiPlusFontManager::Load( const dom::FontDescription& fontDescription, bool isUnderlined, bool isStrikeout ) const
{
    auto fontId = fontDescription.cssFont;
    ranges::transform( fontId, fontId.begin(), []( wint_t c ) { return std::towlower( c ); } );

    if ( isUnderlined )
    {
        fontId += L" underline";
    }
    if ( isStrikeout )
    {
        fontId += L" line-through";
    }

    if ( idToFont_.contains( fontId ) )
    {
        return idToFont_.at( fontId );
    }

    const auto [it, isEmplaced] = idToFont_.try_emplace( fontId, FetchFont( fontDescription, isUnderlined, isStrikeout ) );
    return it->second;
}

void GdiPlusFontManager::ClearCache()
{
    idToFont_.clear();
}

} // namespace smp
