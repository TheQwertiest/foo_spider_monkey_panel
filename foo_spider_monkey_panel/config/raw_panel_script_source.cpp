#include <stdafx.h>

#include "raw_panel_script_source.h"

#include <qwr/visitor.h>

namespace smp::config
{

ScriptSourceType GetSourceType( const RawScriptSourceVariant& source )
{
    return std::visit(
        qwr::Visitor{
            []( const RawModulePackage& /*arg*/ ) { return ScriptSourceType::ModulePackage; },
            []( const RawSmpPackage& /*arg*/ ) { return ScriptSourceType::SmpPackage; },
            [&]( const RawScriptFile& /*arg*/ ) { return ScriptSourceType::File; },
            [&]( const RawSampleFile& /*arg*/ ) { return ScriptSourceType::Sample; },
            []( const RawInMemoryScript& /*arg*/ ) { return ScriptSourceType::InMemory; } },
        source );
}

} // namespace smp::config
