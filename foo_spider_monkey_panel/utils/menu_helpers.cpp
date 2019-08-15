#include <stdafx.h>
#include "menu_helpers.h"

#include <unordered_map>
#include <optional>

namespace std
{
template <>
struct hash<GUID>
{
    size_t operator()( const GUID& guid ) const noexcept
    {
        const uint64_t guid64_1 =
            ( static_cast<uint64_t>( guid.Data1 ) << 32 )
            | ( static_cast<uint32_t>( guid.Data2 ) << 16 )
            | guid.Data3;
        uint64_t guid64_2;
        memcpy( &guid64_2, guid.Data4, sizeof( guid.Data4 ) );

        std::hash<std::uint64_t> hash;
        return hash( guid64_1 ) ^ hash( guid64_2 );
    }
};
} // namespace std

namespace
{

using GuidMenuMap = std::unordered_map<GUID, mainmenu_group::ptr>;

bool match_menu_command( const std::u8string& path, const std::u8string& command )
{
    const auto commandLen = command.length();
    const auto pathLen = path.length();

    if ( commandLen > pathLen )
    {
        return false;
    }

    if ( commandLen == pathLen 
         && !_stricmp( command.c_str(), path.c_str() ) )
    {
        return true;
    }

    return ( ( path[pathLen - commandLen - 1] == '/' ) && !_stricmp( path.c_str() + pathLen - commandLen, command.c_str() ) );
}

std::optional<contextmenu_node*> find_context_command_recur( const std::u8string& p_command, std::u8string& basePath, contextmenu_node* p_parent )
{
    assert( p_parent && p_parent->get_type() == contextmenu_item_node::TYPE_POPUP );
     
    for ( size_t child_id = 0; child_id < p_parent->get_num_children(); ++child_id )
    {
        contextmenu_node* child = p_parent->get_child( child_id );
        if ( !child )
        {
            continue;
        }

        std::u8string curPath = basePath;
        curPath += child->get_name();

        switch ( child->get_type() )
        {
        case contextmenu_item_node::TYPE_POPUP:
        {
            curPath += '/';

            if ( auto retVal = find_context_command_recur( p_command, curPath, child );
                 retVal )
            {
                return retVal;
            }

            break;
        }
        case contextmenu_item_node::TYPE_COMMAND:
        {
            if ( match_menu_command( curPath, p_command ) )
            {
                return child;
            }
            break;
        }
        }
    }

    return std::nullopt;
}

/// @throw pfc::exception
bool execute_context_command_by_name_unsafe( const std::u8string& name, const metadb_handle_list& p_handles, unsigned flags )
{
    contextmenu_manager::ptr cm;
    contextmenu_manager::g_create( cm );

    if ( p_handles.get_count() )
    {
        cm->init_context( p_handles, flags );
    }
    else
    {
        cm->init_context_now_playing( flags );
    }

    auto pRoot = cm->get_root();
    if ( !pRoot || pRoot->get_type() != contextmenu_item_node::TYPE_POPUP )
    {
        return false;
    }

    std::u8string emptyPath;
    if ( auto retVal = find_context_command_recur( name, emptyPath, pRoot );
         retVal && retVal.value() )
    {
        retVal.value()->execute();
        return true;
    }

    return false;
}

GuidMenuMap GenerateGuidMainmenuMap()
{
    GuidMenuMap guidMap;
    for ( service_enum_t<mainmenu_group> e; !e.finished(); ++e )
    {
        auto mmg = e.get();
        guidMap[mmg->get_guid()] = mmg;
    }

    return guidMap;
}

std::u8string generate_mainmenu_command_path( const GuidMenuMap& group_guid_map, const service_ptr_t<mainmenu_commands> ptr )
{
    std::u8string path;

    GUID group_guid = ptr->get_parent();
    while ( group_guid_map.count( group_guid ) )
    {
        auto group_ptr = group_guid_map.at( group_guid );

        if ( mainmenu_group_popup::ptr group_popup_ptr;
             group_ptr->service_query_t( group_popup_ptr ) )
        {
            pfc::string8_fast displayName;
            group_popup_ptr->get_display_string( displayName );

            if ( !displayName.is_empty() )
            {
                path = fmt::format( "{}/{}", displayName.c_str(), path );
            }
        }

        group_guid = group_ptr->get_parent();
    }

    return path;
}

std::optional<mainmenu_node::ptr> find_mainmenu_command_v2_node_recur( mainmenu_node::ptr node, const std::u8string& basePath, const std::u8string& name )
{
    assert( node.is_valid() );

    std::u8string curPath = basePath;

    if ( mainmenu_node::type_separator == node->get_type() )
    {
        return std::nullopt;
    }

    pfc::string8_fast displayName;
    uint32_t tmp;
    node->get_display( displayName, tmp );
    if ( !displayName.is_empty() )
    {
        curPath += displayName.c_str();
    }

    switch ( node->get_type() )
    {
    case mainmenu_node::type_command:
    {
        if ( match_menu_command( curPath, name ) )
        {
            return node;
        }
        break;
    }
    case mainmenu_node::type_group:
    {
        if ( curPath[curPath.length() - 1] != '/' )
        {
            curPath += '/';
        }

        for ( auto i: ranges::view::indices( node->get_children_count() ) )
        {
            mainmenu_node::ptr child = node->get_child( i );
            if ( auto retVal = find_mainmenu_command_v2_node_recur( child, curPath, name );
                 retVal )
            {
                return retVal;
            }
        }
        break;
    }
    default:
    {
        assert( 0 );
    }
    }

    return std::nullopt;
}

/// @throw pfc::exception
template <typename F_New, typename F_Old>
bool ApplyFnOnMainmenuNode( const std::u8string& name, F_New fnNew, F_Old fnOld )
{
    const GuidMenuMap group_guid_text_map = GenerateGuidMainmenuMap();

    for ( service_enum_t<mainmenu_commands> e; !e.finished(); ++e )
    {
        auto mmc = e.get();

        for ( auto idx: ranges::view::indices( mmc->get_command_count() ) )
        {
            std::u8string path = generate_mainmenu_command_path( group_guid_text_map, mmc );

            if ( mainmenu_commands_v2::ptr mmc_v2;
                 mmc->service_query_t( mmc_v2 ) && mmc_v2->is_command_dynamic( idx ) )
            { // new fb2k v1.0 commands
                mainmenu_node::ptr node = mmc_v2->dynamic_instantiate( idx );

                if ( auto retVal = find_mainmenu_command_v2_node_recur( node, path, name );
                     retVal && retVal->is_valid() )
                {
                    fnNew( retVal.value() );
                    return true;
                }

                continue;
            }

            // old commands
            pfc::string8_fast command;
            mmc->get_name( idx, command );
            path += command;

            if ( match_menu_command( path, name ) )
            {
                fnOld( idx, mmc );
                return true;
            }
        }
    }

    return false;
}

} // namespace

