#pragma once

namespace smp::dom
{

std::optional<Gdiplus::Color> FromCssColour( const std::string& cssColour );

std::string ToCssColour( const Gdiplus::Color& colour );

} // namespace smp::dom
