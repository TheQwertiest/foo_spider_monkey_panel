#include <stdafx.h>

#include "panel_config.h"

#include <config/default_script.h>
#include <config/serializers/binary.h>
#include <config/serializers/com.h>
#include <config/serializers/json.h>
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

PanelConfig::PanelConfig()
{
    ResetToDefault();
}

void PanelConfig::ResetToDefault()
{
    properties = {};

    panelSettings = {};
    panelSettings.id = [] {
        const auto guidStr = smp::utils::GuidToStr( smp::utils::GenerateGuid() );
        return qwr::unicode::ToU8( guidStr );
    }();

    RawInMemoryScript source;
    source.isModule = true;
    source.script = GetDefaultScript();
    scriptSource = { std::move( source ) };
}

PanelConfig PanelConfig::Load( stream_reader& reader, size_t size, abort_callback& abort )
{
    try
    {
        PanelConfig panelSettings;

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
            const PanelConfig binarySettings = smp::config::binary::LoadSettings( reader, abort );
            try
            { // check if we have json config appended
                return smp::config::json::LoadConfig( reader, abort );
            }
            catch ( const qwr::QwrException& )
            {
                return binarySettings;
            }
        }
        case SettingsType::Json:
        {
            return smp::config::json::LoadConfig( reader, abort );
        }
        default:
        {
            throw qwr::QwrException( "Unexpected panel settings format: {}", ver );
        }
        }
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void PanelConfig::Save( stream_writer& writer, abort_callback& abort ) const
{
    writer.write_object_t( static_cast<uint32_t>( SettingsType::Json ), abort );
    smp::config::json::SaveConfig( writer, abort, *this );
}

void PanelConfig::SaveDefault( stream_writer& writer, abort_callback& abort )
{
    PanelConfig{}.Save( writer, abort );
}

} // namespace smp::config
