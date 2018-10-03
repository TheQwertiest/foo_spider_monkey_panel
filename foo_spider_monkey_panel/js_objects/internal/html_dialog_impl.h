#pragma once

#include <optional>
#include <string>

namespace mozjs
{

std::optional<JS::Value> ShowHtmlDialogImpl( JSContext* cx, uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options );

}
