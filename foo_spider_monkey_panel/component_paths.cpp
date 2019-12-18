#include <stdafx.h>

#include "component_paths.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

std::u8string StringFromDirPath( const fs::path& path )
{
    const auto tmpPath = fs::path( path ).lexically_normal() / "";
    return tmpPath.u8string();
}

} // namespace

namespace smp
{

std::u8string get_fb2k_component_path()
{
    pfc::string8_fast tmp;
    uGetModuleFileName( core_api::get_my_instance(), tmp );

    return StringFromDirPath( fs::u8path( tmp.c_str() ).parent_path() );
}

std::u8string get_fb2k_path()
{
    pfc::string8_fast tmp;
    uGetModuleFileName( nullptr, tmp );

    return StringFromDirPath( fs::u8path( tmp.c_str() ).parent_path() );
}

std::u8string get_profile_path()
{
    pfc::string8_fast tmp;
    // `get_profile_path` returns strings like "file://c:\documents_and_settings\username\blah\foobar2000",
    // so we need to convert them with `g_get_display_path`
    filesystem::g_get_display_path( core_api::get_profile_path(), tmp );

    return StringFromDirPath( fs::u8path( tmp.c_str() ) );
}

} // namespace smp
