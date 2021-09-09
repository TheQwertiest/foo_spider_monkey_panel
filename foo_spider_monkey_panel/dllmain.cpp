#include <stdafx.h>

#include <config/delayed_package_utils.h>
#include <events/event_dispatcher.h>
#include <js_engine/js_engine.h>
#include <utils/thread_pool_instance.h>

#include <qwr/abort_callback.h>
#include <qwr/delayed_executor.h>
#include <qwr/error_popup.h>

#include <Scintilla.h>

#include <unordered_map>

DECLARE_COMPONENT_VERSION( SMP_NAME, SMP_VERSION, SMP_ABOUT );
VALIDATE_COMPONENT_FILENAME( SMP_DLL_NAME );

namespace smp::com
{
// Script TypeLib
ITypeLibPtr g_typelib;
} // namespace smp::com

namespace
{

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

} // namespace

namespace
{

ULONG_PTR g_pGdiToken = 0;
CAppModule g_wtlModule;
HMODULE g_hRichEdit = nullptr;

std::unordered_map<SubsystemId::Type, SubsystemError> g_subsystemFailures;

} // namespace

namespace
{

void InitializeSubsystems( HINSTANCE ins )
{
    if ( auto hr = OleInitialize( nullptr );
         FAILED( hr ) )
    {
        g_subsystemFailures[SubsystemId::SMP_OLE] = { "OleInitialize failed", static_cast<uint32_t>( hr ) };
    }

    {
        std::array<wchar_t, MAX_PATH> path{};
        (void)GetModuleFileName( ins, path.data(), path.size() ); // NULL-terminated in OS newer than WinXP

        if ( auto hr = LoadTypeLibEx( path.data(), REGKIND_NONE, &smp::com::g_typelib );
             FAILED( hr ) )
        {
            g_subsystemFailures[SubsystemId::SMP_TYPELIB] = { "LoadTypeLibEx failed", static_cast<uint32_t>( hr ) };
        }
    }

    if ( int iRet = Scintilla_RegisterClasses( ins );
         !iRet )
    {
        g_subsystemFailures[SubsystemId::SMP_SCINTILLA] = { "Scintilla_RegisterClasses failed", static_cast<uint32_t>( iRet ) };
    }

    {
        Gdiplus::GdiplusStartupInput gdip_input;
        if ( auto gdiRet = Gdiplus::GdiplusStartup( &g_pGdiToken, &gdip_input, nullptr );
             Gdiplus::Ok != gdiRet )
        {
            g_subsystemFailures[SubsystemId::SMP_GDIPLUS] = { "OleInitialize failed", static_cast<uint32_t>( gdiRet ) };
        }
    }

    if ( auto hr = g_wtlModule.Init( nullptr, ins );
         FAILED( hr ) )
    {
        g_subsystemFailures[SubsystemId::SMP_WTL] = { "WTL module Init failed", static_cast<uint32_t>( hr ) };
    }

    // WTL ActiveX support
    if ( !AtlAxWinInit() )
    {
        g_subsystemFailures[SubsystemId::SMP_WTL_AX] = { "AtlAxWinInit failed", static_cast<uint32_t>( GetLastError() ) };
    }

    {
        g_hRichEdit = LoadLibrary( CRichEditCtrl::GetLibraryName() );
        if ( !g_hRichEdit )
        {
            g_subsystemFailures[SubsystemId::SMP_RICHEDIT] = { "Failed to load RichEdit library", static_cast<uint32_t>( GetLastError() ) };
        }
    }
}

void FinalizeSubsystems()
{
    if ( !g_subsystemFailures.contains( SubsystemId::SMP_RICHEDIT ) )
    {
        FreeLibrary( g_hRichEdit );
    }
    if ( !g_subsystemFailures.contains( SubsystemId::SMP_WTL ) )
    {
        g_wtlModule.Term();
    }
    if ( !g_subsystemFailures.contains( SubsystemId::SMP_GDIPLUS ) )
    {
        Gdiplus::GdiplusShutdown( g_pGdiToken );
    }
    if ( !g_subsystemFailures.contains( SubsystemId::SMP_SCINTILLA ) )
    {
        Scintilla_ReleaseResources();
    }
    if ( !g_subsystemFailures.contains( SubsystemId::SMP_OLE ) )
    {
        OleUninitialize();
    }
}

class InitStageCallback : public init_stage_callback
{
    void on_init_stage( t_uint32 stage ) override
    {
        if ( stage == init_stages::before_config_read )
        { // process delayed packages here to avoid potential file locks after config reads
            try
            {
                smp::config::ProcessDelayedPackages();
            }
            catch ( const qwr::QwrException& e )
            {
                qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Failed to process delayed packages:\n{}", e.what() ) );
            }
        }
    }
};

class InitQuitSmp : public initquit
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
        smp::EventDispatcher::Get().NotifyAllAboutExit();
        qwr::GlobalAbortCallback::GetInstance().Abort();
        smp::GetThreadPoolInstance().Finalize();
    }

private:
    static void CheckSubsystemStatus()
    {
        if ( g_subsystemFailures.empty() )
        {
            return;
        }

        std::string errorText =
            "Spider Monkey Panel initialization failed!\r\n"
            "The component will not function properly!\r\n"
            "Failures:\r\n\r\n";

        for ( const auto& [key, failure]: g_subsystemFailures )
        {
            errorText += fmt::format( "{}: error code: {:#x}\r\n", failure.description, failure.errorCode );
        }

        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorText );
    }
};

FB2K_SERVICE_FACTORY( InitStageCallback );
FB2K_SERVICE_FACTORY( InitQuitSmp );

} // namespace

extern "C" BOOL WINAPI DllMain( HINSTANCE ins, DWORD reason, LPVOID /*lp*/ )
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
