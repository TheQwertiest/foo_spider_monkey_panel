#pragma once

#include <oleauto.h>

namespace smp::com
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr );

} // namespace smp::com
