#include "stdafx.h"
#include "thread_pool.h"
#include "popup_msg.h"
#include "panel_manager.h"
#include "user_message.h"

#include <js_engine/js_engine.h>

#include <map>
#include <sstream>

DECLARE_COMPONENT_VERSION(
    SMP_NAME,
    SMP_VERSION,
    SMP_NAME_WITH_VERSION " by TheQwertiest\n"
                          "Based on JScript Panel by marc2003\n"
                          "Based on WSH Panel Mod by T.P. Wang\n\n"
                          "Build: " __TIME__ ", " __DATE__ "\n"
                          "Columns UI SDK Version: " UI_EXTENSION_VERSION );

VALIDATE_COMPONENT_FILENAME( SMP_DLL_NAME );

// Script TypeLib
ITypeLibPtr g_typelib;

namespace
{

ULONG_PTR g_gdip_token;
CAppModule g_wtl_module;

enum SubsystemId : uint32_t
{
    SMP_OLE,
    SMP_TYPELIB,
    SMP_SCINTILLA,
    SMP_GDIPLUS,
    SMP_WTL,
    SMP_WTL_AX,
    ENUM_END
};

struct SubsystemError
{
    const char* description = "";
    uint32_t errorCode = 0;
};

std::map<SubsystemId, SubsystemError> g_subsystem_failures;

void InitializeSubsystems( HINSTANCE ins )
{
    if ( HRESULT hr = OleInitialize( nullptr );
         FAILED( hr ) )
    {
        g_subsystem_failures[SMP_OLE] = { "OleInitialize failed", static_cast<uint32_t>( hr ) };
    }

    {
        wchar_t path[MAX_PATH];
        DWORD len = GetModuleFileName( ins, path, sizeof( path ) ); // NULL-terminated in OS newer than WinXP

        if ( HRESULT hr = LoadTypeLibEx( path, REGKIND_NONE, &g_typelib );
             FAILED( hr ) )
        {
            g_subsystem_failures[SMP_TYPELIB] = { "LoadTypeLibEx failed", static_cast<uint32_t>( hr ) };
        }
    }

    if ( int iRet = Scintilla_RegisterClasses( ins );
         !iRet )
    {
        g_subsystem_failures[SMP_SCINTILLA] = { "Scintilla_RegisterClasses failed", static_cast<uint32_t>( iRet ) };
    }

    {
        Gdiplus::GdiplusStartupInput gdip_input;
        if ( Gdiplus::Status gdiRet = Gdiplus::GdiplusStartup( &g_gdip_token, &gdip_input, nullptr );
             Gdiplus::Ok != gdiRet )
        {
            g_subsystem_failures[SMP_GDIPLUS] = { "OleInitialize failed", static_cast<uint32_t>( gdiRet ) };
        }
    }

    if ( HRESULT hr = g_wtl_module.Init( nullptr, ins );
         FAILED( hr ) )
    {
        g_subsystem_failures[SMP_WTL] = { "WTL module Init failed", static_cast<uint32_t>( hr ) };
    }

    // WTL ActiveX support
    if ( !AtlAxWinInit() )
    {
        g_subsystem_failures[SMP_WTL_AX] = { "AtlAxWinInit failed", static_cast<uint32_t>( GetLastError() ) };
    }
}

void FinalizeSubsystems()
{
    if ( !g_subsystem_failures.count( SMP_WTL ) )
    {
        g_wtl_module.Term();
    }
    if ( !g_subsystem_failures.count( SMP_GDIPLUS ) )
    {
        Gdiplus::GdiplusShutdown( g_gdip_token );
    }
    if ( !g_subsystem_failures.count( SMP_SCINTILLA ) )
    {
        Scintilla_ReleaseResources();
    }
    if ( !g_subsystem_failures.count( SMP_OLE ) )
    {
        OleUninitialize();
    }
}

class js_initquit : public initquit
{
public:
    void on_init() override
    {
        // HACK: popup_message services will not be initialized soon after start.
        check_error();
        delay_loader::g_set_ready();
    }

    void on_quit() override
    {
        mozjs::JsEngine::GetInstance().PrepareForExit();
        smp::panel::panel_manager::instance().send_msg_to_all( static_cast<UINT>(smp::InternalMessage::terminate_script), 0, 0 );
        simple_thread_pool::instance().join();
    }

private:
    void check_error()
    {
        if ( g_subsystem_failures.empty() )
        {
            return;
        }

        std::stringstream ss;
        ss << "Spider Monkey Panel initialization failed! The component will not function properly! Failures:\n\n";

        for ( const auto& [key, failure] : g_subsystem_failures )
        {
            ss << failure.description << ": error code 0x" << std::hex << failure.errorCode << "\n";
        }

        popup_msg::g_show( ss.str().c_str(), SMP_NAME );
    }
};

initquit_factory_t<js_initquit> g_initquit;

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
    }

    return TRUE;
}
