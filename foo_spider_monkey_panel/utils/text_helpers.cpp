#include <stdafx.h>

#include "text_helpers.h"

#include <MLang.h>

#include <nonstd/span.hpp>

#include <cwctype>
#include <optional>

namespace
{

using namespace smp::utils;

enum class CodePage : UINT
{
    DosLatin1 = 850,
    ShiftJis = 932,
    Gbk = 936,
    EucKr = 949,
    Big5 = 950,
    WinLatin1 = 1252,
    UsAscii = 20127,
    Utf8 = CP_UTF8,
};

UINT FilterEncodings( nonstd::span<const DetectEncodingInfo> encodings, std::string_view text )
{
    assert( !encodings.empty() );

    if ( encodings.size() == 1 )
    {
        const auto codepage = encodings[0].nCodePage;
        if ( codepage == static_cast<UINT>( CodePage::UsAscii ) )
        { // UsAscii is a subset of Utf8
            return static_cast<UINT>( CodePage::Utf8 );
        }
        else
        {
            return codepage;
        }
    }
    else
    {
        return ranges::max_element( encodings,
                                    []( const auto& a, const auto& b ) {
                                        return ( a.nConfidence < b.nConfidence );
                                    } )
            ->nCodePage;
    }
}

int is_wrap_char( wchar_t current, wchar_t next )
{
    if ( std::iswpunct( current ) )
        return false;

    if ( next == '\0' )
        return true;

    if ( std::iswspace( current ) )
        return true;

    bool currentAlphaNum = !!std::iswalnum( current );

    if ( currentAlphaNum )
    {
        if ( std::iswpunct( next ) )
            return false;
    }

    return !currentAlphaNum || !std::iswalnum( next );
}

void estimate_line_wrap_recur( HDC hdc, std::wstring_view text, size_t width, std::vector<wrapped_item>& out )
{
    size_t textLength = text.size();
    size_t textWidth = get_text_width( hdc, text );

    if ( textWidth <= width || text.size() <= 1 )
    {
        out.emplace_back(
            wrapped_item{
                text,
                textWidth } );
    }
    else
    {
        textLength = ( text.size() * width ) / textWidth;

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
            size_t fallbackTextLength = std::max( textLength, (size_t)1 );

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

std::optional<UINT> DetectCharSet( std::string_view text )
{
    _COM_SMARTPTR_TYPEDEF( IMultiLanguage2, IID_IMultiLanguage2 );
    IMultiLanguage2Ptr lang;

    HRESULT hr = lang.CreateInstance( CLSID_CMultiLanguage, nullptr, CLSCTX_INPROC_SERVER );
    if ( FAILED( hr ) )
    {
        return std::nullopt;
    }

    const int maxEncodings = 2;
    int encodingCount = maxEncodings;
    DetectEncodingInfo encodings[maxEncodings];
    int iTextSize = text.size();

    hr = lang->DetectInputCodepage( MLDETECTCP_NONE, 0, const_cast<char*>( text.data() ), &iTextSize, encodings, &encodingCount );
    if ( FAILED( hr ) || !encodingCount )
    {
        return std::nullopt;
    }

    return FilterEncodings( nonstd::span<const DetectEncodingInfo>( encodings, encodingCount ), text );
}

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
    // TODO: add error checks
    GetTextExtentPoint32( hdc, text.data(), static_cast<int>( text.size() ), &size );
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

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const std::u8string_view& textId, size_t index )
    : textId( L' ' + smp::unicode::ToWide( textId ) )
    , index( index )
{
    // space is needed for StrCmpLogicalW bug workaround
}

} // namespace smp::utils
