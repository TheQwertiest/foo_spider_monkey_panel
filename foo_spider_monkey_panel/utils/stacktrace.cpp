#include <stdafx.h>

#include "stacktrace.h"

#ifdef SMP_ENABLE_CXX_STACKTRACE

#    include <utils/scope_helpers.h>

#    include <adv_config.h>
#    include <dbghelp.h>
#    pragma comment( lib, "dbghelp.lib" )

#    include <new>

namespace
{

using FmtResultIt = fmt::format_to_n_result<nonstd::span<wchar_t>::iterator>;

FmtResultIt PrintSymbolName( HANDLE hProcess, DWORD64 stackFramePtr, nonstd::span<wchar_t> buffer )
{
    // Converting an address to a symbol is documented here:
    // https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address

    char symbolInfoBuffer[sizeof( SYMBOL_INFOW ) + MAX_SYM_NAME * sizeof( wchar_t )];
    SYMBOL_INFOW* pSymbolInfo = reinterpret_cast<SYMBOL_INFOW*>( symbolInfoBuffer );
    pSymbolInfo->SizeOfStruct = sizeof( SYMBOL_INFOW );
    pSymbolInfo->MaxNameLen = MAX_SYM_NAME;

    if ( !SymFromAddrW( hProcess, stackFramePtr, 0, pSymbolInfo ) )
    {
        return fmt::format_to_n( buffer.data(), buffer.size(), L"(unknown)" );
    }

    return fmt::format_to_n( buffer.data(), buffer.size(), L"{}", pSymbolInfo->Name );
};

FmtResultIt PrintFileLine( HANDLE hProcess, DWORD64 stackFramePtr, nonstd::span<wchar_t> buffer )
{
    IMAGEHLP_LINEW64 lineStruct{};
    lineStruct.SizeOfStruct = sizeof( lineStruct );

    DWORD byteOffset{};

    if ( !SymGetLineFromAddrW64( hProcess, stackFramePtr, &byteOffset, &lineStruct ) )
    {
        return fmt::format_to_n( buffer.data(), buffer.size(), L"(unknown)" );
    }

    std::wstring_view filenameView( lineStruct.FileName );
    if ( const size_t pos = filenameView.find_last_of( L"\\/" );
         pos != std::wstring_view::npos )
    {
        filenameView.remove_prefix( pos + 1 );
    }

    return fmt::format_to_n( buffer.data(), buffer.size(), L"{} : {}", filenameView, lineStruct.LineNumber );
};

} // namespace

namespace smp
{

LONG WINAPI SehHandler_ConsoleStacktrace( EXCEPTION_POINTERS* pExp, DWORD dwExpCode )
{
    const size_t kDynamicBufferSize = 4096;
    std::array<wchar_t, 256> localBuffer;
    std::unique_ptr<wchar_t[]> dynamicBuffer( new ( std::nothrow ) wchar_t[kDynamicBufferSize] );

    auto stackTraceBuffer = [&localBuffer, &dynamicBuffer, kDynamicBufferSize] {
        if ( dynamicBuffer )
        {
            return nonstd::span<wchar_t>{ dynamicBuffer.get(), kDynamicBufferSize };
        }
        else
        {
            return static_cast<nonstd::span<wchar_t>>( localBuffer );
        }
    }();

    GetStackTrace( stackTraceBuffer, GetCurrentProcess(), GetCurrentThread(), pExp->ContextRecord );
    FB2K_console_formatter() << std::wstring_view{ stackTraceBuffer.data(), stackTraceBuffer.size() };

    return EXCEPTION_CONTINUE_SEARCH;
}

void GetStackTrace( nonstd::span<wchar_t> stackTrace,
                    HANDLE hProcess,
                    HANDLE hThread,
                    const CONTEXT* pContext )
{ // avoid exceptions here, since this might be invoked from exception handler
    namespace smp_advconf = smp::config::advanced;

    assert( !stackTrace.empty() );

    nonstd::span<wchar_t> curView = stackTrace.subspan( 0, stackTrace.size() - 1 );
    smp::utils::final_action autoZeroTermination{
        [&curView] {
            curView[0] = L'\0'; ///< curView.size() is always < stackTrace.size()
        }
    };

    const uint32_t maxRecurCount = static_cast<uint32_t>( smp_advconf::stacktrace_max_recursion.get() );
    const uint32_t maxDepth = static_cast<uint32_t>( smp_advconf::stacktrace_max_depth.get() );

    if ( !hThread )
    {
        hThread = GetCurrentThread();
    }
    if ( !hProcess )
    {
        hProcess = GetCurrentProcess();
    }

    SymSetOptions( SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_LINES );
    if ( !SymInitialize( hProcess, nullptr, true ) )
    {
        auto fmtRet = fmt::format_to_n( curView.data(), curView.size(), "<failed to fetch backtrace>: SymInitialize" );
        curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
        return;
    }

    CONTEXT context = [pContext] {
        if ( pContext )
        {
            return *pContext;
        }
        else
        {
            CONTEXT tmpCtx{};
            RtlCaptureContext( &tmpCtx );
            return tmpCtx;
        }
    }();

    // According to https://docs.microsoft.com/en-us/windows/win32/debug/updated-platform-support, we should
    // prefer the 64 suffixed version of all types.
    // We always use AddrModeFlat: https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/ns-dbghelp-address64
    // We must also initialize AddrPc, AddrFrame, and AddrStack prior to calling StackWalk64
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;

    auto fmtRet = fmt::format_to_n( curView.data(), curView.size(), "SMP C++ stack trace:\n" );
    curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };

    DWORD curStackFramePtr{};
    uint32_t curRecurCounter{};
    uint32_t stackCounter{};

    // StackWalk64 is documented here: https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64
    while ( !curView.empty() && StackWalk64( IMAGE_FILE_MACHINE_I386, hProcess, hThread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr ) )
    {
        if ( stackCounter )
        {
            fmtRet = fmt::format_to_n( curView.data(), curView.size(), L"\n" );
            curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
        }

        ++stackCounter;
        if ( stackCounter > maxDepth )
        {
            auto fmtRet = fmt::format_to_n( curView.data(), curView.size(), L"\t<...>" );
            curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
            break;
        }
        if ( frame.AddrPC.Offset == curStackFramePtr )
        {
            ++curRecurCounter;
            if ( curRecurCounter > maxRecurCount )
            {
                auto fmtRet = fmt::format_to_n( curView.data(), curView.size(), L"\t<recursion...>" );
                curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
                break;
            }
        }

        fmtRet = fmt::format_to_n( curView.data(), curView.size(), L"\t" );
        curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
        if ( curView.empty() )
        {
            break;
        }

        fmtRet = PrintSymbolName( hProcess, frame.AddrPC.Offset, curView );
        curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
        if ( curView.empty() )
        {
            break;
        }

        fmtRet = fmt::format_to_n( curView.data(), curView.size(), L"\n\t\t" );
        curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
        if ( curView.empty() )
        {
            break;
        }

        fmtRet = PrintFileLine( hProcess, frame.AddrPC.Offset, curView );
        curView = nonstd::span<wchar_t>{ fmtRet.out, curView.end() };
        if ( curView.empty() )
        {
            break;
        }
    }

    SymCleanup( hProcess );
}

} // namespace smp

#endif // SMP_ENABLE_CXX_STACKTRACE
