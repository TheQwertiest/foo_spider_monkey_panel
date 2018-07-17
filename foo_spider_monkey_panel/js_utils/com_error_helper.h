#pragma once

#include <oleauto.h>

struct JSContext;

namespace mozjs
{

void ReportActiveXError( JSContext* cx, HRESULT hresult, EXCEPINFO& exception, UINT& argerr );

}