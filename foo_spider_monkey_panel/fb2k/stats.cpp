#include <stdafx.h>

#include "stats.h"

#include <utils/logging.h>

namespace
{

constexpr char pinTo[] = "$lower(%artist% - %title%)";
constexpr t_filetimestamp retentionPeriod = system_time_periods::week * 4;

} // namespace

namespace
{

metadb_index_manager::ptr GetIndexManagerInstance();

class metadb_index_client_impl : public metadb_index_client
{
public:
    metadb_index_hash transform( const file_info& info, const playable_location& location ) override;

private:
    titleformat_object::ptr titleFormat_;
};

class init_stage_callback_impl : public init_stage_callback
{
public:
    void on_init_stage( t_uint32 stage ) override;
};

class initquit_impl : public initquit
{
public:
    void on_quit() override;
};

class metadb_display_field_provider_impl : public metadb_display_field_provider
{
public:
    t_uint32 get_field_count() override;
    void get_field_name( t_uint32 index, pfc::string_base& out ) override;
    bool process_field( t_uint32 index, metadb_handle* handle, titleformat_text_out* out ) override;
};

class track_property_provider_impl : public track_property_provider_v2
{
public:
    void enumerate_properties( metadb_handle_list_cref p_tracks, track_property_callback& p_out ) override;

    void enumerate_properties_v2( metadb_handle_list_cref p_tracks, track_property_callback_v2& p_out ) override;

    bool is_our_tech_info( const char* ) override;
};

} // namespace

namespace
{

metadb_index_client* g_client = new service_impl_single_t<metadb_index_client_impl>;

}

namespace
{

using namespace smp::stats;

metadb_index_manager::ptr GetIndexManagerInstance()
{
    static metadb_index_manager::ptr mimp = metadb_index_manager::get();
    assert( !mimp.is_empty() );
    return mimp;
}

metadb_index_hash metadb_index_client_impl::transform( const file_info& info, const playable_location& location )
{
    if ( titleFormat_.is_empty() )
    {
        titleformat_compiler::get()->compile_force( titleFormat_, pinTo );
    }

    pfc::string_formatter str;
    titleFormat_->run_simple( location, &info, str );
    return hasher_md5::get()->process_single_string( str ).xorHalve();
}

void init_stage_callback_impl::on_init_stage( t_uint32 stage )
{
    if ( stage == init_stages::before_config_read )
    {
        auto api = GetIndexManagerInstance();
        try
        {
            api->add( g_client, smp::guid::metadb_index, retentionPeriod );
        }
        catch ( const std::exception& e )
        {
            api->remove( smp::guid::metadb_index );
            smp::utils::LogError( fmt::format(
                "Stats initialization failed:\n"
                "  {}",
                e.what() ) );
            return;
        }
        api->dispatch_global_refresh();
    }
}

void initquit_impl::on_quit()
{
    GetIndexManagerInstance().release();
}

t_uint32 metadb_display_field_provider_impl::get_field_count()
{
    return 5;
}

void metadb_display_field_provider_impl::get_field_name( t_uint32 index, pfc::string_base& out )
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

bool metadb_display_field_provider_impl::process_field( t_uint32 index, metadb_handle* handle, titleformat_text_out* out )
{
    metadb_index_hash hash;
    if ( !g_client->hashHandle( handle, hash ) )
    {
        return false;
    }

    const auto tmp = GetStats( hash );
    switch ( index )
    {
    case 0:
    {
        auto value = tmp.playcount;
        if ( !value )
        {
            return false;
        }
        out->write_int( titleformat_inputtypes::meta, value );
        return true;
    }
    case 1:
    {
        auto value = tmp.loved;
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
        auto value = tmp.rating;
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

void track_property_provider_impl::enumerate_properties( metadb_handle_list_cref p_tracks, track_property_callback& p_out )
{
    const auto stlHandleList = qwr::pfc_x::Make_Stl_CRef( p_tracks );
    if ( stlHandleList.size() == 1 )
    {
        metadb_index_hash hash;
        if ( g_client->hashHandle( stlHandleList.front(), hash ) )
        {
            Fields tmp = GetStats( hash );
            if ( tmp.playcount > 0 )
            {
                p_out.set_property( SMP_NAME, 0, "Playcount", std::to_string( tmp.playcount ).c_str() );
            }
            if ( tmp.loved > 0 )
            {
                p_out.set_property( SMP_NAME, 1, "Loved", std::to_string( tmp.loved ).c_str() );
            }
            if ( !tmp.first_played.empty() )
            {
                p_out.set_property( SMP_NAME, 2, "First Played", tmp.first_played.c_str() );
            }
            if ( !tmp.last_played.empty() )
            {
                p_out.set_property( SMP_NAME, 3, "Last Played", tmp.last_played.c_str() );
            }
            if ( tmp.rating > 0 )
            {
                p_out.set_property( SMP_NAME, 4, "Rating", std::to_string( tmp.rating ).c_str() );
            }
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
                hashes.emplace_back( hash );
            }
        }

        const auto total =
            ranges::accumulate( hashes, 0, []( auto sum, auto&& hash ) {
                return sum + GetStats( hash ).playcount;
            } );

        if ( total )
        {
            p_out.set_property( SMP_NAME, 0, "Playcount", pfc::format_uint( total ) );
        }
    }
}

void track_property_provider_impl::enumerate_properties_v2( metadb_handle_list_cref p_tracks, track_property_callback_v2& p_out )
{
    if ( p_out.is_group_wanted( SMP_NAME ) )
    {
        enumerate_properties( p_tracks, p_out );
    }
}

bool track_property_provider_impl::is_our_tech_info( const char* )
{
    return false;
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( init_stage_callback_impl );
FB2K_SERVICE_FACTORY( initquit_impl );
FB2K_SERVICE_FACTORY( metadb_display_field_provider_impl );
FB2K_SERVICE_FACTORY( track_property_provider_impl );

} // namespace

namespace smp::stats
{

bool HashHandle( metadb_handle_ptr const& pMetadb, metadb_index_hash& hash )
{
    return g_client->hashHandle( pMetadb, hash );
}

Fields GetStats( metadb_index_hash hash )
{
    mem_block_container_impl temp;
    GetIndexManagerInstance()->get_user_data( smp::guid::metadb_index, hash, temp );

    if ( !temp.get_size() )
    {
        return {};
    }

    try
    {
        stream_reader_formatter_simple_ref<false> reader( temp.get_ptr(), temp.get_size() );

        Fields ret;
        reader >> ret.playcount;
        reader >> ret.loved;
        reader >> ret.first_played;
        reader >> ret.last_played;
        reader >> ret.rating;
        return ret;
    }
    catch ( const exception_io_data& )
    {
        return {};
    }
}

void SetStats( metadb_index_hash hash, const Fields& f )
{
    stream_writer_formatter_simple<false> writer;
    writer << f.playcount;
    writer << f.loved;
    writer << f.first_played;
    writer << f.last_played;
    writer << f.rating;
    GetIndexManagerInstance()->set_user_data( smp::guid::metadb_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
}

void RefreshStats( const pfc::list_base_const_t<metadb_index_hash>& hashes )
{
    GetIndexManagerInstance()->dispatch_refresh( smp::guid::metadb_index, hashes );
}

void RefreshStats( const metadb_index_hash& hash )
{
    GetIndexManagerInstance()->dispatch_refresh( smp::guid::metadb_index, hash );
}

} // namespace smp::stats
