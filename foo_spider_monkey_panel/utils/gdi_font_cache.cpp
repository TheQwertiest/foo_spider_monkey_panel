#include <stdafx.h>

#include "gdi_font_cache.h"

#include <mutex>
#include <execution>
#include <qwr/final_action.h>

namespace smp::gdi
{

FontCache& FontCache::Instance()
{
    static FontCache fontCache;
    return fontCache;
}

FontCache::container FontCache::cache = {};

// FIXME: call after/from JS GC instead?
inline constexpr size_t purge_freq = 32; // purge map every nth access
static size_t access_count = 0;

shared_hfont FontCache::CacheFontW( const LOGFONTW& logfont ) noexcept
{
    ++access_count;

    static std::mutex mtx;
    std::scoped_lock<std::mutex> hold( mtx );

    // find a cached hfont, if any
    shared_hfont font = cache[logfont].lock();

    if ( !font )
    {
        // cache miss, add the logfont to cache, return the hfont
        cache[logfont] = ( font = unique_hfont( CreateFontIndirectW( &logfont ) ) );
    }

    RemoveUnused();

    return font;
};

size_t FontCache::RemoveUnused( bool force ) noexcept
{
    // FIXME: call after/from JS GC instead?
    if ( ( !force ) && ( access_count % purge_freq ) )
        return 0;

    return std::erase_if( cache, []( const auto& item ) {
        return item.second.expired();
    } );
};

void FontCache::Enumerate( FontCache::enumproc callback ) const
{
    std::for_each( std::execution::seq, cbegin( cache ), cend( cache ), callback );
}

void FontCache::NornalizeLogfontW( const HDC dc, LOGFONTW& logfont )
{
    smp::gdi::TempObjectSelector autoFont( dc, CreateFontIndirectW( &logfont ) );

    TEXTMETRICW metric = {};
    GetTextMetricsW( dc, &metric );

    logfont.lfHeight = logfont.lfHeight == 0
                ? metric.tmHeight - metric.tmInternalLeading
                : metric.tmHeight;
}

void FontCache::NornalizeLogfontW( const HWND wnd, LOGFONTW& logfont )
{
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    NornalizeLogfontW( dc, logfont );
}

} // namespace smp::gdi
