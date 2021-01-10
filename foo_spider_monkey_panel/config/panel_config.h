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

enum class PackageLocation : uint8_t
{
    Sample = 0,
    LocalAppData = 1,
    Fb2k = 2,
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
    [[nodiscard]] static PanelProperties FromJson( const std::u8string& jsonString );

    /// @throw qwr::QwrException
    [[nodiscard]] std::u8string ToJson() const;

    /// @throw qwr::QwrException
    [[nodiscard]] static PanelProperties Load( stream_reader& reader, abort_callback& abort, SerializationFormat format = SerializationFormat::Json );

    /// @throw qwr::QwrException
    void Save( stream_writer& writer, abort_callback& abort ) const;
};

struct PanelSettings_InMemory
{
    std::u8string script = GetDefaultScript();
    bool shouldGrabFocus = true;
    bool enableDragDrop = false;

    [[nodiscard]] static std::u8string GetDefaultScript();
};

struct PanelSettings_File
{
    std::u8string path;
};

struct PanelSettings_Sample
{
    std::u8string sampleName;
};

struct PanelSettings_Package
{
    std::u8string id;      ///< unique package id
    std::u8string name;    ///< used for logging only
    std::u8string author;  ///< used for logging only
    std::u8string version; ///< used for logging only
};

struct PanelSettings
{
    std::u8string id;
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
