#pragma once

#include <oleauto.h>

namespace smp::error
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr );

} // namespace smp::error
