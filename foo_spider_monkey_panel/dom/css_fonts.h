#pragma once

#include <dom/font_description.h>

#include <optional>

namespace smp::dom
{

std::optional<FontDescription> FromCssFont( const std::wstring& cssFont );

} // namespace smp::dom
