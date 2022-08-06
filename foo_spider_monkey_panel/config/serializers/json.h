#pragma once

#include <config/panel_config.h>

namespace smp::config::json
{

/// @throw qwr::QwrException
[[nodiscard]] PanelConfig LoadConfig( stream_reader& reader, abort_callback& abort );

/// @throw qwr::QwrException
void SaveConfig( stream_writer& writer, abort_callback& abort, const PanelConfig& settings );

/// @throw qwr::QwrException
[[nodiscard]] PanelProperties LoadProperties( stream_reader& reader, abort_callback& abort );

/// @throw qwr::QwrException
void SaveProperties( stream_writer& writer, abort_callback& abort, const PanelProperties& properties );

/// @throw qwr::QwrException
[[nodiscard]] PanelProperties DeserializeProperties( const qwr::u8string& str );

/// @throw qwr::QwrException
[[nodiscard]] qwr::u8string SerializeProperties( const PanelProperties& properties );

} // namespace smp::config::json
