#include <stdafx.h>

#include "text_helpers.h"

#include <MLang.h>

#include <cwctype>
#include <optional>
#include <span>

namespace
{

using namespace smp::utils;

bool is_wrap_char( wchar_t current, wchar_t next )
{
    if ( std::iswpunct( current ) )
    {
        return false;
    }

    if ( next == '\0' )
    {
        return true;
    }

    if ( std::iswspace( current ) )
    {
        return true;
    }

    bool currentAlphaNum = !!std::iswalnum( current );

    if ( currentAlphaNum )
    {
        if ( std::iswpunct( next ) )
        {
            return false;
        }
    }

    return !currentAlphaNum || !std::iswalnum( next );
}

void estimate_line_wrap_recur( HDC hdc, std::wstring_view text, size_t width, std::vector<wrapped_item>& out )
{
    const size_t textWidth = get_text_width( hdc, text );
    if ( textWidth <= width || text.size() <= 1 )
    {
        out.emplace_back(
            wrapped_item{
                text,
                textWidth } );
    }
    else
    {
        size_t textLength = ( text.size() * width ) / textWidth;

        if ( get_text_width( hdc, text.substr( 0, textLength ) ) < width )
        {
            while ( get_text_width( hdc, text.substr( 0, std::min( text.size(), textLength + 1 ) ) ) <= width )
            {
                ++textLength;
            }
        }
        else
        {
            while ( get_text_width( hdc, text.substr( 0, textLength ) ) > width && textLength > 1 )
            {
                --textLength;
            }
        }

        {
            size_t fallbackTextLength = std::max<size_t>( textLength, 1 );

            while ( textLength > 0 && !is_wrap_char( text[textLength - 1], text[textLength] ) )
            {
                --textLength;
            }

            if ( !textLength )
            {
                textLength = fallbackTextLength;
            }

            out.emplace_back(
                wrapped_item{
                    text.substr( 0, textLength ),
                    get_text_width( hdc, text.substr( 0, textLength ) ) } );
        }

        if ( textLength < text.size() )
        {
            estimate_line_wrap_recur( hdc, text.substr( textLength ), width, out );
        }
    }
}

} // namespace

namespace smp::utils
{

size_t get_text_height( HDC hdc, std::wstring_view text )
{
    SIZE size;
    // TODO: add error checks
    GetTextExtentPoint32( hdc, text.data(), static_cast<int>( text.size() ), &size );
    return static_cast<size_t>( size.cy );
}

size_t get_text_width( HDC hdc, std::wstring_view text )
{
    SIZE size;
    size_t count = GetKerningPairs( hdc, 0, 0 );
    if ( count > 0 )
    {
        // If font has kerning pairs then GetTextExtentPoint32 will return an incorrect width if those pairs exist in text.
        // Use DrawText in this case to provide more accurate values.
        RECT rc_calc{ 0, 0, 0, 0 };
        DrawText( hdc, text.data(), -1, &rc_calc, DT_CALCRECT );
        return rc_calc.right;
    }
    else
    {
        // TODO: add error checks
        GetTextExtentPoint32( hdc, text.data(), static_cast<int>( text.size() ), &size );
    }
    return static_cast<size_t>( size.cx );
}

std::vector<wrapped_item> estimate_line_wrap( HDC hdc, const std::wstring& text, size_t width )
{
    std::vector<wrapped_item> lines;

    const wchar_t* curTextPos = text.c_str();
    while ( true )
    {
        const wchar_t* next = wcschr( curTextPos, '\n' );
        if ( !next )
        {
            estimate_line_wrap_recur( hdc, std::wstring_view( curTextPos ), width, lines );
            break;
        }

        const wchar_t* walk = next;
        while ( walk > curTextPos && walk[-1] == '\r' )
        {
            --walk;
        }

        estimate_line_wrap_recur( hdc, std::wstring_view( curTextPos, walk - curTextPos ), width, lines );
        curTextPos = next + 1;
    }

    return lines;
}

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const std::wstring& textId, size_t index )
    : textId( std::wstring{ ' ' } + textId )
    , index( index )
{
    // space is needed for StrCmpLogicalW bug workaround
}

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const qwr::u8string_view& textId, size_t index )
    : textId( L' ' + qwr::unicode::ToWide( textId ) )
    , index( index )
{
    // space is needed for StrCmpLogicalW bug workaround
}

} // namespace smp::utils
