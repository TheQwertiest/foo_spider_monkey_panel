#include <stdafx.h>
#include "file_helpers.h"

#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>
#include <utils/text_helpers.h>

#include <abort_callback.h>

#include <filesystem>

namespace
{

using namespace smp;
using namespace smp::file;

constexpr unsigned char kBom32Be[] = { 0x00, 0x00, 0xfe, 0xff };
constexpr unsigned char kBom32Le[] = { 0xff, 0xfe, 0x00, 0x00 };
constexpr unsigned char kBom16Be[] = { 0xfe, 0xff }; // must be 4byte size
constexpr unsigned char kBom16Le[] = { 0xff, 0xfe }; // must be 4byte size, but not 0xff, 0xfe, 0x00, 0x00
constexpr unsigned char kBom8[] = { 0xef, 0xbb, 0xbf };

// TODO: dirty hack! remove
std::unordered_map<std::wstring, UINT> codepageMap;

template <typename T>
T ConvertFileContent( const std::wstring& path, std::string_view content, UINT codepage )
{
    T fileContent;

    constexpr bool isWide = std::is_same_v<T, std::wstring>;

    const char* curPos = content.data();
    size_t curSize = content.size();

    UINT detectedCodepage = codepage;
    bool isWideCodepage = false;
    if ( curSize >= 4
         && !memcmp( kBom16Le, curPos, sizeof( kBom16Le ) ) )
    {
        curPos += sizeof( kBom16Le );
        curSize -= sizeof( kBom16Le );

        isWideCodepage = true;
    }
    else if ( curSize >= sizeof( kBom8 )
              && !memcmp( kBom8, curPos, sizeof( kBom8 ) ) )
    {
        curPos += sizeof( kBom8 );
        curSize -= sizeof( kBom8 );

        detectedCodepage = CP_UTF8;
    }
    
    if ( !isWideCodepage && detectedCodepage == CP_ACP )
    { // TODO: dirty hack! remove
        if ( codepageMap.count( path ) )
        {
            detectedCodepage = codepageMap[path];
        }
        else
        {
            detectedCodepage = smp::utils::detect_text_charset( std::string_view{ curPos, curSize } );
            codepageMap[path] = detectedCodepage;
        }
    }

    if ( isWideCodepage )
    {
        auto convertToWide = [curPos, curSize] {
            std::wstring tmpString;

            const size_t outputSize = curSize >> 1;

            tmpString.resize( outputSize );
            // Can't use wstring.assign(), because of potential aliasing issues
            memcpy( tmpString.data(), curPos, outputSize );

            return tmpString;
        };

        if constexpr ( isWide )
        {
            fileContent = convertToWide();
        }
        else
        {
            const auto wContent = convertToWide();
            fileContent = pfc::stringcvt::string_utf8_from_wide{ wContent.c_str(), wContent.length() };
        }
    }
    else
    {
        auto codepageToWide = [curPos, curSize, detectedCodepage] {
            std::wstring tmpString;
            size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( detectedCodepage, curPos, curSize );
            tmpString.resize( outputSize );

            outputSize = pfc::stringcvt::convert_codepage_to_wide( detectedCodepage, tmpString.data(), outputSize, curPos, curSize );
            tmpString.resize( outputSize );

            return tmpString;
        };

        if constexpr ( isWide )
        {
            fileContent = codepageToWide();
        }
        else
        {
            if ( CP_UTF8 == detectedCodepage )
            {
                fileContent.set_string( curPos, curSize );
            }
            else
            {
                const std::wstring tmpString = codepageToWide();
                fileContent = pfc::stringcvt::string_utf8_from_wide( tmpString.c_str(), tmpString.length() );
            }
        }
    }

    return fileContent;
}

}

namespace {

class FileReader
{
public:
    FileReader( const pfc::string8_fast& path );
    ~FileReader();
    FileReader( const FileReader &) = delete;
    FileReader& operator =( const FileReader& ) = delete;

    std::string_view GetFileContent() const;

private:
    std::string_view fileContent_;

