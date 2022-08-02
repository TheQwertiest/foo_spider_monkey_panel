#include <stdafx.h>

#include "script_loader.h"

#include <config/package_utils.h>
#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <fb2k/advanced_config.h>
#include <utils/logging.h>

#include <component_paths.h>

#include <js/CompilationAndEvaluation.h>
#include <js/Modules.h>
#include <js/SourceText.h>
#include <js/engine/js_engine.h>
#include <js/engine/js_script_cache.h>
#include <js/utils/current_script_path_hack.h>
#include <qwr/fb2k_paths.h>
#include <qwr/string_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;
using namespace smp;

namespace
{

/// @throw qwr::QwrException
/// @remark Because lambda can't have [[noreturn]]
[[noreturn]] void ThrowInvalidPathError( const fs::path& invalidPath )
{
    throw qwr::QwrException( "Path does not point to a valid file: {}", invalidPath.u8string() );
}

/// @throw qwr::QwrException
auto FindSuitableFileForInclude( const fs::path& path, const std::span<const fs::path>& searchPaths )
{
    try
    {
        const auto verifyRegularFile = [&]( const auto& pathToVerify ) {
            if ( fs::is_regular_file( pathToVerify ) )
            {
                return;
            }

            if ( config::advanced::debug_log_extended_include_error )
            {
                smp::utils::LogDebug( fmt::format(
                    "`include()` failed:\n "
                    "  `{}` is not a regular file\n",
                    pathToVerify.u8string() ) );
            }
            ::ThrowInvalidPathError( pathToVerify );
        };
        const auto verifyFileExists = [&]( const auto& pathToVerify ) {
            if ( fs::exists( pathToVerify ) )
            {
                return;
            }

            if ( config::advanced::debug_log_extended_include_error )
            {
                smp::utils::LogDebug( fmt::format(
                    "`include()` failed:\n "
                    "  `{}` does not exist\n",
                    pathToVerify.u8string() ) );
            }
            ::ThrowInvalidPathError( pathToVerify );
        };

        if ( path.is_absolute() )
        {
            verifyFileExists( path );
            verifyRegularFile( path );

            return path.lexically_normal();
        }
        else
        {
            assert( !searchPaths.empty() );
            for ( const auto& searchPath: searchPaths )
            {
                const auto curPath = searchPath / path;
                if ( fs::exists( curPath ) )
                {
                    verifyRegularFile( curPath );
                    return curPath.lexically_normal();
                }
            }

            if ( config::advanced::debug_log_extended_include_error )
            {
                smp::utils::LogDebug( fmt::format(
                    "`include()` failed:\n"
                    "  file `{}` could not be found using the following search paths:\n"
                    "    {}\n",
                    path.u8string(),
                    fmt::join( searchPaths
                                   | ranges::views::transform( []( const auto& path ) { return fmt::format( "    `{}`", path.u8string() ); } ),
                               "\n  " ) ) );
            }
            ::ThrowInvalidPathError( path );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( "Failed to open file `{}`:\n"
                                 "  {}",
                                 path.u8string(),
                                 qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

/// @throw qwr::QwrException
auto FindSuitableFileForImport( const qwr::u8string& moduleSpecifier, const fs::path& curPath, const std::vector<fs::path>& packageSearchPaths )
{
    try
    {
        qwr::QwrException::ExpectTrue( !moduleSpecifier.empty(), "import for '{}' failed: empty module specifier", moduleSpecifier );

        const auto modulePath = fs::u8path( moduleSpecifier ).lexically_normal();
        const auto normalizedModuleSpecifier = modulePath.u8string();
        const auto isRelativePath = ( moduleSpecifier.starts_with( "/" ) || moduleSpecifier.starts_with( "./" ) || moduleSpecifier.starts_with( "../" ) );
        const auto isAbsolutePath = moduleSpecifier.starts_with( "file://" ) || modulePath.is_absolute();
        const auto ext = modulePath.extension().u8string();
        if ( isRelativePath || isAbsolutePath )
        { // it's a file

            const auto fullPath = [&] {
                if ( isAbsolutePath )
                {
                    constexpr char filePrefix[] = "file://";
                    return ( moduleSpecifier.starts_with( filePrefix )
                                 ? fs::u8path( moduleSpecifier.substr( std::size( filePrefix ) - 1 ) )
                                 : modulePath );
                }
                else
                {
                    return curPath / modulePath;
                }
            }();
            qwr::QwrException::ExpectTrue( fs::exists( fullPath ) && fs::is_regular_file( fullPath ), "import for '{}' failed: file not found", moduleSpecifier );

            qwr::QwrException::ExpectTrue( ext == ".js", "import for '{}' failed: relative and absolute imports must point to `.js` file", moduleSpecifier );

            return fullPath;
        }
        else
        {
            // it's a module
            const auto splitSpecifier = qwr::string::Split( normalizedModuleSpecifier, '\\' );
            qwr::QwrException::ExpectTrue( splitSpecifier.size() >= 2 && splitSpecifier[0].starts_with( "@" ), "import for '{}' failed: invalid module specifier, must be in @AUTHOR/PACKAGE_NAME format", moduleSpecifier );

            const auto packageName = fs::u8path( splitSpecifier[0].substr( 1 ) ) / splitSpecifier[1];
            const auto pathSuffix = fs::u8path( splitSpecifier.size() == 2 ? "" : qwr::string::Join( std::vector( splitSpecifier.begin() + 2, splitSpecifier.end() ), '\\' ) );

            const auto matchedPackagePaths = packageSearchPaths
                                             | ranges::views::transform( [&packageName]( const auto& packagesDir ) {
                                                   return packagesDir / packageName;
                                               } )
                                             | ranges::views::filter( []( const auto& fullPackagePath ) {
                                                   return fs::exists( fullPackagePath ) && fs::is_directory( fullPackagePath );
                                               } )
                                             | ranges::to<std::vector>;
            // TODO: remove u8string after updating fmt
            qwr::QwrException::ExpectTrue( !matchedPackagePaths.empty(), "import for '{}' failed: package not found", moduleSpecifier, packageName.u8string() );
            qwr::QwrException::ExpectTrue( matchedPackagePaths.size() == 1, "import for '{}' failed: matched multiple packages", moduleSpecifier, packageName.u8string() );

            const auto packagePath = matchedPackagePaths[0];
            if ( pathSuffix.empty() || ext.empty() )
            { // it's a module's main
                const auto fullPath = packagePath / pathSuffix / "index.js";
                qwr::QwrException::ExpectTrue( fs::exists( fullPath ) && fs::is_regular_file( fullPath ), "import for '{}' failed: could not resolve main file", moduleSpecifier );

                return fullPath;
            }
            else
            { // it's a module's file
                qwr::QwrException::ExpectTrue( ext == ".js", "import for '{}' failed: invalid file extension", moduleSpecifier );

                const auto fullPath = packagePath / pathSuffix;
                qwr::QwrException::ExpectTrue( fs::exists( fullPath ) && fs::is_regular_file( fullPath ), "import for '{}' failed: file not found", moduleSpecifier );

                return fullPath;
            }
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( "Failed to resolve module `{}`:\n"
                                 "  {}",
                                 moduleSpecifier,
                                 qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

} // namespace

namespace mozjs
{

ScriptLoader::ScriptLoader( JSContext* cx )
    : pJsCtx_( cx )
{
}

void ScriptLoader::Trace( JSTracer* trc )
{
    for ( const auto& [key, pValue]: resolvedModules_ )
    {
        JS::TraceEdge( trc, pValue.get(), "CustomHeap: resolved modules" );
    }
}

void ScriptLoader::PrepareForGc()
{
    resolvedModules_.clear();
}

void ScriptLoader::PopulateImportMeta( JSContext* cx, JS::HandleValue modulePrivate, JS::HandleObject metaObject )
{
    const auto parentScriptPathOpt = convert::to_native::ToValue<std::optional<std::wstring>>( cx, modulePrivate );
    const auto isInMemoryScript = !parentScriptPathOpt || parentScriptPathOpt == L"<main>";

    if ( !JS_DefineProperty( cx, metaObject, "url", isInMemoryScript ? JS::NullHandleValue : modulePrivate, JSPROP_ENUMERATE ) )
    {
        throw JsException();
    }
}

JSObject* ScriptLoader::GetResolvedModule( const qwr::u8string& moduleName, JS::HandleValue modulePrivate )
{
    if ( resolvedModules_.contains( moduleName ) )
    {
        return *resolvedModules_.at( moduleName ).get();
    }

    const auto parentScriptPathOpt = convert::to_native::ToValue<std::optional<std::wstring>>( pJsCtx_, modulePrivate );
    const auto curPath = [&] {
        if ( !parentScriptPathOpt || parentScriptPathOpt == L"<main>" )
        {
            return qwr::path::Component();
        }
        else
        {
            return fs::path{ parentScriptPathOpt->substr( std::size( L"file://" ) - 1 ) }.parent_path();
        }
    }();

    const std::vector packageSearchPaths{ smp::path::Modules_Profile(), smp::path::Modules_Sample() };
    const auto scriptPath = FindSuitableFileForImport( moduleName, curPath, packageSearchPaths );

    const auto compiledModule = JsEngine::GetInstance().GetScriptCache().GetCachedModule( pJsCtx_, scriptPath );

    resolvedModules_.try_emplace( moduleName, std::make_unique<JS::Heap<JSObject*>>( compiledModule ) );

    return compiledModule;
}

void ScriptLoader::ExecuteTopLevelScript( const qwr::u8string& script, bool isModule )
{
    isModule_ = isModule;

    JS::SourceText<mozilla::Utf8Unit> source;
    if ( !source.init( pJsCtx_, script.c_str(), script.length(), JS::SourceOwnership::Borrowed ) )
    {
        throw JsException();
    }

    JS::CompileOptions opts( pJsCtx_ );
    opts.setFileAndLine( "<main>", 1 );

    if ( isModule )
    {
        JS::RootedObject jsModule( pJsCtx_, JS::CompileModule( pJsCtx_, opts, source ) );
        JsException::ExpectTrue( jsModule );

        JS::RootedValue jsScriptPath( pJsCtx_ );
        convert::to_js::ToValue( pJsCtx_, qwr::u8string_view{ opts.filename() }, &jsScriptPath );
        JS::SetModulePrivate( jsModule, jsScriptPath );

        if ( !JS::ModuleInstantiate( pJsCtx_, jsModule ) )
        {
            throw JsException();
        }

        JS::RootedValue dummyRval( pJsCtx_ );
        if ( !JS::ModuleEvaluate( pJsCtx_, jsModule, &dummyRval ) )
        {
            throw JsException();
        }
    }
    else
    {
        JS::RootedValue dummyRval( pJsCtx_ );
        if ( !JS::Evaluate( pJsCtx_, opts, source, &dummyRval ) )
        {
            throw JsException();
        }
    }
}

void ScriptLoader::ExecuteTopLevelScriptFile( const std::filesystem::path& scriptPath, bool isModule )
{
    isModule_ = isModule;

    const std::vector allSearchPaths{ qwr::path::Component() };
    const auto fullScriptPath = ::FindSuitableFileForInclude( scriptPath, allSearchPaths );

    if ( isModule )
    {
        JS::RootedObject jsModule( pJsCtx_, JsEngine::GetInstance().GetScriptCache().GetCachedModule( pJsCtx_, fullScriptPath ) );
        JsException::ExpectTrue( jsModule );

        if ( !JS::ModuleInstantiate( pJsCtx_, jsModule ) )
        {
            throw JsException();
        }

        JS::RootedValue dummyRval( pJsCtx_ );
        if ( !JS::ModuleEvaluate( pJsCtx_, jsModule, &dummyRval ) )
        {
            throw JsException();
        }
    }
    else
    {
        includedFiles_.emplace( fullScriptPath.u8string() );

        JS::RootedScript jsScript( pJsCtx_, JsEngine::GetInstance().GetScriptCache().GetCachedScript( pJsCtx_, fullScriptPath ) );
        assert( jsScript );

        JS::RootedValue dummyRval( pJsCtx_ );
        if ( !JS_ExecuteScript( pJsCtx_, jsScript, &dummyRval ) )
        {
            throw JsException();
        }
    }
}

void ScriptLoader::IncludeScript( const qwr::u8string& path, const config::ParsedPanelSettings& panelSettings, bool alwaysEvaluate )
{
    qwr::QwrException::ExpectTrue( !isModule_, "include() can't be used with in modules" );

    const auto allSearchPaths = [&] {
        std::vector<fs::path> paths;
        if ( const auto currentPathOpt = hack::GetCurrentScriptPath( pJsCtx_ );
             currentPathOpt )
        {
            paths.emplace_back( currentPathOpt->parent_path() );
        }
        if ( panelSettings.packageId )
        {
            paths.emplace_back( config::GetPackageScriptsDir( panelSettings ) );
        }
        paths.emplace_back( qwr::path::Component() );

        return paths;
    }();

    const auto fsPath = ::FindSuitableFileForInclude( fs::u8path( path ), allSearchPaths );

    const auto u8Path = fsPath.u8string();
    if ( !alwaysEvaluate && includedFiles_.contains( u8Path ) )
    {
        return;
    }

    includedFiles_.emplace( u8Path );

    JS::RootedScript jsScript( pJsCtx_, JsEngine::GetInstance().GetScriptCache().GetCachedScript( pJsCtx_, fsPath ) );
    assert( jsScript );

    JS::RootedValue dummyRval( pJsCtx_ );
    if ( !JS_ExecuteScript( pJsCtx_, jsScript, &dummyRval ) )
    {
        throw JsException();
    }
}

} // namespace mozjs
