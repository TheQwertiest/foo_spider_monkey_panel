#include <stdafx.h>

#include "panel_config.h"

#include <config/panel_config_binary.h>
#include <config/panel_config_com.h>
#include <config/panel_config_json.h>
#include <resources/resource.h>
#include <utils/guid_helpers.h>

#include <qwr/string_helpers.h>

namespace
{

enum class SettingsType : uint32_t
{
    Binary = 1,
    Json = 2
};

} // namespace

namespace smp::config
{

PanelProperties PanelProperties::FromJson( const qwr::u8string& jsonString )
{
    return smp::config::json::DeserializeProperties( jsonString );
}

qwr::u8string PanelProperties::ToJson() const
{
    return smp::config::json::SerializeProperties( *this );
}

PanelProperties PanelProperties::Load( stream_reader& reader, abort_callback& abort, SerializationFormat format )
{
    switch ( format )
    {
    case smp::config::SerializationFormat::Com:
        return smp::config::com::LoadProperties( reader, abort );
    case smp::config::SerializationFormat::Binary:
        return smp::config::binary::LoadProperties( reader, abort );
    case smp::config::SerializationFormat::Json:
        return smp::config::json::LoadProperties( reader, abort );
    default:
    {
        assert( false );
        throw qwr::QwrException( "Internal error: unknown serialization format" );
    }
    }
}

void PanelProperties::Save( stream_writer& writer, abort_callback& abort ) const
{
    smp::config::json::SaveProperties( writer, abort, *this );
}

qwr::u8string PanelSettings_InMemory::GetDefaultScript()
{
    puResource puRes = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( IDR_DEFAULT_SCRIPT ), "SCRIPT" );
    if ( puRes )
    {
        return qwr::u8string{ static_cast<const char*>( puRes->GetPointer() ), puRes->GetSize() };
    }
    else
    {
        return qwr::u8string{};
    }
}

PanelSettings::PanelSettings()
{
    ResetToDefault();
}

void PanelSettings::ResetToDefault()
{
    payload = PanelSettings_InMemory{};
    isPseudoTransparent = false;
    edgeStyle = EdgeStyle::NoEdge;
    id = [] {
        const auto guidStr = utils::GuidToStr( utils::GenerateGuid() );
        return qwr::unicode::ToU8( guidStr );
    }();
}

PanelSettings PanelSettings::Load( stream_reader& reader, size_t size, abort_callback& abort )
{
    try
    {
        PanelSettings panelSettings;

        if ( size < sizeof( SettingsType ) )
        { // probably no config at all
            return panelSettings;
        }

        uint32_t ver;
        reader.read_object_t( ver, abort );

        switch ( static_cast<SettingsType>( ver ) )
        {
        case SettingsType::Binary:
        {
            const PanelSettings binarySettings = smp::config::binary::LoadSettings( reader, abort );
            try
            { // check if we have json config appended
                return smp::config::json::LoadSettings( reader, abort );
            }
            catch ( const qwr::QwrException& )
            {
                return binarySettings;
            }
        }
        case SettingsType::Json:
            return smp::config::json::LoadSettings( reader, abort );
        default:
            throw qwr::QwrException( "Unexpected panel settings format: {}", ver );
        }
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void PanelSettings::Save( stream_writer& writer, abort_callback& abort ) const
{
    if ( std::holds_alternative<PanelSettings_InMemory>( payload ) )
    { // append json config for compatibility with older versions
        // TODO: remove in the future versions
        writer.write_object_t( static_cast<uint32_t>( SettingsType::Binary ), abort );
        smp::config::binary::SaveSettings( writer, abort, *this );
        smp::config::json::SaveSettings( writer, abort, *this );
    }
    else
    {
        writer.write_object_t( static_cast<uint32_t>( SettingsType::Json ), abort );
        smp::config::json::SaveSettings( writer, abort, *this );
    }
}

void PanelSettings::SaveDefault( stream_writer& writer, abort_callback& abort )
{
    PanelSettings{}.Save( writer, abort );
}

} // namespace smp::config
