#include <stdafx.h>

#include "ui_menu_edit_with.h"

#include <fb2k/config.h>
#include <graphics/gdi/object_selector.h>
#include <utils/app_info.h>
#include <utils/gdi_helpers.h>
#include <utils/logging.h>

#include <qwr/error_popup.h>
#include <qwr/file_helpers.h>

namespace
{

std::vector<smp::AppInfo> g_CachedEditors;

}

namespace smp::ui
{

BOOL CMenuEditWith::ProcessWindowMessage( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _Inout_ LRESULT& lResult, _In_ DWORD dwMsgMapID /*= 0 */ )
{
    // so that we can access the parent control
    lParam = (LPARAM)hWnd;

    // BEGIN_MSG_MAP
    BOOL bHandled = TRUE;
    ( hWnd );
    ( uMsg );
    ( wParam );
    ( lParam );
    ( lResult );
    ( bHandled );
    switch ( dwMsgMapID )
    {
    case 0:
        // COMMAND_HANDLER_EX
        if ( ( uMsg == WM_COMMAND ) && ( BN_CLICKED == HIWORD( wParam ) ) && ( ID_EDIT_WITH_EXTERNAL == LOWORD( wParam ) ) )
        {
            OnEditClick( (UINT)HIWORD( wParam ), (int)LOWORD( wParam ), (HWND)lParam );
            lResult = 0;
            return TRUE;
        }
        // COMMAND_HANDLER_EX
        if ( ( uMsg == WM_COMMAND ) && ( BN_CLICKED == HIWORD( wParam ) ) && ( ID_EDIT_WITH_INTERNAL == LOWORD( wParam ) ) )
        {
            OnEditClick( (UINT)HIWORD( wParam ), (int)LOWORD( wParam ), (HWND)lParam );
            lResult = 0;
            return TRUE;
        }
        // COMMAND_RANGE_CODE_HANDLER_EX
        if ( ( uMsg == WM_COMMAND ) && ( BN_CLICKED == HIWORD( wParam ) ) && ( LOWORD( wParam ) >= ID_EDIT_WITH_EXTERNAL_IDX_START ) && ( LOWORD( wParam ) <= ID_EDIT_WITH_EXTERNAL_IDX_START + 100 ) )
        {
            OnEditClick( (UINT)HIWORD( wParam ), (int)LOWORD( wParam ), (HWND)lParam );
            lResult = 0;
            return TRUE;
        }
        // END_MSG_MAP
        break;
    default:
        ATLTRACE( static_cast<int>( ATL::atlTraceWindowing ), 0, _T("Invalid message map ID (%i)\n"), dwMsgMapID );
        ATLASSERT( FALSE );
        break;
    }
    return FALSE;
}

void CMenuEditWith::InitMenu()
{
    if ( g_CachedEditors.empty() )
    {
        try
        {
            g_CachedEditors = GetAppsAssociatedWithExtension( L".js" );
        }
        catch ( const qwr::QwrException& e )
        {
            smp::utils::LogError( e.what() );
        }
    }

    currentBmps_.resize( 1000 ); // preallocate to avoid dtors
    if ( !g_CachedEditors.empty() )
    {
        CClientDC clientDc{ nullptr };
        CDC memDc{ ::CreateCompatibleDC( clientDc ) };
        auto idx = ID_EDIT_WITH_EXTERNAL_IDX_START;
        for ( const auto& editor: g_CachedEditors )
        {
            AppendMenu( MF_BYPOSITION, idx, editor.appName.c_str() );

            // TODO: fix icon transparency
            auto icon = GetAppIcon( editor.appPath );
            if ( icon )
            {
                const auto iconW = GetSystemMetrics( SM_CXMENUCHECK );
                const auto iconH = GetSystemMetrics( SM_CYMENUCHECK );

                CBitmap memBmp{ ::CreateCompatibleBitmap( clientDc, iconW, iconH ) };
                GdiObjectSelector autoBmp( memDc, memBmp.m_hBitmap );
                CBrush brush{ ::CreateSolidBrush( GetSysColor( COLOR_MENU ) ) };

                RECT rc{ 0, 0, iconW, iconH };
                memDc.FillRect( &rc, brush );
                memDc.DrawIconEx( rc.left, rc.top, icon, rc.right, rc.bottom, 0, nullptr, DI_NORMAL );

                HBITMAP hBmp = memBmp.Detach();
                currentBmps_.emplace_back( hBmp );

                SetMenuItemBitmaps( idx, MF_BYCOMMAND, hBmp, hBmp );
            }

            ++idx;
        }
        AppendMenu( MF_SEPARATOR );
    }
    AppendMenu( MF_BYPOSITION, ID_EDIT_WITH_INTERNAL, L"Internal editor" );
    AppendMenu( MF_SEPARATOR );
    AppendMenu( MF_BYPOSITION, ID_EDIT_WITH_EXTERNAL, L"Edit with..." );
}

void CMenuEditWith::OnEditClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    namespace fs = std::filesystem;

    switch ( nID )
    {
    case ID_EDIT_WITH_EXTERNAL:
    {
        try
        {
            qwr::file::FileDialogOptions fdOpts{};
            fdOpts.filterSpec.assign( { { L"Executable files", L"*.exe" } } );
            fdOpts.defaultExtension = L"exe";

            const auto editorPathOpt = qwr::file::FileDialog( L"Choose text editor", false, fdOpts );
            if ( editorPathOpt )
            {
                const fs::path editorPath = *editorPathOpt;
                qwr::QwrException::ExpectTrue( fs::exists( editorPath ), "Invalid path" );

                smp::config::default_editor = editorPath.u8string();
            }
        }
        catch ( const fs::filesystem_error& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
        break;
    }
    case ID_EDIT_WITH_INTERNAL:
    {
        smp::config::default_editor = "";
        break;
    }
    default:
    {
        if ( nID < ID_EDIT_WITH_EXTERNAL_IDX_START )
        {
            return;
        }

        const size_t idx = nID - ID_EDIT_WITH_EXTERNAL_IDX_START;
        assert( idx < g_CachedEditors.size() );

        const auto& editor = g_CachedEditors[idx];
        smp::config::default_editor = editor.appPath.u8string();

        break;
    }
    }

    wndCtl.SendMessage( WM_COMMAND, ( BN_CLICKED << 16 ) | ID_EDIT_WITH_MENU, NULL );
}

} // namespace smp::ui
