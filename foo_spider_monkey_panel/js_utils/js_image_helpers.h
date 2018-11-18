#pragma once

#include <string>

class JSObject;
struct JSContext;

namespace mozjs::image
{

JSObject* GetImagePromise( JSContext* cx, HWND hWnd, const std::wstring& imagePath ) noexcept( false );

} // namespace mozjs::image
