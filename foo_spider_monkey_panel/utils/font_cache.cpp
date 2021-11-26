#include <stdafx.h>

#include "font_cache.h"

#include <mutex>
#include <execution>

namespace smp::fontcache
{
static std::unordered_map<LOGFONTW, weak_hfont> cache = {};

inline size_t Purge( bool force ) noexcept
{
    // FIXME: call after/from JS GC instead?
    constexpr size_t purge_freq = 32; // purge map every nth access
    static size_t access_count = 0;
    if ( ( !force ) && ( ++access_count % purge_freq ) )
        return 0;

    return std::erase_if( cache, []( const auto& item ) {
        return item.second.expired();
    } );
};

shared_hfont Cache( const LOGFONTW& logfont ) noexcept
{
    static std::mutex m;
    std::scoped_lock<std::mutex> hold( m );

    shared_hfont font = cache[logfont].lock();

    if ( !font )
        cache[logfont] = ( font = unique_hfont( CreateFontIndirectW( &logfont ) ) );

    Purge();

    return font;
};

void ForEach( std::function<void( const std::pair<LOGFONTW, weak_hfont>& )> callback )
{
    std::for_each(std::execution::seq, cbegin(cache), cend(cache), callback );
}

} // namespace smp::fontcache

namespace smp::logfont
{

void Make
(
    const std::wstring& _In_ fontName,
    const int32_t _In_ fontSize,
    const uint32_t _In_ fontStyle,
    LOGFONTW& _Out_ logfont
)
{
    BOOL font_smoothing = 0;
    UINT smoothing_type = 0;
    // UINT smoothing_contrast = 0;
    // UINT smoothing_orientation = 0;

    SystemParametersInfoW( SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0 );
    SystemParametersInfoW( SPI_GETFONTSMOOTHINGTYPE, 0, &smoothing_type, 0 );
    // SystemParametersInfoW( SPI_GETFONTSMOOTHINGCONTRAST,    0, &smoothing_contrast,    0 );
    // SystemParametersInfoW( SPI_GETFONTSMOOTHINGORIENTATION, 0, &smoothing_orientation, 0 );

    logfont = LOGFONTW
    {
        // size = 0  ==      " default "
        // size > 0  ==      line height px (tmHeight)  (aka em height)
        // size < 0  ==      char height px (tmHeight - tmInternalLeading)
      ( 0 - fontSize ),   // size, see above,
        0,                // avg width
        0,                // escapement, letter orientation vs baseline (only in GM_ADVANCED)
        0,                // baseline orientation, both on 0.1-units (450=45deg)
      ( fontStyle & Gdiplus::FontStyleBold      ) ? FW_BOLD : FW_NORMAL,
    !!( fontStyle & Gdiplus::FontStyleItalic    ),
    !!( fontStyle & Gdiplus::FontStyleUnderline ),
    !!( fontStyle & Gdiplus::FontStyleStrikeout ),
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        static_cast<BYTE>( font_smoothing
                               ? ( smoothing_type == FE_FONTSMOOTHINGCLEARTYPE
                                       ? CLEARTYPE_QUALITY
                                       : ANTIALIASED_QUALITY )
                               : DEFAULT_QUALITY ),
        DEFAULT_PITCH | FF_DONTCARE,
        0
    };

    fontName.copy( logfont.lfFaceName, LF_FACESIZE - 1 );
}

void Normalize
(
    const HDC _In_ dc,
    LOGFONTW& _Inout_ logfont
)
{
    if ( logfont.lfHeight > 0 )
        return;

    smp::gdi::ObjectSelector autoFont( dc, CreateFontIndirectW( &logfont ), true );

    TEXTMETRICW metric = {};
    GetTextMetricsW( dc, &metric );

    logfont.lfHeight = logfont.lfHeight == 0
                ? metric.tmHeight - metric.tmInternalLeading
                : metric.tmHeight;
}

} // namespace smp::logfont
