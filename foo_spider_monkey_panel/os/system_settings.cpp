#include <stdafx.h>

#include "system_settings.h"

namespace smp::os
{

SystemSettings& SystemSettings::Instance()
{
    static SystemSettings ss;
    return ss;
}

SystemSettings::SystemSettings()
{
    Reinit();
}

void SystemSettings::Reinit()
{
    int32_t tmp = 0;
    bool bRet = ::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &tmp, 0 );
    if ( bRet )
    {
        scrollLines_ = static_cast<uint32_t>( tmp );
    }

    bRet = ::SystemParametersInfo( SPI_GETWHEELSCROLLCHARS, 0, &tmp, 0 );
    if ( bRet )
    {
        scrollChars_ = static_cast<uint32_t>( tmp );
    }
}

uint32_t SystemSettings::GetScrollLines() const
{
    return scrollLines_;
}

uint32_t SystemSettings::GetScrollChars() const
{
    return scrollChars_;
}

bool SystemSettings::IsPageScroll( bool isVertical ) const
{
    return ( isVertical ? scrollLines_ == WHEEL_PAGESCROLL : scrollChars_ == WHEEL_PAGESCROLL );
}

} // namespace smp::os
