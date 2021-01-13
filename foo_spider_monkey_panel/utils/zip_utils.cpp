#include <stdafx.h>

#include "zip_utils.h"

#include <miniz/miniz.h>

#include <qwr/final_action.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace smp
{

ZipPacker::ZipPacker( const fs::path& zipFile )
    : pZip_( new mz_zip_archive{} )
    , zipFile_( zipFile )
{
    try
    {
        if ( fs::exists( zipFile_ ) )
        {
            qwr::QwrException::ExpectTrue( fs::is_regular_file( zipFile_ ), "Can't create zip file: non-deleteable item with the same name already exists" );
            fs::remove( zipFile_ );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }

    const auto zRet = mz_zip_writer_init_file( pZip_.get(), zipFile_.u8string().c_str(), 0 );
    qwr::QwrException::ExpectTrue( !!zRet, "mz_zip_writer_init_file failed: {:#x}", pZip_->m_last_error );
}

ZipPacker::~ZipPacker()
{
    const bool hasFailed = ( pZip_->m_last_error != 0 || pZip_->m_zip_mode != MZ_ZIP_MODE_INVALID );
    if ( !hasFailed )
    {
        return;
    }

    if ( pZip_->m_zip_mode == MZ_ZIP_MODE_WRITING || pZip_->m_zip_mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED )
    {
        mz_zip_writer_end( pZip_.get() );
    }

    try
    {
        if ( fs::exists( zipFile_ ) && fs::is_regular_file( zipFile_ ) )
        {
            fs::remove( zipFile_ );
        }
    }
    catch ( const fs::filesystem_error& )
    {
    }
}

void ZipPacker::AddFile( const fs::path& srcFile, const std::u8string& destFileName )
{
    auto zRet = mz_zip_writer_add_file( pZip_.get(), destFileName.c_str(), srcFile.u8string().c_str(), "", 0, MZ_BEST_COMPRESSION );
    qwr::QwrException::ExpectTrue( !!zRet,
                                   "Failed to add file to archive: `{}`\n"
                                   "  mz_zip_writer_add_file failed: {:#x}",
                                   srcFile.filename().u8string(),
                                   pZip_->m_last_error );
}
void ZipPacker::AddFolder( const fs::path& srcFolder, const std::u8string& destFolderName )
{
    try
    {
        for ( const auto it: fs::recursive_directory_iterator( srcFolder ) )
        {
            if ( it.is_directory() )
            {
                continue;
            }

            fs::path dstFilePath = fs::relative( it.path(), srcFolder );
            if ( !destFolderName.empty() )
            {
                dstFilePath = fs::u8path( destFolderName ) / dstFilePath;
            }
            AddFile( it.path(), dstFilePath.u8string() );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void ZipPacker::Finish()
{
    auto zRet = mz_zip_writer_finalize_archive( pZip_.get() );
    qwr::QwrException::ExpectTrue( !!zRet, "mz_zip_writer_finalize_archive failed: {:#x}", pZip_->m_last_error );

    zRet = mz_zip_writer_end( pZip_.get() );
    qwr::QwrException::ExpectTrue( !!zRet, "mz_zip_writer_end failed: {:#x}", pZip_->m_last_error );
}

void UnpackZip( const fs::path& zipFile, const fs::path& dstFolder )
{
    try
    {
        qwr::QwrException::ExpectTrue( fs::exists( zipFile ), "File does not exist" );
        qwr::QwrException::ExpectTrue( fs::is_regular_file( zipFile ), "File is not a zip archive" );

        if ( fs::exists( dstFolder ) )
        {
            qwr::QwrException::ExpectTrue( fs::is_directory( dstFolder ), "Destination is not a folder" );
        }
        else
        {
            fs::create_directories( dstFolder );
        }

        mz_zip_archive mzZip{};
        auto zRet = mz_zip_reader_init_file( &mzZip, zipFile.u8string().c_str(), 0 );
        qwr::QwrException::ExpectTrue( !!zRet, "mz_zip_reader_init_file failed: {:#x}", mzZip.m_last_error );

        qwr::final_action autoZip( [&] { mz_zip_reader_end( &mzZip ); } );

        const size_t fileCount = (size_t)mz_zip_reader_get_num_files( &mzZip );
        if ( !fileCount )
        {
            return;
        }

        for ( const auto i: ranges::views::indices( fileCount ) )
        {
            mz_zip_archive_file_stat zFileStat;
            zRet = mz_zip_reader_file_stat( &mzZip, i, &zFileStat );
            qwr::QwrException::ExpectTrue( !!zRet, "mz_zip_reader_file_stat failed: {:#x}", mzZip.m_last_error );

            assert( zFileStat.m_filename );
            const fs::path curPath = dstFolder / qwr::unicode::ToWide( std::u8string_view{ zFileStat.m_filename, strlen( zFileStat.m_filename ) } );
            if ( zFileStat.m_is_directory )
            {
                if ( !fs::exists( curPath ) )
                {
                    fs::create_directories( curPath );
                }
            }
            else
            {
                if ( !fs::exists( curPath.parent_path() ) )
                {
                    fs::create_directories( curPath.parent_path() );
                }
                zRet = mz_zip_reader_extract_to_file( &mzZip, i, curPath.u8string().c_str(), 0 );
                qwr::QwrException::ExpectTrue( !!zRet, "mz_zip_reader_extract_to_file failed: {:#x}", mzZip.m_last_error );
            }
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace smp
