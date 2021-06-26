#pragma once

#include <filesystem>

namespace mozjs::hack
{

/// @brief This is a hack, don't use it unless it's REALLY necessary
/// @throw qwr::QwrException
/// @throw smp::JsException
std::optional<std::filesystem::path> GetCurrentScriptPath( JSContext* cx );

} // namespace mozjs::hack
