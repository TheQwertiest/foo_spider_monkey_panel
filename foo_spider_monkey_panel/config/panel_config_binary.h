#pragma once

#include <config/panel_config.h>

namespace smp::config::binary
{

/// @throw qwr::QwrException
[[nodiscard]] PanelSettings LoadSettings( stream_reader& reader, abort_callback& abort );

/// @throw qwr::QwrException
void SaveSettings( stream_writer& writer, abort_callback& abort, const PanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] PanelProperties LoadProperties( stream_reader& reader, abort_callback& abort );

/// @throw qwr::QwrException
void SaveProperties( stream_writer& writer, abort_callback& abort, const PanelProperties& properties );

} // namespace smp::config::binary
