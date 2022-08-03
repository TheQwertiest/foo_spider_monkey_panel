#pragma once

#include <config/panel_config_formats.h>
#include <js_backend/utils/serialized_value.h>

#include <optional>
#include <unordered_map>

namespace smp::config
{

struct PanelProperties
{
    using PropertyMap = std::unordered_map<std::wstring, std::shared_ptr<mozjs::SerializedJsValue>>;
    PropertyMap values;

public:
    /// @throw qwr::QwrException
    [[nodiscard]] static PanelProperties FromJson( const qwr::u8string& jsonString );

    /// @throw qwr::QwrException
    [[nodiscard]] qwr::u8string ToJson() const;

    /// @throw qwr::QwrException
    [[nodiscard]] static PanelProperties Load( stream_reader& reader, abort_callback& abort, SerializationFormat format = SerializationFormat::Json );

    /// @throw qwr::QwrException
    void Save( stream_writer& writer, abort_callback& abort ) const;
};

} // namespace smp::config
