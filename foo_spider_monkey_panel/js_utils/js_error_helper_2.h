#pragma once;

struct JSErrorFormatString;
struct JSContext;

namespace mozjs
{

std::string GetCurrentExceptionText( JSContext* cx );

}
