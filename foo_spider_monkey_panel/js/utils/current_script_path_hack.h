#pragma once

#include <filesystem>

// There is no proper API to retrieve the JSScript object associated
// with the currently executing script (aside from Debugger API).
// Hence hacks had to be made...

namespace mozjs::hack
{

/// @brief This is a hack, don't use it unless it's REALLY necessary
/// @throw qwr::QwrException
/// @throw smp::JsException
[[nodiscard]] std::optional<std::filesystem::path>
GetCurrentScriptPath( JSContext* cx );

} // namespace mozjs::hack
