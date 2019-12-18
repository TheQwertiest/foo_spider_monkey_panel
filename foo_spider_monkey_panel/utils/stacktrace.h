#include <nonstd/span.hpp>

namespace smp
{

LONG WINAPI SehHandler_ConsoleStacktrace( EXCEPTION_POINTERS* pExp, DWORD dwExpCode );

void GetStackTrace( nonstd::span<wchar_t> stackTrace,
                    HANDLE hProcess = nullptr,
                    HANDLE hThread = nullptr,
                    const CONTEXT* pContext = nullptr );

} // namespace smp
