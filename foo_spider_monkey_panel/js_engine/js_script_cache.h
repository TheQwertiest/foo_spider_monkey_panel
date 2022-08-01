#pragma once

#include <js/TypeDecls.h>
#include <mozilla/RefPtr.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace js::frontend
{
struct CompilationStencil;
};

namespace JS
{
using Stencil = js::frontend::CompilationStencil;
}

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

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] JSObject* GetCachedModule( JSContext* pJsCtx, const std::filesystem::path& absolutePath );

private:
    [[nodiscard]] RefPtr<JS::Stencil> GetCachedStencil( JSContext* pJsCtx, const std::filesystem::path& absolutePath, const std::string& hackedPathId, const JS::CompileOptions& compileOpts, bool isModule );

private:
    struct CachedStencil
    {
        RefPtr<JS::Stencil> stencil;
        std::filesystem::file_time_type writeTime;
    };

    std::unordered_map<qwr::u8string, CachedStencil> scriptCache_;
    std::unordered_map<qwr::u8string, CachedStencil> moduleCache_;
};

} // namespace mozjs
