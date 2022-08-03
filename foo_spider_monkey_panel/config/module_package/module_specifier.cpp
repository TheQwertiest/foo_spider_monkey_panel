#include <stdafx.h>

#include "module_specifier.h"

#include <component_paths.h>

#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

namespace smp::config
{

void VerifyModulePackageName( const qwr::u8string& name )
{
    try
    {
        std::regex regexp( "^@[a-z0-9][a-z0-9\\.\\-_]{2,}/[a-z0-9][a-z0-9\\.\\-_]{2,}$" );
        if ( !std::regex_search( name, regexp ) )
        {
            throw qwr::QwrException( "Invalid package name `{}`: must be in @AUTHOR/PACKAGE_NAME format", name );
        }
    }
    catch ( const std::runtime_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

std::tuple<qwr::u8string, qwr::u8string> ParseBareModuleSpecifier( const qwr::u8string& moduleSpecifier )
{
    try
    {
        // Note: same as package name, but with subfolders allowed
        std::regex regexp( "^(@[a-z0-9][a-z0-9\\.\\-_]{2,}/[a-z0-9][a-z0-9\\.\\-_]{2,})((?:/[a-z0-9][a-z0-9\\.\\-_]{2,})*)$" );
        std::smatch match;
        if ( !std::regex_match( moduleSpecifier, match, regexp ) )
        {
            throw qwr::QwrException( "Invalid module specifier `{}`: must be in @AUTHOR/PACKAGE_NAME format", moduleSpecifier );
        }
        assert( match.size() == 3 );

        return std::make_tuple( match[1].str(), match[2].str() );
    }
    catch ( const std::runtime_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

} // namespace smp::config
