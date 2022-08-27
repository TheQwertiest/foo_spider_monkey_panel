#pragma once

#include <js/TypeDecls.h>

namespace mozjs::backport
{

// TODO: https://searchfox.org/mozilla-central/source/js/src/builtin/ModuleObject.cpp#2329
bool OnModuleEvaluationFailureSync( JSContext* cx,
                                    JS::HandleObject evaluationPromise );

} // namespace mozjs::backport
