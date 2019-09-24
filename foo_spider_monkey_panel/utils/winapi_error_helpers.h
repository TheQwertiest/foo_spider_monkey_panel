#pragma once

namespace smp::error
{

_Post_satisfies_( SUCCEEDED( hr ) )
void CheckHR( HRESULT hr, std::string_view functionName );

_Post_satisfies_( checkValue ) 
void CheckWinApi( bool checkValue, std::string_view functionName );

/// @details This overload is needed for SAL: it can't understand that `(bool)ptr == true` is the same as  `ptr != null`
void CheckWinApi( _Post_notnull_ void* checkValue, std::string_view functionName );

} // namespace smp::error
