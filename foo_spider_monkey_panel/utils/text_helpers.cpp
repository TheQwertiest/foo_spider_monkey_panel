#include <stdafx.h>
#include "text_helpers.h"

#include <MLang.h>

namespace
{

using namespace smp::utils;

UINT FilterEncodings( const DetectEncodingInfo encodings[], int encodingCount, std::string_view text )
{
    UINT codepage = encodings[0].nCodePage;

    // MLang fine tunes
    if ( encodingCount == 2 && encodings[0].nCodePage == 1252 )
    {
        switch ( encodings[1].nCodePage )
        {
        case 850:
        case 65001:
        {
            codepage = 65001;
            break;
        }
        // DBCS
        case 932: // shift-jis
        case 936: // gbk
        case 949: // korean
        case 950: // big5
        {            
            // '¡¯', <= special char
            // "ve" "d" "ll" "m" 't' 're'
            bool fallback = true;

            constexpr char pattern[] = "\x92";
            auto pPos = std::search( text.cbegin(), text.cend(), pattern, pattern + sizeof( pattern ) - 1 );
            if ( pPos != text.cend() )
            {
                constexpr char pattern2[] = "vldmtr ";
                pPos = std::search( text.cbegin(), text.cend(), pattern, pattern2 + sizeof( pattern2 ) - 1 );
                if ( pPos != text.cend() )
                {
                    codepage = encodings[0].nCodePage;
                    fallback = false;
                }
            }

            if ( fallback )
            {
                codepage = encodings[1].nCodePage;
            }

            break;
        }
        }
    }

    if ( codepage == 20127 )
    { // ASCII?
        codepage = CP_ACP;
    }

    return codepage;
}

std::wstring make_sort_string( const char* in )
{
    std::wstring out( pfc::stringcvt::estimate_utf8_to_wide( in ) + 1, 0 );
    out[0] = ' '; //StrCmpLogicalW bug workaround.
    pfc::stringcvt::convert_utf8_to_wide_unchecked( out.data() + 1, in );
    return out;
}

int is_wrap_char( wchar_t current, wchar_t next )
{
    if ( iswpunct( current ) )
        return false;

    if ( next == '\0' )
        return true;

    if ( iswspace( current ) )
        return true;

    int currentAlphaNum = iswalnum( current );

    if ( currentAlphaNum )
    {
        if ( iswpunct( next ) )
            return false;
    }

    return currentAlphaNum == 0 || iswalnum( next ) == 0;
}

void estimate_line_wrap_recur( HDC hdc, std::wstring_view text, size_t width, std::list<wrapped_item>& out )
{
    size_t textLength = text.size();
    size_t textWidth = get_text_width( hdc, text );

    if ( textWidth <= width || text.size() <= 1 )
    {
        out.emplace_back(
            wrapped_item{
                std::wstring{ text.data(), text.size() },
                textWidth } );
    }
    else
    {
        textLength = ( text.size() * width ) / textWidth;

        if ( get_text_width( hdc, std::wstring_view( text.data(), textLength ) ) < width )
        {
            while ( get_text_width( hdc, std::wstring_view( text.data(), std::min( text.size(), textLength + 1 ) ) ) <= width )
            {
                ++textLength;
            }
        }
        else
        {
            while ( get_text_width( hdc, std::wstring_view( text.data(), textLength ) ) > width && textLength > 1 )
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
                    std::wstring{ text.data(), textLength },
                    get_text_width( hdc, std::wstring_view( text.data(), textLength ) ) } );
        }

        if ( textLength < text.size() )
        {
            estimate_line_wrap_recur( hdc, std::wstring_view( text.data() + textLength, text.size() - textLength ), width, out );
        }
    }
}

} // namespace

namespace smp::utils
{

UINT detect_text_charset( std::string_view text )
{
    _COM_SMARTPTR_TYPEDEF( IMultiLanguage2, IID_IMultiLanguage2 );
    IMultiLanguage2Ptr lang;

    HRESULT hr = lang.CreateInstance( CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER );
    if ( FAILED( hr ) )
    {
        return CP_ACP;
    }

    const int maxEncodings = 2;
    int encodingCount = maxEncodings;
    DetectEncodingInfo encodings[maxEncodings];
    int iTextSize = text.size();

    hr = lang->DetectInputCodepage( MLDETECTCP_NONE, 0, (char*)text.data(), &iTextSize, encodings, &encodingCount );
    if ( FAILED( hr ) )
    {
        return CP_ACP;
    }

    return FilterEncodings( encodings, encodingCount, text );
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

void estimate_line_wrap( HDC hdc, const std::wstring& text, size_t width, std::list<wrapped_item>& out )
{
    const wchar_t* curTextPos = text.c_str();
    while ( true )
    {
        const wchar_t* next = wcschr( curTextPos, '\n' );
        if ( !next )
        {
            estimate_line_wrap_recur( hdc, std::wstring_view( curTextPos ), width, out );
            break;
        }

        const wchar_t* walk = next;
        while ( walk > curTextPos && walk[-1] == '\r' )
        {
            --walk;
        }

        estimate_line_wrap_recur( hdc, std::wstring_view( curTextPos, walk - curTextPos ), width, out );
        curTextPos = next + 1;
    }
}

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const std::wstring& textId, size_t index )
    : textId( std::wstring{ ' ' } + textId )
    , index( index )
{
}

StrCmpLogicalCmpData::StrCmpLogicalCmpData( const pfc::string8_fast& textId, size_t index )
    : textId( make_sort_string( textId ) )
    , index( index )
{
}

} // namespace smp::utils
