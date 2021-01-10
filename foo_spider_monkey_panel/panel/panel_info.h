#pragma once

namespace smp::panel
{

struct PanelInfo
{
    std::u8string panelId;
    std::u8string scriptName;
    std::u8string scriptVersion;
    std::u8string scriptAuthor;

    std::u8string BuildInfoString( bool full = true ) const
    {
        std::u8string ret = fmt::format( "{{{}}}", panelId );

        if ( !scriptName.empty() )
        {
            ret += fmt::format( ": {}", scriptName );
            if ( full )
            {
                if ( !scriptVersion.empty() )
                {
                    ret += fmt::format( " v{}", scriptVersion );
                }
                if ( !scriptAuthor.empty() )
                {
                    ret += fmt::format( " by {}", scriptAuthor );
                }
            }
        }

        return ret;
    }
};

} // namespace smp::panel
