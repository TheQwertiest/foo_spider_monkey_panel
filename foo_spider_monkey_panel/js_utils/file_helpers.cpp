#include <stdafx.h>
#include "file_helpers.h"

#include <js_utils/scope_helper.h>

#include <helpers.h>

#include <filesystem>


namespace mozjs::file
{

std::wstring CleanPath( const std::wstring &path )
{// TODO: replace with `ReplaceChar` or ReplaceString
    std::wstring cleanedPath = path;
    for ( auto& curChar : cleanedPath )
    {
        if ( L'/' == curChar )
        {
            curChar = L'\\';
        }
    }

    return cleanedPath;
}

std::optional<std::wstring> ReadFromFile( JSContext* cx, const pfc::string8_fast& path, uint32_t codepage )
{
    const auto cleanPath = [&]
    {
        pfc::string8_fast tmpPath = path;
        tmpPath.replace_string( "/", "\\", 0 );
        return tmpPath;
    }();

    namespace fs = std::filesystem;

    fs::path fsPath = fs::u8path( cleanPath.c_str() );
    std::error_code dummyErr;
    if ( !fs::exists( fsPath ) || !fs::is_regular_file( fsPath, dummyErr ) )
    {
        JS_ReportErrorUTF8( cx, "Path does not point to a valid file: %s", cleanPath.c_str() );
        return std::nullopt;
    }

    // Prepare file

    HANDLE hFile = CreateFile( fsPath.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( !hFile )
    {
        JS_ReportErrorUTF8( cx, "Failed to open script file: %s", cleanPath.c_str() );
        return std::nullopt;
    }
    scope::final_action autoFile( [hFile]()
    {
        CloseHandle( hFile );
    } );

    HANDLE hFileMapping = CreateFileMapping( hFile, nullptr, PAGE_READONLY, 0, 0, nullptr );
    if ( !hFileMapping )
    {
        JS_ReportErrorUTF8( cx, "Internal error: CreateFileMapping failed for `%s`", cleanPath.c_str() );
        return std::nullopt;
    }
    scope::final_action autoMapping( [hFileMapping]()
    {
        CloseHandle( hFileMapping );
    } );

    DWORD dwFileSize = GetFileSize( hFile, nullptr );
    LPCBYTE pAddr = (LPCBYTE)MapViewOfFile( hFileMapping, FILE_MAP_READ, 0, 0, 0 );
    if ( !pAddr )
    {
        JS_ReportErrorUTF8( cx, "Internal error: MapViewOfFile failed for `%s`", cleanPath.c_str() );
        return std::nullopt;
    }
    scope::final_action autoAddress( [pAddr]()
    {
        UnmapViewOfFile( pAddr );
    } );

    if ( dwFileSize == INVALID_FILE_SIZE )
    {
        JS_ReportErrorUTF8( cx, "Internal error: failed to read file size of `%s`", cleanPath.c_str() );
        return std::nullopt;
    }

    // Read file

    std::wstring fileContent;

    const unsigned char bom32Be[] = { 0x00, 0x00, 0xfe, 0xff };
    const unsigned char bom32Le[] = { 0xff, 0xfe, 0x00, 0x00 };
    const unsigned char bom16Be[] = { 0xfe, 0xff };// must be 4byte size
    const unsigned char bom16Le[] = { 0xff, 0xfe };// must be 4byte size, but not 0xff, 0xfe, 0x00, 0x00
    const unsigned char bom8[] = { 0xef, 0xbb, 0xbf };

    // TODO: handle all other BOM cases as well

    if ( codepage )
    {
        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( codepage, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( codepage, fileContent.data(), outputSize, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );
    }
    else if ( dwFileSize >= 4
              && !memcmp( bom16Le, pAddr, sizeof( bom16Le ) ) )
    {
        pAddr += sizeof( bom16Le );
        dwFileSize -= sizeof( bom16Le );

        const size_t outputSize = (dwFileSize >> 1) + 1;
        fileContent.resize( outputSize );

        memcpy( fileContent.data(), (const char*)pAddr, dwFileSize );
        fileContent[outputSize] = 0;
    }
    else if ( dwFileSize >= sizeof( bom8 )
              && !memcmp( bom8, pAddr, sizeof( bom8 ) ) )
    {
        pAddr += sizeof( bom8 );
        dwFileSize -= sizeof( bom8 );

        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( CP_UTF8, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( CP_UTF8, fileContent.data(), outputSize, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );
    }
    else
    {
        uint32_t detectedCodepage = helpers::detect_text_charset( (const char*)pAddr, dwFileSize );

        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( detectedCodepage, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( detectedCodepage, fileContent.data(), outputSize, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );
    }

    return fileContent;
}

}
