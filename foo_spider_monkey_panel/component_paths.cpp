#include <stdafx.h>
#include "component_paths.h"

namespace smp
{

std::u8string get_fb2k_component_path()
{
    pfc::string8_fast path;
    uGetModuleFileName( core_api::get_my_instance(), path );
    path = pfc::string_directory( path );
    path.fix_dir_separator( '\\' );

    return path.c_str();
}

std::u8string get_fb2k_path()
{
    pfc::string8_fast path;
    uGetModuleFileName( nullptr, path );
    path = pfc::string_directory( path );
    path.fix_dir_separator( '\\' );

    return path.c_str();
}

std::u8string get_profile_path()
{
    pfc::string8_fast path = file_path_display( core_api::get_profile_path() ).get_ptr();
    path.fix_dir_separator( '\\' );

    return path.c_str();
}

} // namespace smp
