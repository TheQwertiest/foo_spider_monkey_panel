#pragma once

#include <config.h>

namespace smp::config
{

bool LoadProperties_Binary( PanelProperties::config_map& data, stream_reader& reader, abort_callback& abort );
void SaveProperties_Binary( const PanelProperties::config_map& data, stream_writer& writer, abort_callback& abort );

bool LoadProperties_Com( PanelProperties::config_map& data, stream_reader& reader, abort_callback& abort );

} // namespace smp::config
