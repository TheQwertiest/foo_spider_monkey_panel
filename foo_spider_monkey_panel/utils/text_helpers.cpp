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

void WrapTextRecur( HDC hdc, std::wstring_view text, size_t width, std::vector<WrappedTextLine>& out )
{
    const auto textWidth = GetTextWidth( hdc, text );
    if ( textWidth <= width || text.size() <= 1 )
    {
        out.emplace_back(
            WrappedTextLine{
                text,
                textWidth } );
    }
    else
    {
        size_t textLength = ( text.size() * width ) / textWidth;

        if ( GetTextWidth( hdc, text.substr( 0, textLength ) ) < width )
        {
            while ( GetTextWidth( hdc, text.substr( 0, std::min( text.size(), textLength + 1 ) ) ) <= width )
            {
                ++textLength;
            }
        }
        else
        {
            while ( GetTextWidth( hdc, text.substr( 0, textLength ) ) > width && textLength > 1 )
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
                WrappedTextLine{
                    text.substr( 0, textLength ),
                    GetTextWidth( hdc, text.substr( 0, textLength ) ) } );
        }

        if ( textLength < text.size() )
        {
            WrapTextRecur( hdc, text.substr( textLength ), width, out );
        }
    }
}

} // namespace

namespace smp::utils
{

size_t GetTextHeight( HDC hdc, std::wstring_view text )
{
    SIZE size;
    // TODO: add error checks
    GetTextExtentPoint32( hdc, text.data(), static_cast<int>( text.size() ), &size );
    return static_cast<size_t>( size.cy );
}

size_t GetTextWidth( HDC hdc, std::wstring_view text, bool accurate )
{
    // If font has kerning pairs then GetTextExtentPoint32 will return an inaccurate width if those pairs exist in text.
    // DrawText returns a completely accurate value, but is slower and should not be called from inside estimate_line_wrap
    if ( accurate && text.size() > 1 && GetKerningPairs( hdc, 0, 0 ) > 0 )
    {
        RECT rc_calc{ 0, 0, 0, 0 };
        DrawText( hdc, text.data(), -1, &rc_calc, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE );
        return rc_calc.right;
    }
    else
    {
        SIZE size;
        // TODO: add error checks
        GetTextExtentPoint32( hdc, text.data(), static_cast<int>( text.size() ), &size );
        return static_cast<size_t>( size.cx );
    }
}

std::vector<WrappedTextLine> WrapText( HDC hdc, const std::wstring& text, size_t maxWidth )
{
    std::vector<WrappedTextLine> lines;

    const wchar_t* curTextPos = text.c_str();
    while ( true )
    {
        const wchar_t* next = wcschr( curTextPos, '\n' );
        if ( !next )
        {
            WrapTextRecur( hdc, std::wstring_view( curTextPos ), maxWidth, lines );
            break;
        }

        const wchar_t* walk = next;
        while ( walk > curTextPos && walk[-1] == '\r' )
        {
            --walk;
        }

        WrapTextRecur( hdc, std::wstring_view( curTextPos, walk - curTextPos ), maxWidth, lines );
        curTextPos = next + 1;
    }

    return lines;
}

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const std::wstring& textId, size_t index )
    : textId( fmt::format( L" {}", textId ) )
    , index( index )
{
    // additional space is needed for StrCmpLogicalW bug workaround
}

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const qwr::u8string_view& textId, size_t index )
    : textId( fmt::format( L" {}", qwr::unicode::ToWide( textId ) ) )
    , index( index )
{
    // additional space is needed for StrCmpLogicalW bug workaround
}

} // namespace smp::utils
