#pragma once

#include <js_utils/serialized_value.h>

#include <optional>

namespace smp::config
{

enum class Version : uint32_t
{
    SMP_VERSION_100 = 1, // must start with 1 so we don't break component upgrades
    CONFIG_VERSION_CURRENT = SMP_VERSION_100
};

enum class EdgeStyle : uint8_t
{
    NO_EDGE = 0,
    SUNKEN_EDGE,
    GREY_EDGE,
};

DWORD edge_style_from_config( EdgeStyle edge_style );

class PanelProperties
{
public:
    using config_map = std::unordered_map<std::wstring, std::shared_ptr<mozjs::SerializedJsValue>>;

    config_map& get_val();
    std::optional<mozjs::SerializedJsValue> get_config_item( const std::wstring& propName );
    void set_config_item( const std::wstring& propName, const mozjs::SerializedJsValue& serializedValue );
    void remove_config_item( const std::wstring& propName );

    static void g_load( config_map& data, stream_reader* reader, abort_callback& abort ) throw();
    static void g_save( const config_map& data, stream_writer* writer, abort_callback& abort ) throw();
    void load( stream_reader* reader, abort_callback& abort ) throw();
    void save( stream_writer* writer, abort_callback& abort ) const throw();

private:
    config_map m_map;
};

class PanelSettings
{
public:
    PanelSettings();

public:
    GUID& get_config_guid();
    WINDOWPLACEMENT& get_windowplacement();
    bool& get_grab_focus();
    bool& get_pseudo_transparent();
    const bool& get_pseudo_transparent() const;
    const EdgeStyle& get_edge_style() const;
    pfc::string_base& get_script_code();
    PanelProperties& get_config_prop();
    static pfc::string8_fast get_default_script_code();
    EdgeStyle& get_edge_style();

    void load_config( stream_reader* reader, t_size size, abort_callback& abort );
    void reset_config();
    void save_config( stream_writer* writer, abort_callback& abort ) const;

private:
    GUID m_config_guid;
    WINDOWPLACEMENT m_wndpl;
    PanelProperties m_config_prop;
    pfc::string8 m_script_code;
    EdgeStyle m_edge_style;
    bool m_grab_focus;
    bool m_pseudo_transparent;
};

} // namespace smp::config
