#pragma once

#include <filesystem>

struct mz_zip_archive;

namespace smp
{

class ZipPacker
{
public:
    /// @throw SmpException
    [[nodiscard]] ZipPacker( const std::filesystem::path& zipFile );

    /// @remark Deletes zip file on error
    ~ZipPacker();

    /// @throw SmpException
    void AddFile( const std::filesystem::path& srcFile, const qwr::u8string& destFileName );
    /// @throw SmpException
    void AddFolder( const std::filesystem::path& srcFolder, const qwr::u8string& destFolderName = "" );
    /// @throw SmpException
    void Finish();

private:
    std::unique_ptr<mz_zip_archive> pZip_;
    std::filesystem::path zipFile_;
};

/// @throw SmpException
void UnpackZip( const std::filesystem::path& zipFile, const std::filesystem::path& dstFolder );

} // namespace smp
