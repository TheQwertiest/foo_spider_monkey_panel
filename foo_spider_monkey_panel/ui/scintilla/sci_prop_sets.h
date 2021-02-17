#pragma once

#include <map>
#include <span>

namespace smp::config::sci
{

struct ScintillaProp
{
    qwr::u8string key;
    qwr::u8string defaultval;
    qwr::u8string val;
};

using ScintillaPropList = std::vector<ScintillaProp>;

class ScintillaPropsCfg : public cfg_var
{
public:
    struct DefaultPropValue
    {
        const char* key;
        const char* defaultval;
    };

public:
    ScintillaPropsCfg( const GUID& p_guid );

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
        bool operator()( const qwr::u8string& a, const qwr::u8string& b ) const;
    };

    using ScintillaPropValues = std::map<qwr::u8string, qwr::u8string, StriCmpAscii>;

private:
    void init_data( std::span<const DefaultPropValue> p_default );
    void merge_data( const ScintillaPropValues& data_map );

private:
    ScintillaPropList m_data;
};

} // namespace smp::config::sci
