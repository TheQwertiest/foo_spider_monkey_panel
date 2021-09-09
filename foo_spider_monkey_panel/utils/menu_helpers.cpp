#include <stdafx.h>

#include "menu_helpers.h"

#include <utils/guid_helpers.h>

#include <optional>
#include <unordered_map>

namespace
{

using GuidMenuMap = std::unordered_map<GUID, mainmenu_group::ptr, smp::utils::GuidHasher>;

bool DoesPathMatchCommand( const qwr::u8string& path, const qwr::u8string& command )
{
    const auto commandLen = command.length();
    const auto pathLen = path.length();

    if ( commandLen > pathLen )
    {
        return false;
    }

    if ( commandLen == pathLen )
    {
        return !_stricmp( command.c_str(), path.c_str() );
    }

    return ( ( path[pathLen - commandLen - 1] == '/' ) && !_stricmp( path.c_str() + pathLen - commandLen, command.c_str() ) );
}

contextmenu_node* FindContextCommandRecur( const qwr::u8string& p_command, qwr::u8string& basePath, contextmenu_node* p_parent )
{
    assert( p_parent && p_parent->get_type() == contextmenu_item_node::TYPE_POPUP );

    for ( const auto idx: ranges::views::indices( p_parent->get_num_children() ) )
    {
        auto* pChild = p_parent->get_child( idx );
        if ( !pChild )
        {
            continue;
        }

        auto curPath = basePath;
        curPath += pChild->get_name();

        switch ( pChild->get_type() )
        {
        case contextmenu_item_node::TYPE_POPUP:
        {
            curPath += '/';

            if ( auto retVal = FindContextCommandRecur( p_command, curPath, pChild );
                 retVal )
            {
                return retVal;
            }

            break;
        }
        case contextmenu_item_node::TYPE_COMMAND:
        {
            if ( DoesPathMatchCommand( curPath, p_command ) )
            {
                return pChild;
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }

    return nullptr;
}

/// @throw pfc::exception
bool ExecuteContextCommandByNameUnsafe( const qwr::u8string& name, const metadb_handle_list& handles, uint32_t flags )
{
    contextmenu_manager::ptr cm;
    contextmenu_manager::g_create( cm );

    if ( handles.get_count() )
    {
        cm->init_context( handles, flags );
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

    qwr::u8string emptyPath;
    if ( auto retVal = FindContextCommandRecur( name, emptyPath, pRoot );
         retVal )
    {
        retVal->execute();
        return true;
    }

    return false;
}

GuidMenuMap GenerateGuidMainmenuMap()
{
    GuidMenuMap guidMap;
    for ( service_enum_t<mainmenu_group> e; !e.finished(); ++e )
    {
        const auto mmg = e.get();
        guidMap.try_emplace( mmg->get_guid(), mmg );
    }

    return guidMap;
}

qwr::u8string GenerateMainmenuCommandPath( const GuidMenuMap& group_guid_map, const service_ptr_t<mainmenu_commands>& ptr )
{
    qwr::u8string path;

    auto groupGuid = ptr->get_parent();
    while ( group_guid_map.contains( groupGuid ) )
    {
        const auto& pGroup = group_guid_map.at( groupGuid );

        if ( mainmenu_group_popup::ptr pGroupPopup;
             pGroup->service_query_t( pGroupPopup ) )
        {
            pfc::string8_fast displayName;
            pGroupPopup->get_display_string( displayName );

            if ( !displayName.is_empty() )
            {
                path = fmt::format( "{}/{}", displayName, path );
            }
        }

        groupGuid = pGroup->get_parent();
    }

    return path;
}

mainmenu_node::ptr FindMainmenuCommandV2NodeRecur( mainmenu_node::ptr node, const qwr::u8string& basePath, const qwr::u8string& name )
{
    assert( node.is_valid() );

    if ( mainmenu_node::type_separator == node->get_type() )
    {
        return {};
    }

    auto curPath = basePath;

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
        if ( DoesPathMatchCommand( curPath, name ) )
        {
            return node;
        }
        break;
    }
    case mainmenu_node::type_group:
    {
        if ( curPath.back() != '/' )
        {
            curPath += '/';
        }

        for ( auto i: ranges::views::indices( node->get_children_count() ) )
        {
            auto pChild = node->get_child( i );
            if ( auto retVal = FindMainmenuCommandV2NodeRecur( pChild, curPath, name );
                 retVal.is_valid() )
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

    return {};
}

/// @throw pfc::exception
template <typename F_New, typename F_Old>
bool ApplyFnOnMainmenuNode( const qwr::u8string& name, F_New fnNew, F_Old fnOld )
{
    const auto group_guid_text_map = GenerateGuidMainmenuMap();

    for ( service_enum_t<mainmenu_commands> e; !e.finished(); ++e )
    {
        auto mmc = e.get();

        for ( const auto idx: ranges::views::indices( mmc->get_command_count() ) )
        {
            auto path = GenerateMainmenuCommandPath( group_guid_text_map, mmc );

            if ( mainmenu_commands_v2::ptr mmc_v2;
                 mmc->service_query_t( mmc_v2 ) && mmc_v2->is_command_dynamic( idx ) )
            { // new fb2k v1.0 commands
                auto node = mmc_v2->dynamic_instantiate( idx );

                if ( auto retVal = FindMainmenuCommandV2NodeRecur( node, path, name );
                     retVal.is_valid() )
                {
                    fnNew( retVal );
                    return true;
                }

                continue;
            }

            // old commands
            pfc::string8_fast command;
            mmc->get_name( idx, command );
            path += command;

            if ( DoesPathMatchCommand( path, name ) )
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

void ExecuteContextCommandByName( const qwr::u8string& name, const metadb_handle_list& handles, uint32_t flags )
{
    try
    {
        bool bRet = ExecuteContextCommandByNameUnsafe( name, handles, flags );
        qwr::QwrException::ExpectTrue( bRet, "Unknown menu command: {}", name );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void ExecuteMainmenuCommandByName( const qwr::u8string& name )
{
    try
    {
        bool bRet = ApplyFnOnMainmenuNode(
            name,
            []( auto node ) { node->execute( nullptr ); },
            []( auto idx, auto ptr ) { ptr->execute( idx, nullptr ); } );
        qwr::QwrException::ExpectTrue( bRet, "Unknown menu command: {}", name );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

uint32_t GetMainmenuCommandStatusByName( const qwr::u8string& name )
{
    try
    {
        uint32_t status;
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
        qwr::QwrException::ExpectTrue( bRet, "Unknown menu command: {}", name );

        return status;
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

} // namespace smp::utils
