#pragma once

#include <js_utils/js_fwd.h>

#include <mozilla/RefPtr.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace mozjs
{

class JsScriptCache
{
public:
    JsScriptCache();
    ~JsScriptCache();

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] JSScript* GetCachedScript( JSContext* pJsCtx, const std::filesystem::path& absolutePath );

private:
    [[nodiscard]] RefPtr<JS::Stencil> GetCachedStencil( JSContext* pJsCtx, const std::filesystem::path& absolutePath, const std::string& hackedPathId, const JS::CompileOptions& compileOpts );

private:
    struct CachedScriptStencil
    {
        RefPtr<JS::Stencil> scriptStencil;
        std::filesystem::file_time_type writeTime;
    };

    std::unordered_map<qwr::u8string, CachedScriptStencil> scriptCache_;
};

} // namespace mozjs
