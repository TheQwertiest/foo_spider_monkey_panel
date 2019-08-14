#include <stdafx.h>
#include "error_popup.h"

#include <utils/delayed_executor.h>

namespace smp::utils
{

void ShowErrorPopup( const std::string& errorText )
{
    FB2K_console_formatter() << errorText.c_str();
    smp::utils::DelayedExecutor::GetInstance().AddTask( [errorText] {
        popup_message::g_show( errorText.c_str(), SMP_NAME );
    } );
    MessageBeep( MB_ICONASTERISK );
}

} // namespace smp::utils
