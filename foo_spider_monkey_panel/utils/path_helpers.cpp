#include <stdafx.h>

#include "path_helpers.h"

namespace fs = std::filesystem;

namespace smp::utils
{

std::vector<fs::path> GetFilesRecursive( const fs::path& path )
{
    try
    {
        assert( fs::is_directory( path ) );

        std::vector<fs::path> files;

        for ( const auto& it: fs::recursive_directory_iterator( path ) )
        {
            if ( it.is_directory() )
            {
                continue;
            }
            files.emplace_back( it.path() );
        }

        return files;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace smp::utils
