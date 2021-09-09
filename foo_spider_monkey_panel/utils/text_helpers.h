#pragma once

#include <list>
#include <string>

namespace smp::utils
{

[[nodiscard]] size_t GetTextHeight( HDC hdc, std::wstring_view text );
[[nodiscard]] size_t GetTextWidth( HDC hdc, std::wstring_view text, bool accurate = false );

struct WrappedTextLine
{
    std::wstring_view text;
    size_t width;
};
[[nodiscard]] std::vector<WrappedTextLine> WrapText( HDC hdc, const std::wstring& text, size_t maxWidth );

struct StrCmpLogicalCmpData
{
    StrCmpLogicalCmpData( const std::wstring& textId, size_t index );
    StrCmpLogicalCmpData( const qwr::u8string_view& textId, size_t index );

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
