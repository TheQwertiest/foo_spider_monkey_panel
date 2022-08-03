#include <stdafx.h>

#include "resolved_panel_script_settings.h"

#include <config/module_package/package_manager.h>
#include <config/smp_package/package_manager.h>

#include <component_paths.h>

#include <qwr/visitor.h>

namespace fs = std::filesystem;

namespace smp::config
{

ResolvedPanelScriptSettings::ResolvedPanelScriptSettings( const ScriptSourceVariant& source )
    : source_( source )
{
}

ResolvedPanelScriptSettings ResolvedPanelScriptSettings::ResolveSource( const RawScriptSourceVariant& source )
{
    return std::visit( qwr::Visitor(
                           []( const RawScriptFile& arg ) {
                               ScriptFile source;
                               source.isModule = arg.isModule;
                               source.scriptPath = arg.scriptPath;

                               return ResolvedPanelScriptSettings{ std::move( source ) };
                           },
                           []( const RawInMemoryScript& arg ) {
                               InMemoryScript source;
                               source.isModule = arg.isModule;
                               source.script = arg.script;

                               return ResolvedPanelScriptSettings{ std::move( source ) };
                           },
                           []( const RawSampleFile& arg ) {
                               ScriptFile source;
                               source.isModule = arg.isModule;
                               source.scriptPath = path::ScriptSamples() / arg.name;

                               return ResolvedPanelScriptSettings( std::move( source ) );
                           },
                           []( const RawModulePackage& arg ) {
                               const auto packageJsonOpt = ModulePackageManager{}.GetPackage( arg.name );
                               qwr::QwrException::ExpectTrue( packageJsonOpt.has_value(), "Package not found: `{}`", arg.name );

                               const auto package = ModulePackage::FromFile( *packageJsonOpt );
                               package.ValidatePackagePath();

                               return ResolvedPanelScriptSettings( package );
                           },
                           []( const RawSmpPackage& arg ) {
                               const auto packageJsonOpt = SmpPackageManager{}.GetPackage( arg.id );
                               const auto valueOrEmpty = []( const qwr::u8string& str ) -> qwr::u8string {
                                   return ( str.empty() ? "<empty>" : str );
                               };
                               qwr::QwrException::ExpectTrue( packageJsonOpt.has_value(),
                                                              "Package not found: `{} ({} by {})`",
                                                              arg.id,
                                                              valueOrEmpty( arg.name ),
                                                              valueOrEmpty( arg.author ) );

                               const auto package = SmpPackage::FromFile( *packageJsonOpt );
                               package.ValidatePackagePath();

                               return ResolvedPanelScriptSettings( package );
                           } ),
                       source );
}

ScriptSourceType ResolvedPanelScriptSettings::GetSourceType() const
{
    return std::visit(
        qwr::Visitor{
            []( const ModulePackage& /*arg*/ ) { return ScriptSourceType::ModulePackage; },
            []( const SmpPackage& /*arg*/ ) { return ScriptSourceType::SmpPackage; },
            [&]( const ScriptFile& arg ) { return ( arg.isSample ? ScriptSourceType::Sample : ScriptSourceType::File ); },
            []( const InMemoryScript& /*arg*/ ) { return ScriptSourceType::InMemory; } },
        source_ );
}

bool ResolvedPanelScriptSettings::IsModuleScript() const
{
    return std::visit(
        qwr::Visitor{
            []( const ModulePackage& /*arg*/ ) { return true; },
            []( const SmpPackage& /*arg*/ ) { return false; },
            []( const ScriptFile& arg ) { return arg.isModule; },
            []( const InMemoryScript& arg ) { return arg.isModule; } },
        source_ );
}

bool ResolvedPanelScriptSettings::IsSampleScript() const
{
    return std::visit(
        qwr::Visitor{
            []( const ModulePackage& arg ) { return arg.isSample; },
            []( const SmpPackage& /*arg*/ ) { return false; },
            [&]( const ScriptFile& arg ) { return arg.isSample; },
            []( const InMemoryScript& /*arg*/ ) { return false; } },
        source_ );
}

const qwr::u8string& ResolvedPanelScriptSettings::GetScriptName() const
{
    return std::visit(
        qwr::Visitor{
            []( const ModulePackage& arg ) -> decltype( auto ) { return ( arg.displayedName ); },
            []( const SmpPackage& arg ) -> decltype( auto ) { return ( arg.name ); },
            []( const ScriptFile& arg ) -> decltype( auto ) { return ( arg.name ); },
            []( const InMemoryScript& arg ) -> decltype( auto ) { return ( arg.name ); } },
        source_ );
}

qwr::u8string ResolvedPanelScriptSettings::GetScriptVersion() const
{
    return std::visit(
        qwr::Visitor{
            []( const ModulePackage& arg ) { return arg.version.ToString(); },
            []( const SmpPackage& arg ) { return arg.version; },
            []( const ScriptFile& arg ) { return arg.version; },
            []( const InMemoryScript& arg ) { return arg.version; } },
        source_ );
}

const qwr::u8string& ResolvedPanelScriptSettings::GetScriptAuthor() const
{
    return std::visit( []( const auto& arg ) -> decltype( auto ) { return ( arg.author ); }, source_ );
}

const qwr::u8string& ResolvedPanelScriptSettings::GetScriptDescription() const
{
    return std::visit( []( const auto& arg ) -> decltype( auto ) { return ( arg.description ); }, source_ );
}

bool ResolvedPanelScriptSettings::ShouldGrabFocus() const
{
    return std::visit( []( const auto& arg ) -> decltype( auto ) { return ( arg.shouldGrabFocus ); }, source_ );
}

bool ResolvedPanelScriptSettings::ShouldEnableDragDrop() const
{
    return std::visit( []( const auto& arg ) -> decltype( auto ) { return ( arg.enableDragDrop ); }, source_ );
}

const qwr::u8string& ResolvedPanelScriptSettings::GetScript() const
{
    assert( GetSourceType() == ScriptSourceType::InMemory );
    return std::get<InMemoryScript>( source_ ).script;
}

fs::path ResolvedPanelScriptSettings::GetScriptPath() const
{
    return std::visit(
        qwr::Visitor{
            []( const ModulePackage& arg ) { return arg.GetEntryFile(); },
            []( const SmpPackage& arg ) { return arg.entryFile; },
            []( const ScriptFile& arg ) { return arg.scriptPath; },
            []( const InMemoryScript& arg ) -> fs::path {
                assert( false );
                throw qwr::QwrException( "Should not reach here" );
            } },
        source_ );
}

const SmpPackage& ResolvedPanelScriptSettings::GetSmpPackage() const
{
    assert( GetSourceType() == ScriptSourceType::SmpPackage );
    return std::get<SmpPackage>( source_ );
}

const ModulePackage& ResolvedPanelScriptSettings::GetModulePackage() const
{
    assert( GetSourceType() == ScriptSourceType::ModulePackage );
    return std::get<ModulePackage>( source_ );
}

NonPackageScript& ResolvedPanelScriptSettings::GetScriptRuntimeData()
{
    if ( std::holds_alternative<ScriptFile>( source_ ) )
    {
        return std::get<ScriptFile>( source_ );
    }
    else if ( std::holds_alternative<InMemoryScript>( source_ ) )
    {
        return std::get<InMemoryScript>( source_ );
    }
    else
    {
        assert( false );
        throw qwr::QwrException( "Should not reach here" );
    }
}

} // namespace smp::config
