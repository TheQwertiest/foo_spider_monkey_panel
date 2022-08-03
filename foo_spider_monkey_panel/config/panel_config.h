#pragma once

#include <config/panel_properties.h>
#include <config/panel_settings.h>
#include <config/raw_panel_script_source.h>

namespace smp::config
{

struct PanelConfig
{
    PanelProperties properties;
    PanelSettings panelSettings;
    RawScriptSourceVariant scriptSource;

public:
    PanelConfig();

    void ResetToDefault();

    /// @throw qwr::QwrException
    [[nodiscard]] static PanelConfig Load( stream_reader& reader, size_t size, abort_callback& abort );

    /// @throw qwr::QwrException
    void Save( stream_writer& writer, abort_callback& abort ) const;

    /// @throw qwr::QwrException
    static void SaveDefault( stream_writer& writer, abort_callback& abort );
};

} // namespace smp::config
