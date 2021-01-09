#pragma once

#include <oleauto.h>

namespace qwr::error
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr );

} // namespace qwr::error
