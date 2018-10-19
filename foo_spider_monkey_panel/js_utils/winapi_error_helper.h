#pragma once

namespace mozjs::error
{

void CheckHR( HRESULT hr, std::string_view functionName );
void CheckWinApi( bool checkValue, std::string_view functionName );

} // namespace mozjs::error