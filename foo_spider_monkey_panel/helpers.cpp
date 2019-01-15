#include "stdafx.h"
#include "helpers.h"

#include <com_objects/script_interface_impl.h>
#include <utils/text_helpers.h>
#include <user_message.h>


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
