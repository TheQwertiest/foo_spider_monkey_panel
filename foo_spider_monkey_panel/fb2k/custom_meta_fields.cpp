#include <stdafx.h>

#include "custom_meta_fields.h"

#include <utils/logging.h>

#include <qwr/algorithm.h>
#include <qwr/visitor.h>

using namespace smp::custom_meta;

namespace
{

constexpr const char kPinQuery[] = "$lower(%artist% - %title%)";
constexpr const t_filetimestamp kRetentionPeriod = system_time_periods::week * 4;
const std::unordered_set<qwr::u8string> kBuiltinField{
    "smp_playcount",
    "smp_loved",
    "smp_first_played",
    "smp_last_played",
    "smp_rating",
};

std::vector<FieldInfo> gRegisteredFields{
    { "smp_playcount", "Playcount", FieldValueType::kUInt32, true },
    { "smp_loved", "Loved", FieldValueType::kUInt32 },
    { "smp_first_played", "First Played", FieldValueType::kString },
    { "smp_last_played", "Last Played", FieldValueType::kString },
    { "smp_rating", "Rating", FieldValueType::kUInt32 },
};

} // namespace

namespace
{

class MetadbIndexClientImpl : public metadb_index_client
{
public:
    static metadb_index_client* Get();

    metadb_index_hash transform( const file_info& info, const playable_location& location ) override;

private:
    titleformat_object::ptr titleFormat_;
};

class InitStageCallbackImpl : public init_stage_callback
{
public:
    void on_init_stage( t_uint32 stage ) override;
};

class InitQuitImpl : public initquit
{
public:
    void on_quit() override;
};

class MetadbDisplayFieldProviderImpl : public metadb_display_field_provider
{
public:
    t_uint32 get_field_count() override;
    void get_field_name( t_uint32 index, pfc::string_base& out ) override;
    bool process_field( t_uint32 index, metadb_handle* handle, titleformat_text_out* out ) override;
};

class TrackPropertyProviderImpl : public track_property_provider_v2
{
public:
    void enumerate_properties( metadb_handle_list_cref p_tracks, track_property_callback& p_out ) override;
    void enumerate_properties_v2( metadb_handle_list_cref p_tracks, track_property_callback_v2& p_out ) override;
    bool is_our_tech_info( const char* p_name ) override;
};

} // namespace

