#include <stdafx.h>
#include "component_paths.h"

namespace smp
{

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

} // namespace smp
