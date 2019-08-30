#pragma once

#include <string>
#include <list>

namespace smp::utils
{

UINT detect_text_charset( std::string_view text );

size_t get_text_height( HDC hdc, std::wstring_view text );
size_t get_text_width( HDC hdc, std::wstring_view text );

struct wrapped_item
{
    std::wstring_view text;
    size_t width;
};
std::vector<smp::utils::wrapped_item> estimate_line_wrap( HDC hdc, const std::wstring& text, size_t width );

struct StrCmpLogicalCmpData
{
    StrCmpLogicalCmpData( const std::wstring& textId, size_t index );
    StrCmpLogicalCmpData( const std::u8string_view& textId, size_t index );

    std::wstring textId; ///< if set manually (not via ctor), must be prepended with ` ` for StrCmpLogicalW bug workaround
    size_t index;
};

template <int8_t direction = 1>
bool StrCmpLogicalCmp( const StrCmpLogicalCmpData& a, const StrCmpLogicalCmpData& b )
{
    int ret = direction * StrCmpLogicalW( a.textId.c_str(), b.textId.c_str() );
    if ( !ret )
    {
        return ( a.index < b.index );
    }
    else
    {
        return ( ret < 0 );
    }
}

} // namespace smp::utils
