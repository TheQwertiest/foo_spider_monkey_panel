#pragma once

namespace smp::panel
{

struct PanelInfo
{
    qwr::u8string panelId;
    qwr::u8string scriptName;
    qwr::u8string scriptVersion;
    qwr::u8string scriptAuthor;

    qwr::u8string BuildInfoString( bool full = true ) const
    {
        qwr::u8string ret = fmt::format( "{{{}}}", panelId );

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
