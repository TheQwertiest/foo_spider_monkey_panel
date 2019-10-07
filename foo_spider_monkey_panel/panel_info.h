#pragma once

namespace smp::panel
{

struct PanelInfo
{
    std::u8string name;
    std::u8string version;
    std::u8string author;

    PanelInfo( const GUID& guid )
        : guid_( guid )
    {
    }

    void clear()
    {
        name.clear();
        version.clear();
        author.clear();
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
            ret += fmt::format( "{{{}}}", pfc::print_guid( guid_ ) );
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
    GUID guid_;
};

} // namespace smp::panel
