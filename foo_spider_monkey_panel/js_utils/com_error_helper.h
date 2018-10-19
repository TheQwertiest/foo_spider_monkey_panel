#pragma once

#include <oleauto.h>

struct JSContext;

namespace mozjs::error
{

void ReportActiveXError( JSContext* cx, HRESULT hresult, EXCEPINFO& exception, UINT& argerr );

} // namespace mozjs::error
