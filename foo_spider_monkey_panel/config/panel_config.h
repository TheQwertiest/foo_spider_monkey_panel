#pragma once

#include <js_utils/serialized_value.h>

#include <memory>
#include <optional>
#include <unordered_map>

namespace smp::config
{

enum class SerializationFormat : uint8_t
{
    Com,
    Binary,
    Json
};

enum class EdgeStyle : uint8_t
{
    NoEdge = 0,
    SunkenEdge,
    GreyEdge,
    Default = NoEdge,
};

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

struct PanelSettings_InMemory
{
    qwr::u8string script = GetDefaultScript();
    bool shouldGrabFocus = true;
    bool enableDragDrop = false;

    [[nodiscard]] static qwr::u8string GetDefaultScript();
};

struct PanelSettings_File
{
    qwr::u8string path;
};

struct PanelSettings_Sample
{
    qwr::u8string sampleName;
};

struct PanelSettings_Package
{
    qwr::u8string id;      ///< unique package id
    qwr::u8string name;    ///< used for logging only
    qwr::u8string author;  ///< used for logging only
    qwr::u8string version; ///< used for logging only
};

struct PanelSettings
{
    qwr::u8string id;
    EdgeStyle edgeStyle;
    bool isPseudoTransparent;
    PanelProperties properties;

    using ScriptVariant = std::variant<PanelSettings_InMemory, PanelSettings_File, PanelSettings_Sample, PanelSettings_Package>;
    ScriptVariant payload;

public:
    PanelSettings();

    void ResetToDefault();

    /// @throw qwr::QwrException
    [[nodiscard]] static PanelSettings Load( stream_reader& reader, size_t size, abort_callback& abort );

    /// @throw qwr::QwrException
    void Save( stream_writer& writer, abort_callback& abort ) const;

    /// @throw qwr::QwrException
    static void SaveDefault( stream_writer& writer, abort_callback& abort );
};

} // namespace smp::config