namespace smp::utils
{

bool execute_context_command_by_name( const std::u8string& name, const metadb_handle_list& p_handles, unsigned flags )
{
    try
    {
        return execute_context_command_by_name_unsafe( name, p_handles, flags );
    }
    catch ( const pfc::exception& )
    {
        return false;
    }
}
bool execute_mainmenu_command_by_name( const std::u8string& name )
{
    try
    {
        return ApplyFnOnMainmenuNode( name,
                                      []( auto node ) { node->execute( nullptr ); },
                                      []( auto idx, auto ptr ) { ptr->execute( idx, nullptr ); } );
    }
    catch ( const pfc::exception& )
    {
        return false;
    }
}
void get_mainmenu_command_status_by_name( const std::u8string& name, uint32_t& status )
{
    try
    {
        bool bRet = ApplyFnOnMainmenuNode(
            name,
            [&status]( auto node ) {
                pfc::string8_fast tmp;
                node->get_display( tmp, status );
            },
            [&status]( auto idx, auto ptr ) {
                pfc::string8_fast tmp;
                ptr->get_display( idx, tmp, status );
            } );

        SmpException::ExpectTrue( bRet, "Unknown menu command" );
    }
    catch ( const pfc::exception& e )
    {
        throw SmpException( e.what() );
    }
}

} // namespace smp::utils
