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

    std::u8string build_info_string( bool full = true ) const
    {
        std::u8string ret;

        if ( !name.empty() )
        {
            ret += name;
        }
        else
        {
            ret += fmt::format( "{{{}}}", pfc::print_guid( m_guid_ref ) );
        }

        if ( full )
        {
            if ( !version.empty() )
            {
                ret += fmt::format( " v{}", version );
            }
            if ( !author.empty() )
            {
                ret += fmt::format( " by {}", author );
            }
        }

        return ret;
    }

private:
    const GUID& m_guid_ref;
};

} // namespace smp::panel
