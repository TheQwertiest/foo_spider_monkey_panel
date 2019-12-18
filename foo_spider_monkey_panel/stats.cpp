#include <stdafx.h>

#include "stats.h"

namespace
{

using namespace smp::stats;

constexpr char pinTo[] = "$lower(%artist% - %title%)";
constexpr t_filetimestamp retentionPeriod = system_time_periods::week * 4;

metadb_index_manager::ptr g_cachedAPI;

metadb_index_manager::ptr theAPI()
{
    auto ret = g_cachedAPI;
    if ( ret.is_empty() )
    {
        ret = metadb_index_manager::get();
    }
    return ret;
}

class metadb_index_client_impl : public metadb_index_client
{
public:
    metadb_index_hash transform( const file_info& info, const playable_location& location ) override
    {
        if ( titleFormat_.is_empty() )
        {
            titleformat_compiler::get()->compile_force( titleFormat_, pinTo );
        }

        pfc::string_formatter str;
        titleFormat_->run_simple( location, &info, str );
        return hasher_md5::get()->process_single_string( str ).xorHalve();
    }

private:
    titleformat_object::ptr titleFormat_;
};
metadb_index_client* g_client = new service_impl_single_t<metadb_index_client_impl>;

class init_stage_callback_impl : public init_stage_callback
{
public:
    void on_init_stage( t_uint32 stage ) override
    {
        if ( stage == init_stages::before_config_read )
        {
            auto api = theAPI();
            try
            {
                api->add( g_client, smp::guid::metadb_index, retentionPeriod );
            }
            catch ( std::exception const& e )
            {
                api->remove( smp::guid::metadb_index );
                FB2K_console_formatter() << SMP_NAME " stats: Critical initialization failure: " << e;
                return;
            }
            api->dispatch_global_refresh();
        }
    }
};
service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;

class initquit_impl : public initquit
{
public:
    void on_quit() override
    {
        g_cachedAPI.release();
    }
};
service_factory_single_t<initquit_impl> g_initquit_impl;

class metadb_display_field_provider_impl : public metadb_display_field_provider
{
public:
    t_uint32 get_field_count() override
    {
        return 5;
    }
    void get_field_name( t_uint32 index, pfc::string_base& out ) override
    {
        switch ( index )
        {
        case 0:
            out = "smp_playcount";
            break;
        case 1:
            out = "smp_loved";
            break;
        case 2:
            out = "smp_first_played";
            break;
        case 3:
            out = "smp_last_played";
            break;
        case 4:
            out = "smp_rating";
            break;
        default:
            assert( false );
            break;
        }
    }
    bool process_field( t_uint32 index, metadb_handle* handle, titleformat_text_out* out ) override
    {
        metadb_index_hash hash;
        if ( !g_client->hashHandle( handle, hash ) )
        {
            return false;
        }

        const fields tmp = get( hash );

        switch ( index )
        {
        case 0:
        {
            stats_t value = tmp.playcount;
            if ( !value )
            {
                return false;
            }
            out->write_int( titleformat_inputtypes::meta, value );
            return true;
        }
        case 1:
        {
            stats_t value = tmp.loved;
            if ( !value )
            {
                return false;
            }
            out->write_int( titleformat_inputtypes::meta, value );
            return true;
        }
        case 2:
        {
            const auto& value = tmp.first_played;
            if ( value.empty() )
            {
                return false;
            }
            out->write( titleformat_inputtypes::meta, value.c_str(), value.length() );
            return true;
        }
        case 3:
        {
            const auto& value = tmp.last_played;
            if ( value.empty() )
            {
                return false;
            }
            out->write( titleformat_inputtypes::meta, value.c_str(), value.length() );
            return true;
        }
        case 4:
        {
            stats_t value = tmp.rating;
            if ( !value )
            {
                return false;
            }
            out->write_int( titleformat_inputtypes::meta, value );
            return true;
        }
        default:
            return false;
        }
    }
};
service_factory_single_t<metadb_display_field_provider_impl> g_metadb_display_field_provider_impl;

class track_property_provider_impl : public track_property_provider_v2
{
public:
    void enumerate_properties( metadb_handle_list_cref p_tracks, track_property_callback& p_out ) override
    {
        const auto stlHandleList = smp::pfc_x::Make_Stl_CRef( p_tracks );
        if ( stlHandleList.size() == 1 )
        {
            metadb_index_hash hash;
            if ( g_client->hashHandle( stlHandleList.front(), hash ) )
            {
                fields tmp = get( hash );
                p_out.set_property( SMP_NAME, 0, "Playcount", std::to_string( tmp.playcount ).c_str() );
                p_out.set_property( SMP_NAME, 1, "Loved", std::to_string( tmp.loved ).c_str() );
                p_out.set_property( SMP_NAME, 2, "First Played", tmp.first_played.c_str() );
                p_out.set_property( SMP_NAME, 3, "Last Played", tmp.last_played.c_str() );
                p_out.set_property( SMP_NAME, 4, "Rating", std::to_string( tmp.rating ).c_str() );
            }
        }
        else
        {
            std::vector<metadb_index_hash> hashes;
            hashes.reserve( stlHandleList.size() );

            for ( const auto& handle: stlHandleList )
            {
                metadb_index_hash hash;
                if ( g_client->hashHandle( handle, hash ) )
                {
                    hashes.push_back( hash );
                }
            }

            const uint64_t total =
                ranges::accumulate( hashes, 0, []( auto sum, auto&& hash ) {
                    return sum + get( hash ).playcount;
                } );

            if ( total > 0 )
            {
                p_out.set_property( SMP_NAME, 0, "Playcount", pfc::format_uint( total ) );
            }
        }
    }

    void enumerate_properties_v2( metadb_handle_list_cref p_tracks, track_property_callback_v2& p_out ) override
    {
        if ( p_out.is_group_wanted( SMP_NAME ) )
        {
            enumerate_properties( p_tracks, p_out );
        }
    }

    bool is_our_tech_info( const char* ) override
    {
        return false;
    }
};
service_factory_single_t<track_property_provider_impl> g_track_property_provider_impl;

} // namespace

namespace smp::stats
{

bool hashHandle( metadb_handle_ptr const& pMetadb, metadb_index_hash& hash )
{
    return g_client->hashHandle( pMetadb, hash );
}

fields get( metadb_index_hash hash )
{
    mem_block_container_impl temp;
    theAPI()->get_user_data( smp::guid::metadb_index, hash, temp );

    if ( !temp.get_size() )
    {
        return fields();
    }

    fields ret;
    try
    {
        stream_reader_formatter_simple_ref<false> reader( temp.get_ptr(), temp.get_size() );

        reader >> ret.playcount;
        reader >> ret.loved;
        reader >> ret.first_played;
        reader >> ret.last_played;
        reader >> ret.rating;
    }
    catch ( const exception_io_data& )
    {
        return fields();
    }

    return ret;
}

void set( metadb_index_hash hash, fields f )
{
    stream_writer_formatter_simple<false> writer;
    writer << f.playcount;
    writer << f.loved;
    writer << f.first_played;
    writer << f.last_played;
    writer << f.rating;
    theAPI()->set_user_data( smp::guid::metadb_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
}

void refresh( const pfc::list_base_const_t<metadb_index_hash>& hashes )
{
    theAPI()->dispatch_refresh( smp::guid::metadb_index, hashes );
}

void refresh( const metadb_index_hash& hash )
{
    theAPI()->dispatch_refresh( smp::guid::metadb_index, hash );
}

} // namespace smp::stats
