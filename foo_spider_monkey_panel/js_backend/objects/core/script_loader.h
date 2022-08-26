#pragma once

#include <js_backend/objects/core/builtin_modules.h>

#include <js/TypeDecls.h>

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace smp::config
{

class ResolvedPanelScriptSettings;

}

namespace mozjs
{

class ScriptLoader
{
public:
    ScriptLoader( JSContext* cx );

public:
    void Trace( JSTracer* trc );
    void PrepareForGc();

public:
    /// @throw qwr::QwrException
    /// @throw smp::JsException
    static void PopulateImportMeta( JSContext* cx, JS::HandleValue modulePrivate, JS::HandleObject metaObject );

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    JSObject* GetResolvedModule( const qwr::u8string& moduleName, JS::HandleValue modulePrivate );

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    void ExecuteTopLevelScript( const qwr::u8string& script, bool isModule );

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    void ExecuteTopLevelScriptFile( const std::filesystem::path& scriptPath, bool isModule );

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    void IncludeScript( const qwr::u8string& path, const smp::config::ResolvedPanelScriptSettings& scriptSettings, bool alwaysEvaluate = false );

private:
    JSObject* GetCompiledModule( const std::filesystem::path& scriptPath );
    JSObject* GetCompiledInternalModule( BuiltinModuleId moduleId );
    JSScript* GetCompiledScript( const std::filesystem::path& scriptPath );

private:
    JSContext* pJsCtx_ = nullptr;

    bool isModule_ = false;
    std::unordered_set<std::string> includedFiles_;
    std::unordered_map<std::string, std::unique_ptr<JS::Heap<JSObject*>>> resolvedModules_;
};

} // namespace mozjs
