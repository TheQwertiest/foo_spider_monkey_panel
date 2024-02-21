#include <stdafx.h>

#include "ui_popup.h"

#include <graphics/gdi/object_selector.h>
#include <utils/gdi_helpers.h>

#include <qwr/pfc_helpers_ui.h>

namespace smp::ui
{

CPopup::CPopup( qwr::u8string_view message, qwr::u8string_view caption )
    : message_( message )
    , caption_( caption )
{
}

LRESULT CPopup::OnInitDialog( HWND /*hwndFocus*/, LPARAM /*lParam*/ )
{
    DlgResize_Init( false );

    AdjustToFit();

    // Disable manual resizing
    SetWindowLong( GWL_STYLE, GetWindowLong( GWL_STYLE ) & ~WS_THICKFRAME );

    uSetWindowText( m_hWnd, caption_.c_str() );
    uSendDlgItemMessageText( m_hWnd, IDC_LTEXT_MESSAGE, WM_SETTEXT, 0, message_.c_str() );

    CenterWindow();

    return FALSE;
}

LRESULT CPopup::OnCommand( UINT /*codeNotify*/, int id, HWND /*hwndCtl*/ )
{
    if ( id == IDOK || id == IDCANCEL )
    {
        EndDialog( id );
    }

    return 0;
}

void CPopup::AdjustToFit()
{
    if ( message_.empty() )
    {
        return;
    }

    CStatic promptCtrl( GetDlgItem( IDC_LTEXT_MESSAGE ) );
    CRect promptRc{};
    promptCtrl.GetClientRect( &promptRc );

    CRect newPromptRc = promptRc;
    {
        CPaintDC paintDc( m_hWnd );
        CDCHandle cdc( paintDc.m_hDC );

        GdiObjectSelector autoFont( cdc, promptCtrl.GetFont() );

        const auto promptW = qwr::unicode::ToWide( message_ );
        cdc.DrawText( const_cast<wchar_t*>( promptW.c_str() ), -1, &newPromptRc, DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK );
    }

    CButton okCtrl( GetDlgItem( IDOK ) );
    CRect okRc{};
    okCtrl.GetClientRect( &okRc );

    newPromptRc.bottom += okRc.Height();

    if ( newPromptRc.bottom > promptRc.bottom || newPromptRc.right > promptRc.right )
    {
        WINDOWPLACEMENT placement{};
        placement.length = sizeof( WINDOWPLACEMENT );
        if ( GetWindowPlacement( &placement ) )
        {
            if ( newPromptRc.bottom > promptRc.bottom )
            {
                placement.rcNormalPosition.bottom += newPromptRc.bottom - promptRc.bottom;
            }
            if ( newPromptRc.right > promptRc.right )
            {
                placement.rcNormalPosition.right += newPromptRc.right - promptRc.right;
            }
            SetWindowPlacement( &placement );
        }
    }
}

} // namespace smp::ui
