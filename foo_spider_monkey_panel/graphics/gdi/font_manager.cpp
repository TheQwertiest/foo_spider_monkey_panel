#include <stdafx.h>

#include "font_manager.h"

#include <dom/css_fonts.h>
#include <graphics/gdi/object_selector.h>

#include <qwr/utility.h>
#include <qwr/winapi_error_helpers.h>

#include <cwctype>

namespace
{

/// @throw qwr::QwrException
auto FetchFont( CDCHandle cDc, const smp::dom::FontDescription& fontDescription, bool isUnderlined = false, bool isStrikeout = false )
{
    auto pFont = std::make_unique<CFont>();
    pFont->CreateFontW( -static_cast<int>( fontDescription.size ),
                        0,
                        0,
                        0,
                        fontDescription.weight,
                        fontDescription.style == smp::dom::FontStyle::italic,
                        isUnderlined,
                        isStrikeout,
                        DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY,
                        DEFAULT_PITCH | FF_DONTCARE,
                        fontDescription.family.c_str() );
    qwr::error::CheckWinApi( !!pFont, "CreateFontW" );

    smp::GdiObjectSelector autoFont( cDc, pFont->m_hFont );

    TEXTMETRICW metrics{};
    auto iRet = cDc.GetTextMetrics( &metrics );
    qwr::error::CheckWinApi( iRet, "GetTextMetrics" );

    const auto trueLineHeight = metrics.tmAscent + metrics.tmInternalLeading + metrics.tmExternalLeading + metrics.tmDescent;
    const auto magicLineHeight = fontDescription.size * trueLineHeight / metrics.tmHeight;

    return smp::make_not_null_shared<const smp::GdiFontData>(
        std::move( pFont ),
        static_cast<int32_t>( metrics.tmAscent ),
        static_cast<int32_t>( metrics.tmDescent ),
        static_cast<int32_t>( trueLineHeight ),
        static_cast<int32_t>( magicLineHeight ) );
}

} // namespace

namespace smp
{

GdiFontManager::GdiFontManager()
    // TODO: replace constant with config
    : idToFont_( 100 )
{
}

GdiFontManager& GdiFontManager::Get()
{
    assert( core_api::is_main_thread() );
    static GdiFontManager cache;
    return cache;
}

not_null_shared<const GdiFontData>
GdiFontManager::Load( CDCHandle cdc, const dom::FontDescription& fontDescription, bool isUnderlined, bool isStrikeout ) const
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

    if ( idToFont_.Contains( fontId ) )
    {
        return idToFont_.Get( fontId );
    }

    auto pFont = [&] {
        if ( idToFontWeak_.contains( fontId ) )
        { // reuse if value is still available
            auto pWeakFont = idToFontWeak_.at( fontId );
            if ( auto pFont = pWeakFont.lock() )
            {
                return not_null_shared<const GdiFontData>( pFont );
            }
        }

        return FetchFont( cdc, fontDescription, isUnderlined, isStrikeout );
    }();

    idToFont_.Put( fontId, pFont );
    idToFontWeak_.try_emplace( fontId, pFont.get() );
    return pFont;
}

void GdiFontManager::ClearCache()
{
    idToFont_.Clear();
    idToFontWeak_.clear();
}

} // namespace smp
