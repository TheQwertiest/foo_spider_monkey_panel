#pragma once

namespace smp::utils::string
{

bool Contains( std::string_view sv, std::string_view str );

// Extract float number but capped at 999 and with no more than 3 fraction digits
std::optional<float> ExtractBoundNumber( std::string_view& sv );

void LeftTrimWhitespace( std::string_view& sv );
void RightTrimWhitespace( std::string_view& sv );
void TrimWhitespace( std::string_view& sv );

} // namespace smp::utils::string
