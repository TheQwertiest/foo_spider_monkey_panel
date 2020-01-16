#pragma once

#include <nonstd/span.hpp>

#include <map>

namespace smp::ui::sci
{

struct ScintillaProp
{
    std::u8string key;
    std::u8string defaultval;
    std::u8string val;
};

using ScintillaPropList = std::vector<ScintillaProp>;

class ScintillaCfg : public cfg_var
{
public:
    struct DefaultPropValue
    {
        const char* key;
        const char* defaultval;
    };

public:
    ScintillaCfg( const GUID& p_guid, nonstd::span<const DefaultPropValue> p_default );

    [[nodiscard]] ScintillaPropList& val();
    [[nodiscard]] const ScintillaPropList& val() const;

    // cfg_var
    void get_data_raw( stream_writer* p_stream, abort_callback& p_abort ) override;
    void set_data_raw( stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort ) override;

    void reset();
    void export_to_file( const wchar_t* filename );
    void import_from_file( const char* filename );

private:
    struct StriCmpAscii
    {
        bool operator()( const std::u8string& a, const std::u8string& b ) const;
    };

    using ScintillaPropValues = std::map<std::u8string, std::u8string, StriCmpAscii>;

private:
    void init_data( nonstd::span<const DefaultPropValue> p_default );
    void merge_data( const ScintillaPropValues& data_map );

private:
    ScintillaPropList m_data;
};

extern ScintillaCfg g_scintillaCfg;

} // namespace scintilla