namespace
{
metadb_index_manager_v2::ptr GetIndexManager()
{
    static metadb_index_manager_v2::ptr mimp = metadb_index_manager_v2::get();
    assert( !mimp.is_empty() );
    return mimp;
}

void FinalizeIndexManager()
{
    GetIndexManager().release();
}

std::optional<metadb_index_hash> HashHandle( metadb_handle_ptr handle )
{
    metadb_index_hash hash;
    if ( !MetadbIndexClientImpl::Get()->hashHandle( handle, hash ) )
    {
        return std::nullopt;
    }
    return hash;
}

metadb_index_client* MetadbIndexClientImpl::Get()
{
    static metadb_index_client* pClient = new service_impl_single_t<MetadbIndexClientImpl>;
    return pClient;
}

metadb_index_hash MetadbIndexClientImpl::transform( const file_info& info, const playable_location& location )
{
    if ( titleFormat_.is_empty() )
    {
        titleformat_compiler::get()->compile_force( titleFormat_, kPinQuery );
    }

    pfc::string_formatter str;
    titleFormat_->run_simple( location, &info, str );
    return hasher_md5::get()->process_single_string( str ).xorHalve();
}

void InitStageCallbackImpl::on_init_stage( t_uint32 stage )
{
    if ( stage == init_stages::before_config_read )
    {
        auto api = GetIndexManager();
        try
        {
            api->add( MetadbIndexClientImpl::Get(), smp::guid::metadb_index, kRetentionPeriod );
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

void InitQuitImpl::on_quit()
{
    FinalizeIndexManager();
}

t_uint32 MetadbDisplayFieldProviderImpl::get_field_count()
{
    return gRegisteredFields.size();
}

void MetadbDisplayFieldProviderImpl::get_field_name( t_uint32 index, pfc::string_base& out )
{
    if ( index < gRegisteredFields.size() )
    {
        out = gRegisteredFields[index].metaName.c_str();
    }
}

bool MetadbDisplayFieldProviderImpl::process_field( t_uint32 index, metadb_handle* handle, titleformat_text_out* out )
{
    if ( index >= gRegisteredFields.size() )
    {
        return false;
    }
    const auto& fieldInfo = gRegisteredFields[index];

    const auto fields = GetData( handle );
    const auto pFieldValue = qwr::FindAsPointer( fields, fieldInfo.metaName );
    if ( !pFieldValue )
    {
        return false;
    }
    const auto& value = *pFieldValue;

    switch ( fieldInfo.valueType )
    {
    case FieldValueType::kUInt32:
    {
        const auto pRawValue = std::get_if<uint32_t>( &value );
        if ( !pRawValue || !*pRawValue )
        {
            return false;
        }

        out->write_int( titleformat_inputtypes::meta, *pRawValue );
        return true;
    }
    case FieldValueType::kString:
    {
        const auto pRawValue = std::get_if<qwr::u8string>( &value );
        if ( !pRawValue || pRawValue->empty() )
        {
            return false;
        }

        out->write( titleformat_inputtypes::meta, pRawValue->c_str(), pRawValue->length() );
        return true;
    }
    default:
        assert( false );
        return false;
    }
}

void TrackPropertyProviderImpl::enumerate_properties( metadb_handle_list_cref p_tracks, track_property_callback& p_out )
{
    const auto isSingle = ( p_tracks.size() == 1 );

    // group and filter field values
    std::unordered_map<size_t, std::vector<FieldValue>> idxToValues;
    for ( const auto& handle: p_tracks )
    {
        FieldValueMap fields = GetData( handle );
        for ( const auto& [name, value]: fields )
        {
            const auto enumStatsView = gRegisteredFields | ranges::views::enumerate;
            const auto it = ranges::find_if( enumStatsView, [&]( const auto& elem ) { return elem.second.metaName == name; } );
            if ( it == enumStatsView.end() )
            {
                continue;
            }

            const auto& [i, fieldInfo] = *it;
            if ( !isSingle && !fieldInfo.isSummable )
            {
                continue;
            }

            switch ( fieldInfo.valueType )
            {
            case FieldValueType::kUInt32:
            {
                const auto pRawValue = std::get_if<uint32_t>( &value );
                if ( pRawValue && *pRawValue )
                {
                    idxToValues[i].push_back( *pRawValue );
                }
                break;
            }
            case FieldValueType::kString:
            {
                const auto pRawValue = std::get_if<qwr::u8string>( &value );
                if ( pRawValue && !pRawValue->empty() )
                {
                    idxToValues[i].push_back( *pRawValue );
                }
                break;
            }
            default:
                assert( false );
            }
        }
    }

    // generate final field values
    for ( const auto& [i, values]: idxToValues )
    {
        FieldValue ret;
        bool isInitialized = false;
        for ( const auto& value: values )
        {
            std::visit(
                qwr::Visitor{
                    [&]( uint32_t arg ) {
                        if ( !isInitialized )
                        {
                            ret.emplace<uint32_t>();
                            isInitialized = true;
                        }
                        std::get<uint32_t>( ret ) += arg;
                    },
                    [&]( const qwr::u8string& arg ) {
                        if ( !isInitialized )
                        {
                            ret.emplace<qwr::u8string>();
                            isInitialized = true;
                        }
                        else
                        {
                            std::get<qwr::u8string>( ret ) += "; ";
                        }
                        std::get<qwr::u8string>( ret ) += arg;
                    } },
                value );
        }

        const auto& fieldInfo = gRegisteredFields[i];
        std::visit(
            qwr::Visitor{
                [&]( uint32_t arg ) { p_out.set_property( SMP_NAME, i, fieldInfo.displayedName.c_str(), std::to_string( arg ).c_str() ); },
                [&]( const qwr::u8string& arg ) { p_out.set_property( SMP_NAME, i, fieldInfo.displayedName.c_str(), arg.c_str() ); } },
            ret );
    }
}

void TrackPropertyProviderImpl::enumerate_properties_v2( metadb_handle_list_cref p_tracks, track_property_callback_v2& p_out )
{
    if ( p_out.is_group_wanted( SMP_NAME ) )
    {
        enumerate_properties( p_tracks, p_out );
    }
}

bool TrackPropertyProviderImpl::is_our_tech_info( const char* /*p_name*/ )
{
    return false;
}

auto SetDataImpl( metadb_index_hash hash, const FieldValueMap& fields )
{
    stream_writer_formatter_simple<false> writer;
    for ( auto i: ranges::views::indices( kBuiltinField.size() ) )
    {
        const auto fieldInfo = gRegisteredFields[i];
        switch ( fieldInfo.valueType )
        {
        case FieldValueType::kUInt32:
        {
            uint32_t tmp = 0;
            if ( const auto pFieldValue = qwr::FindAsPointer( fields, fieldInfo.metaName ) )
            {
                auto pRawValue = std::get_if<uint32_t>( pFieldValue );
                if ( pRawValue )
                {
                    tmp = *pRawValue;
                }
            }
            writer << tmp;
            break;
        }
        case FieldValueType::kString:
        {
            qwr::u8string tmp;
            if ( const auto pFieldValue = qwr::FindAsPointer( fields, fieldInfo.metaName ) )
            {
                auto pRawValue = std::get_if<qwr::u8string>( pFieldValue );
                if ( pRawValue )
                {
                    tmp = *pRawValue;
                }
            }
            writer << tmp;
            break;
        }
        default:
            assert( false );
        }
    }

    // TODO: handle custom user fields
    return writer;
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( InitStageCallbackImpl );
FB2K_SERVICE_FACTORY( InitQuitImpl );
FB2K_SERVICE_FACTORY( MetadbDisplayFieldProviderImpl );
FB2K_SERVICE_FACTORY( TrackPropertyProviderImpl );

} // namespace

namespace smp::custom_meta
{

std::optional<FieldInfo> GetFieldInfo( const qwr::u8string& fieldName )
{
    const auto it = ranges::find_if( gRegisteredFields, [&]( const auto& elem ) { return elem.metaName == fieldName; } );
    if ( it == gRegisteredFields.end() )
    {
        return std::nullopt;
    }
    return *it;
}

FieldValueMap GetData( metadb_handle_ptr handle )
{
    const auto hashOpt = ::HashHandle( handle );
    if ( !hashOpt )
    {
        return {};
    }

    mem_block_container_impl readerBuffer;
    GetIndexManager()->get_user_data( smp::guid::metadb_index, *hashOpt, readerBuffer );

    if ( !readerBuffer.get_size() )
    {
        return {};
    }

    try
    {
        stream_reader_formatter_simple_ref<false> reader( readerBuffer.get_ptr(), readerBuffer.get_size() );

        FieldValueMap ret;
        qwr::u8string userData;
        for ( auto i: ranges::views::indices( kBuiltinField.size() ) )
        {
            const auto fieldInfo = gRegisteredFields[i];
            switch ( fieldInfo.valueType )
            {
            case FieldValueType::kUInt32:
            {
                uint32_t tmp;
                reader >> tmp;
                ret.try_emplace( fieldInfo.metaName, tmp );
                break;
            }
            case FieldValueType::kString:
            {
                qwr::u8string tmp;
                reader >> tmp;
                ret.try_emplace( fieldInfo.metaName, tmp );
                break;
            }
            default:
                assert( false );
            }
        }

        // TODO: handle custom user fields

        return ret;
    }
    catch ( const exception_io_data& /*e*/ )
    {
        return {};
    }
}

void SetData( metadb_handle_ptr handle, const FieldValueMap& fields )
{
    const auto hashOpt = ::HashHandle( handle );
    if ( !hashOpt )
    {
        return;
    }

    auto writer = SetDataImpl( *hashOpt, fields );
    GetIndexManager()->set_user_data( smp::guid::metadb_index, *hashOpt, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
}

void SetData( const metadb_handle_list& handles, const FieldValueMap& fields )
{
    auto pTransaction = GetIndexManager()->begin_transaction();
    for ( const auto& handle: handles )
    {
        const auto hashOpt = ::HashHandle( handle );
        if ( !hashOpt )
        {
            continue;
        }

        auto writer = SetDataImpl( *hashOpt, fields );
        pTransaction->set_user_data( smp::guid::metadb_index, *hashOpt, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
    }
    pTransaction->commit();
}

void RefreshData( const metadb_handle_list& handles )
{
    pfc::list_t<metadb_index_hash> hashes;
    for ( const auto& handle: handles )
    {
        if ( const auto hashOpt = ::HashHandle( handle ) )
        {
            hashes.add_item( *hashOpt );
        }
    }

    GetIndexManager()->dispatch_refresh( smp::guid::metadb_index, hashes );
}

void RefreshData( metadb_handle_ptr handle )
{
    if ( const auto hashOpt = ::HashHandle( handle ) )
    {
        GetIndexManager()->dispatch_refresh( smp::guid::metadb_index, *hashOpt );
    }
}

} // namespace smp::custom_meta
