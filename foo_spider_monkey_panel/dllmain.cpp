#include <stdafx.h>

#include <config/package_utils.h>
#include <js_engine/js_engine.h>
#include <panel/message_manager.h>
#include <panel/user_message.h>
#include <utils/thread_pool_instance.h>

#include <qwr/abort_callback.h>
#include <qwr/delayed_executor.h>
#include <qwr/error_popup.h>

#include <Scintilla.h>

#include <unordered_map>

DECLARE_COMPONENT_VERSION( SMP_NAME, SMP_VERSION, SMP_ABOUT );
VALIDATE_COMPONENT_FILENAME( SMP_DLL_NAME );

// Script TypeLib
ITypeLibPtr g_typelib;

namespace
{

ULONG_PTR g_pGdiToken = 0;
CAppModule g_wtlModule;
HMODULE g_hRichEdit = nullptr;

namespace SubsystemId
{
enum Type : uint32_t
{
    SMP_OLE,
    SMP_TYPELIB,
    SMP_SCINTILLA,
    SMP_GDIPLUS,
    SMP_WTL,
    SMP_WTL_AX,
    SMP_RICHEDIT,
    ENUM_END
};
}

struct SubsystemError
{
    const char* description = "";
    uint32_t errorCode = 0;
};

std::unordered_map<SubsystemId::Type, SubsystemError> g_subsystem_failures;

void InitializeSubsystems( HINSTANCE ins )
{
    if ( HRESULT hr = OleInitialize( nullptr );
         FAILED( hr ) )
    {
        g_subsystem_failures[SubsystemId::SMP_OLE] = { "OleInitialize failed", static_cast<uint32_t>( hr ) };
    }

    {
        std::array<wchar_t, MAX_PATH> path{};
        (void)GetModuleFileName( ins, path.data(), path.size() ); // NULL-terminated in OS newer than WinXP

        if ( HRESULT hr = LoadTypeLibEx( path.data(), REGKIND_NONE, &g_typelib );
             FAILED( hr ) )
        {
            g_subsystem_failures[SubsystemId::SMP_TYPELIB] = { "LoadTypeLibEx failed", static_cast<uint32_t>( hr ) };
        }
    }

    if ( int iRet = Scintilla_RegisterClasses( ins );
         !iRet )
    {
        g_subsystem_failures[SubsystemId::SMP_SCINTILLA] = { "Scintilla_RegisterClasses failed", static_cast<uint32_t>( iRet ) };
    }

    {
        Gdiplus::GdiplusStartupInput gdip_input;
        if ( Gdiplus::Status gdiRet = Gdiplus::GdiplusStartup( &g_pGdiToken, &gdip_input, nullptr );
             Gdiplus::Ok != gdiRet )
        {
            g_subsystem_failures[SubsystemId::SMP_GDIPLUS] = { "OleInitialize failed", static_cast<uint32_t>( gdiRet ) };
        }
    }

    if ( HRESULT hr = g_wtlModule.Init( nullptr, ins );
         FAILED( hr ) )
    {
        g_subsystem_failures[SubsystemId::SMP_WTL] = { "WTL module Init failed", static_cast<uint32_t>( hr ) };
    }

    // WTL ActiveX support
    if ( !AtlAxWinInit() )
    {
        g_subsystem_failures[SubsystemId::SMP_WTL_AX] = { "AtlAxWinInit failed", static_cast<uint32_t>( GetLastError() ) };
    }

    {
        g_hRichEdit = LoadLibrary( CRichEditCtrl::GetLibraryName() );
        if ( !g_hRichEdit )
        {
            g_subsystem_failures[SubsystemId::SMP_RICHEDIT] = { "Failed to load RichEdit library", static_cast<uint32_t>( GetLastError() ) };
        }
    }
}

void FinalizeSubsystems()
{
    if ( !g_subsystem_failures.contains( SubsystemId::SMP_RICHEDIT ) )
    {
        FreeLibrary( g_hRichEdit );
    }
    if ( !g_subsystem_failures.contains( SubsystemId::SMP_WTL ) )
    {
        g_wtlModule.Term();
    }
    if ( !g_subsystem_failures.contains( SubsystemId::SMP_GDIPLUS ) )
    {
        Gdiplus::GdiplusShutdown( g_pGdiToken );
    }
    if ( !g_subsystem_failures.contains( SubsystemId::SMP_SCINTILLA ) )
    {
        Scintilla_ReleaseResources();
    }
    if ( !g_subsystem_failures.contains( SubsystemId::SMP_OLE ) )
    {
        OleUninitialize();
    }
}

class js_initquit : public initquit
{
public:
    void on_init() override
    {
        qwr::DelayedExecutor::GetInstance().EnableExecution(); ///< Enable task processing (e.g. error popups)
        CheckSubsystemStatus();
    }

    void on_quit() override
    { // Careful when changing invocation order here!
        mozjs::JsEngine::GetInstance().PrepareForExit();
        smp::panel::message_manager::instance().send_msg_to_all( static_cast<UINT>( smp::InternalSyncMessage::terminate_script ) );
        qwr::GlobalAbortCallback::GetInstance().Abort();
        smp::GetThreadPoolInstance().Finalize();
    }

private:
    static void CheckSubsystemStatus()
    {
        if ( g_subsystem_failures.empty() )
        {
            return;
        }

        std::string errorText =
            "Spider Monkey Panel initialization failed!\r\n"
            "The component will not function properly!\r\n"
            "Failures:\r\n\r\n";

        for ( const auto& [key, failure]: g_subsystem_failures )
        {
            errorText += fmt::format( "{}: error code: {:#x}\r\n", failure.description, failure.errorCode );
        }

        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorText );
    }
};

FB2K_SERVICE_FACTORY( js_initquit );

} // namespace

extern "C" BOOL WINAPI DllMain( HINSTANCE ins, DWORD reason, [[maybe_unused]] LPVOID lp )
{
    switch ( reason )
    {
    case DLL_PROCESS_ATTACH:
    {
        InitializeSubsystems( ins );
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        FinalizeSubsystems();
        break;
    }
    default:
        break;
    }

    return TRUE;
}
