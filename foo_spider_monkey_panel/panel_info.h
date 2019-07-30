#pragma once

namespace smp::panel
{

struct PanelInfo
{
    std::u8string name;
    std::u8string version;
    std::u8string author;

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

    pfc::string8 build_info_string( bool full = true ) const
    {
        pfc::string8 ret;

        if ( !name.empty() )
        {
            ret << name.c_str();
        }
        else
        {
            ret << "{" << pfc::print_guid( m_guid_ref ) << "}";
        }

        if ( full )
        {
            if ( !version.empty() )
            {
                ret << " v" << version.c_str();
            }
            if ( !author.empty() )
            {
                ret << " by " << author.c_str();
            }
        }

        return ret;
    }

private:
    const GUID& m_guid_ref;
};

} // namespace smp::panel
