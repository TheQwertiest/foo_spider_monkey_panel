#include <stdafx.h>

#include "error_popup.h"

#include <utils/delayed_executor.h>

namespace smp::utils
{

void ReportErrorWithPopup( const std::string& errorText )
{
    FB2K_console_formatter() << errorText;
    smp::utils::DelayedExecutor::GetInstance().AddTask( [errorText] {
        popup_message::g_show( errorText.c_str(), SMP_NAME );
    } );
    MessageBeep( MB_ICONASTERISK );
}

} // namespace smp::utils
