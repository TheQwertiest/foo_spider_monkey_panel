#pragma once

#include <config/panel_config.h>

namespace smp::config::com
{

/// @throw qwr::QwrException
[[nodiscard]] PanelProperties LoadProperties( stream_reader& reader, abort_callback& abort );

} // namespace smp::config::com
