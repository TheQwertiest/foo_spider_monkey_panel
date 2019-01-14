#include "stdafx.h"
#include "helpers.h"

#include <com_objects/script_interface_impl.h>

#include <user_message.h>
#include <abort_callback.h>

#include <MLang.h>

namespace helpers
{

COLORREF convert_argb_to_colorref( DWORD argb )
{
    return RGB( argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT );
}

DWORD convert_colorref_to_argb( COLORREF color )
{
    // COLORREF : 0x00bbggrr
    // ARGB : 0xaarrggbb
    return ( GetRValue( color ) << RED_SHIFT )
           | ( GetGValue( color ) << GREEN_SHIFT )
           | ( GetBValue( color ) << BLUE_SHIFT )
           | 0xff000000;
}

bool read_file( const char* path, pfc::string_base& content )
{
    HANDLE hFile = uCreateFile( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        return false;
    }

    HANDLE hFileMapping = uCreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    if ( !hFileMapping )
    {
        CloseHandle( hFile );
        return false;
    }

    DWORD dwFileSize = GetFileSize( hFile, NULL );
    LPCBYTE pAddr = (LPCBYTE)MapViewOfFile( hFileMapping, FILE_MAP_READ, 0, 0, 0 );
    if ( !pAddr )
    {
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        return false;
    }

    if ( dwFileSize == INVALID_FILE_SIZE )
    {
        UnmapViewOfFile( pAddr );
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        return false;
    }

    bool status = false;

    if ( dwFileSize > 3 )
    {
        // UTF16 LE?
        if ( pAddr[0] == 0xFF && pAddr[1] == 0xFE )
        {
            const wchar_t* pSource = (const wchar_t*)( pAddr + 2 );
            t_size len = ( dwFileSize >> 1 ) - 1;

            content = pfc::stringcvt::string_utf8_from_wide( pSource, len );
            status = true;
        }
        // UTF8?
        else if ( pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF )
        {
            const char* pSource = (const char*)( pAddr + 3 );
            t_size len = dwFileSize - 3;

            content.set_string( pSource, len );
            status = true;
        }
    }

    if ( !status )
    {
        const char* pSource = (const char*)( pAddr );
        t_size pSourceSize = dwFileSize;

        t_size tmp = detect_charset( path );
        if ( tmp == CP_UTF8 )
        {
            content.set_string( pSource, pSourceSize );
        }
        else
        {
            content = pfc::stringcvt::string_utf8_from_ansi( pSource, pSourceSize );
        }
        status = true;
    }

    UnmapViewOfFile( pAddr );
    CloseHandle( hFileMapping );
    CloseHandle( hFile );
    return status;
}

bool write_file( const char* path, const pfc::string_base& content, bool write_bom )
{
    int offset = write_bom ? 3 : 0;
    HANDLE hFile = uCreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        return false;
    }

    HANDLE hFileMapping = uCreateFileMapping( hFile, NULL, PAGE_READWRITE, 0, content.get_length() + offset, NULL );
    if ( !hFileMapping )
    {
        CloseHandle( hFile );
        return false;
    }

    PBYTE pAddr = (PBYTE)MapViewOfFile( hFileMapping, FILE_MAP_WRITE, 0, 0, 0 );
    if ( !pAddr )
    {
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        return false;
    }

    if ( write_bom )
    {
        const BYTE utf8_bom[] = { 0xef, 0xbb, 0xbf };
        memcpy( pAddr, utf8_bom, 3 );
    }
    memcpy( pAddr + offset, content.get_ptr(), content.get_length() );

