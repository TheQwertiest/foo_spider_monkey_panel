#pragma once

#include <string>

namespace smp::utils
{

void LogDebug( const qwr::u8string& message );
void LogWarning( const qwr::u8string& message );
void LogError( const qwr::u8string& message );

} // namespace smp::utils

namespace smp::inline error_popuop
{

void ReportException( const qwr::u8string& title, const qwr::u8string& errorText );
void ReportExceptionWithPopup( const qwr::u8string& title, const qwr::u8string& errorText );

} // namespace smp::inline error_popuop
