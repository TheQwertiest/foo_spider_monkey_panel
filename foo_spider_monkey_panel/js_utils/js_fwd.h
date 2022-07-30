#pragma once

class JSObject;
struct JSContext;
struct JSClass;

namespace js::frontend
{
struct CompilationStencil;
};

namespace JS
{
using Stencil = js::frontend::CompilationStencil;
}