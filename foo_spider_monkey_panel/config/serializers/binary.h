#pragma once

#include <config/panel_config.h>

namespace smp::config::binary
{

/// @throw qwr::QwrException
[[nodiscard]] PanelConfig LoadSettings( stream_reader& reader, abort_callback& abort );

/// @throw qwr::QwrException
[[nodiscard]] PanelProperties LoadProperties( stream_reader& reader, abort_callback& abort );

} // namespace smp::config::binary
