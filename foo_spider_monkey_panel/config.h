#pragma once

#include <js_utils/serialized_value.h>

#include <optional>

namespace smp::config
{

enum class EdgeStyle : uint8_t
{
    NO_EDGE = 0,
    SUNKEN_EDGE,
    GREY_EDGE,
};

struct PanelProperties
{
    using PropertyMap = std::unordered_map<std::wstring, std::shared_ptr<mozjs::SerializedJsValue>>;

    bool LoadBinary( stream_reader& reader, abort_callback& abort );    
    bool LoadJson( stream_reader& reader, abort_callback& abort, bool loadRawString = false );
    bool LoadLegacy( stream_reader& reader, abort_callback& abort );
    void SaveBinary( stream_writer& writer, abort_callback& abort ) const;
    void SaveJson( stream_writer& writer, abort_callback& abort, bool saveAsRawString = false ) const;

public:
    PropertyMap values;
};

struct PanelSettings
{
    PanelSettings();

    static std::u8string GetDefaultScript();

    void ResetToDefault();

    void Load( stream_reader& reader, t_size size, abort_callback& abort );
    void Save( stream_writer& writer, abort_callback& abort ) const;
    static void SaveDefault( stream_writer& writer, abort_callback& abort );

public:
    GUID guid;
    WINDOWPLACEMENT windowPlacement;

    EdgeStyle edgeStyle;
    bool shouldGrabFocus;
    bool isPseudoTransparent;

    std::u8string script;
    PanelProperties properties;
};

} // namespace smp::config
