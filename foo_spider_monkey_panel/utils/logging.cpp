#include <stdafx.h>

#include "logging.h"

#include <qwr/delayed_executor.h>

namespace smp::utils
{

void ConsoleMessage( const qwr::u8string& message, const qwr::u8string& severity = "Info" )
{
    FB2K_console_formatter()
        << fmt::format( "{} ({}):\n{}:\n{}", SMP_NAME, SMP_UNDERSCORE_NAME, severity, message );
}

void LogDebug   ( const qwr::u8string& message ) { ConsoleMessage( message, "Debug"   ); }
void LogWarning ( const qwr::u8string& message ) { ConsoleMessage( message, "Warning" ); }
void LogError   ( const qwr::u8string& message ) { ConsoleMessage( message, "Error"   ); }

} // namespace smp::utils

namespace smp::inline error_popuop
{

void ReportException( const qwr::u8string& title, const qwr::u8string& errorText )
{
    MessageBeep( MB_ICONERROR );
    smp::utils::ConsoleMessage( fmt::format( "\n{}: {}\n", title, errorText ), "Exception" );
}

void ReportExceptionWithPopup( const qwr::u8string& title, const qwr::u8string& errorText )
{
    ReportException( title, errorText );

    fb2k::inMainThread2( [title, errorText]
    {
        qwr::DelayedExecutor::GetInstance()
            .AddTask( [errorText, title]
            {
                popup_message::g_show( errorText.c_str(), title.c_str(), popup_message::icon_error );
            } );
    } );
}

} // namespace smp::inline error_popup
