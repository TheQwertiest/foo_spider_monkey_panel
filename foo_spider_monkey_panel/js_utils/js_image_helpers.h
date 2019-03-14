#pragma once

#include <string>

class JSObject;
struct JSContext;

namespace mozjs::image
{

/// @throw smp::SmpException
/// @throw smp::JsException
JSObject* GetImagePromise( JSContext* cx, HWND hWnd, const std::wstring& imagePath );

} // namespace mozjs::image