    UnmapViewOfFile( pAddr );
    CloseHandle( hFileMapping );
    CloseHandle( hFile );
    return true;
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

pfc::string8_fast get_fb2k_component_path()
{
    pfc::string8_fast path;
    uGetModuleFileName( core_api::get_my_instance(), path );
    path = pfc::string_directory( path );
    path.fix_dir_separator( '\\' );

    return path;
}

pfc::string8_fast get_fb2k_path()
{
    pfc::string8_fast path;
    uGetModuleFileName( nullptr, path );
    path = pfc::string_directory( path );
    path.fix_dir_separator( '\\' );

    return path;
}

pfc::string8_fast get_profile_path()
{
    pfc::string8_fast path = file_path_display( core_api::get_profile_path() );
    path.fix_dir_separator( '\\' );

    return path;
}

t_size detect_charset( const char* fileName )
{
    pfc::string8_fast text;

    try
    {
        file_ptr io;
        auto& abort = smp::GlobalAbortCallback::GetInstance();
        filesystem::g_open_read( io, fileName, abort );
        io->read_string_raw( text, abort );
    }
    catch ( const pfc::exception& )
    {
        return 0;
    }

    return detect_text_charset( const_cast<char*>( text.get_ptr() ), text.get_length() );
}

size_t detect_text_charset( const char* text, size_t textSize )
{
    _COM_SMARTPTR_TYPEDEF( IMultiLanguage2, IID_IMultiLanguage2 );
    IMultiLanguage2Ptr lang;

    HRESULT hr = lang.CreateInstance( CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER );
    if ( FAILED( hr ) )
    {
        return 0;
    }

    const int maxEncodings = 2;
    int encodingCount = maxEncodings;
    DetectEncodingInfo encodings[maxEncodings];
    int iTextSize = textSize;

    hr = lang->DetectInputCodepage( MLDETECTCP_NONE, 0, (char*)text, &iTextSize, encodings, &encodingCount );
    if ( FAILED( hr ) )
    {
        return 0;
    }

    unsigned codepage = 0;
    bool found = false;

    // MLang fine tunes
    if ( encodingCount == 2 && encodings[0].nCodePage == 1252 )
    {
        switch ( encodings[1].nCodePage )
        {
        case 850:
        case 65001:
            found = true;
            codepage = 65001;
            break;
            // DBCS
        case 932: // shift-jis
        case 936: // gbk
        case 949: // korean
        case 950: // big5
        {
            // '¡¯', <= special char
            // "ve" "d" "ll" "m" 't' 're'
            bool fallback = true;

            const char pattern[] = "\x92";
            const char* pPos = std::search( text, text + textSize, pattern, pattern + sizeof( pattern ) - 1 );
            if ( pPos != ( text + textSize ) )
            {
                const char pattern2[] = "vldmtr ";
                pPos = std::search( text, text + textSize, pattern, pattern2 + sizeof( pattern2 ) - 1 );
                if ( pPos != ( text + textSize ) )
                {
                    codepage = encodings[0].nCodePage;
                    fallback = false;
                }
            }

            if ( fallback )
            {
                codepage = encodings[1].nCodePage;
            }
            found = true;
        }
        break;
        }
    }

    if ( !found )
    {
        codepage = encodings[0].nCodePage;
    }
    // ASCII?
    if ( codepage == 20127 )
    {
        codepage = 0;
    }

    return codepage;
}

void estimate_line_wrap( HDC hdc, const std::wstring& text, size_t width, std::list<helpers::wrapped_item>& out )
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

void estimate_line_wrap_recur( HDC hdc, std::wstring_view text, size_t width, std::list<helpers::wrapped_item>& out )
{
    size_t textLength = text.size();
    size_t textWidth = get_text_width( hdc, text );

    if ( textWidth <= width || text.size() <= 1 )
    {
        out.emplace_back(
            wrapped_item{
                _bstr_t( SysAllocStringLen( text.data(), text.size() ), false ),
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
                    _bstr_t( SysAllocStringLen( text.data(), textLength ), false ),
                    get_text_width( hdc, std::wstring_view( text.data(), textLength ) ) } );
        }

        if ( textLength < text.size() )
        {
            estimate_line_wrap_recur( hdc, std::wstring_view( text.data() + textLength, text.size() - textLength ), width, out );
        }
    }
}

std::wstring make_sort_string( const char* in )
{
    std::wstring out( pfc::stringcvt::estimate_utf8_to_wide( in ) + 1, 0 );
    out[0] = ' '; //StrCmpLogicalW bug workaround.
    pfc::stringcvt::convert_utf8_to_wide_unchecked( out.data() + 1, in );
    return out;
}

js_process_locations::js_process_locations( int playlist_idx, UINT base, bool to_select )
    : m_playlist_idx( playlist_idx )
    , m_base( base )
    , m_to_select( to_select )
{
}

void js_process_locations::on_completion( metadb_handle_list_cref p_items )
{
    pfc::bit_array_val selection( m_to_select );
    auto api = playlist_manager::get();
    t_size playlist = m_playlist_idx == -1 ? api->get_active_playlist() : m_playlist_idx;

    if ( playlist < api->get_playlist_count() && !api->playlist_lock_is_present( playlist ) )
    {
        api->playlist_insert_items( playlist, m_base, p_items, selection );
        if ( m_to_select )
        {
            api->set_active_playlist( playlist );
            api->playlist_set_focus_item( playlist, m_base );
        }
    }
}

void js_process_locations::on_aborted()
{
}

} // namespace helpers
