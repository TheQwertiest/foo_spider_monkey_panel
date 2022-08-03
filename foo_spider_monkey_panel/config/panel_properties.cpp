#include <stdafx.h>

#include "panel_properties.h"

#include <config/serializers/binary.h>
#include <config/serializers/com.h>
#include <config/serializers/json.h>

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

} // namespace smp::config
