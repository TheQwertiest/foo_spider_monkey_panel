#pragma once

namespace smp::panel
{

struct PanelInfo
{
    pfc::string_simple name;
    pfc::string_simple version;
    pfc::string_simple author;

    PanelInfo( const GUID& guid_ref )
        : m_guid_ref( guid_ref )
    {
    }

    void clear()
    {
        name = "";
        version = "";
        author = "";
    }

    pfc::string8 build_info_string() const
    {
        pfc::string8 ret;

        if ( !name.is_empty() )
        {
            ret << name;
        }
        else
        {
            ret << "{" << pfc::print_guid( m_guid_ref ) << "}";
        }

        if ( !version.is_empty() )
        {
            ret << " v" << version;
        }
        if ( !author.is_empty() )
        {
            ret << " by " << author;
        }

        return ret;
    }

private:
    const GUID& m_guid_ref;
};

} // namespace smp::panel