    HANDLE hFile_ = nullptr;
    HANDLE hFileMapping_ = nullptr;
    LPCBYTE pFileView_ = nullptr;
    size_t fileSize_ = 0;
};

FileReader::FileReader( const pfc::string8_fast& path )
{
    namespace fs = std::filesystem;

    fs::path fsPath = fs::u8path( path.c_str() );
    std::error_code dummyErr;
    if ( !fs::exists( fsPath ) || !fs::is_regular_file( fsPath, dummyErr ) )
    {
        throw SmpException( smp::string::Formatter() << "Path does not point to a valid file: " << path.c_str() );
    }

    if ( !fs::file_size( fsPath ) )
    { // CreateFileMapping fails on file with zero length, so we need to bail out early
        return;
    }

    // Prepare file

    hFile_ = CreateFile( fsPath.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( !hFile_ )
    {
        throw SmpException( smp::string::Formatter() << "Failed to open script file: " << path.c_str() );
    }
    utils::final_action autoFile( [hFile = hFile_]() {
        CloseHandle( hFile );
    } );

    hFileMapping_ = CreateFileMapping( hFile_, nullptr, PAGE_READONLY, 0, 0, nullptr );
    if ( !hFileMapping_ )
    {
        throw SmpException( smp::string::Formatter() << "Internal error: CreateFileMapping failed for `" << path.c_str() << "`" );
    }
    utils::final_action autoMapping( [hFileMapping = hFileMapping_]() {
        CloseHandle( hFileMapping );
    } );

    fileSize_ = GetFileSize( hFile_, nullptr );
    pFileView_ = (LPCBYTE)MapViewOfFile( hFileMapping_, FILE_MAP_READ, 0, 0, 0 );
    if ( !pFileView_ )
    {
        throw SmpException( smp::string::Formatter() << "Internal error: MapViewOfFile failed for `" << path.c_str() << "`" );
    }
    utils::final_action autoAddress( [pFileView = pFileView_]() {
        UnmapViewOfFile( pFileView );
    } );

    if ( fileSize_ == INVALID_FILE_SIZE )
    {
        throw SmpException( smp::string::Formatter() << "Internal error: failed to read file size of `" << path.c_str() << "`" );
    }

    autoAddress.cancel();
    autoMapping.cancel();
    autoFile.cancel();
}

FileReader::~FileReader()
{
     if ( pFileView_ )
     {
         UnmapViewOfFile( pFileView_ );
     }
     if ( hFileMapping_ )
     {
         CloseHandle( hFileMapping_ );
     }
     if ( hFile_ )
     {
         CloseHandle( hFile_ );
     }
}

std::string_view FileReader::GetFileContent() const
{
    const LPCBYTE dummyVal = reinterpret_cast<LPCBYTE>( &fileSize_ );
    return std::string_view{ reinterpret_cast<const char*>( pFileView_ ? pFileView_ : dummyVal ), fileSize_ };
}

} // namespace

namespace smp::file
{

pfc::string8_fast CleanPath( const pfc::string8_fast& path )
{
    pfc::string8_fast tmpPath = path;
    tmpPath.replace_string( "/", "\\", 0 );
    return tmpPath;
}

std::wstring CleanPathW( const std::wstring& path )
{ // TODO: replace with `ReplaceChar` or ReplaceString
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

pfc::string8_fast ReadFile( const pfc::string8_fast& path, UINT codepage )
{
    namespace fs = std::filesystem;

    FileReader fileReader( path );
    return ConvertFileContent<pfc::string8_fast>( fs::absolute( fs::u8path( path.c_str() ) ).wstring(), fileReader.GetFileContent(), codepage );
}

std::wstring ReadFileW( const pfc::string8_fast& path, UINT codepage )
{
    namespace fs = std::filesystem;

    FileReader fileReader( path );
    return ConvertFileContent<std::wstring>( fs::absolute( fs::u8path( path.c_str() ) ).wstring(), fileReader.GetFileContent(), codepage );
}

bool WriteFile( const char* path, const pfc::string_base& content, bool write_bom )
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
        memcpy( pAddr, kBom8, sizeof( kBom8 ) );
    }
    memcpy( pAddr + offset, content.get_ptr(), content.get_length() );

    UnmapViewOfFile( pAddr );
    CloseHandle( hFileMapping );
    CloseHandle( hFile );
    return true;
}

UINT DetectFileCharset( const char* fileName )
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

    return smp::utils::detect_text_charset( std::string_view{ text, text.get_length() } );
}

} // namespace smp::file
