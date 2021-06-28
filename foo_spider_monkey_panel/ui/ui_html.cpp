#include <stdafx.h>

#include "ui_html.h"

#include <com_objects/dispatch_ptr.h>
#include <convert/com.h>
#include <convert/js_to_native.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_property_helper.h>

#include <qwr/fb2k_paths.h>
#include <qwr/final_action.h>
#include <qwr/hook_handler.h>
#include <qwr/winapi_error_helpers.h>

namespace smp::ui
{
using namespace mozjs;

CDialogHtml::CDialogHtml( JSContext* cx, const std::wstring& htmlCodeOrPath, JS::HandleValue options )
    : pJsCtx_( cx )
    , htmlCodeOrPath_( htmlCodeOrPath )
{
    ParseOptions( options );
}

CDialogHtml::~CDialogHtml()
{
    if ( hIcon_ )
    {
        DestroyIcon( hIcon_ );
    }
}

LRESULT CDialogHtml::OnInitDialog( HWND, LPARAM )
{
    qwr::final_action autoExit( [&] {
        EndDialog( -1 );
    } );

    SetOptions();

    HWND hIE = static_cast<HWND>( GetDlgItem( IDC_IE ) );
    try
    {
        CAxWindow wndIE = hIE;

        IObjectWithSitePtr pOWS = nullptr;
        HRESULT hr = wndIE.QueryHost( IID_IObjectWithSite, reinterpret_cast<void**>( &pOWS ) );
        qwr::error::CheckHR( hr, "QueryHost" );

        hr = pOWS->SetSite( static_cast<IServiceProvider*>( this ) );
        qwr::error::CheckHR( hr, "SetSite" );

        IWebBrowserPtr pBrowser;
        hr = wndIE.QueryControl( &pBrowser );
        qwr::error::CheckHR( hr, "QueryControl" );

        _variant_t v;
        hr = pBrowser->Navigate( _bstr_t( L"about:blank" ), &v, &v, &v, &v ); ///< Document object is only available after Navigate
        qwr::error::CheckHR( hr, "Navigate" );

        IDispatchPtr pDocDispatch;
        hr = pBrowser->get_Document( &pDocDispatch );
        qwr::error::CheckHR( hr, "get_Document" );

        IHTMLDocument2Ptr pDocument = pDocDispatch;

        {
            // Request default handler from MSHTML client site
            IOleObjectPtr pOleObject( pDocument );
            IOleClientSitePtr pClientSite;
            hr = pOleObject->GetClientSite( &pClientSite );
            qwr::error::CheckHR( hr, "GetClientSite" );

            pDefaultUiHandler_ = pClientSite;

            // Set the new custom IDocHostUIHandler
            ICustomDocPtr pCustomDoc( pDocument );
            hr = pCustomDoc->SetUIHandler( this );
            qwr::error::CheckHR( hr, "SetUIHandler" );
        }

        {
            pOleInPlaceHandler_ = pBrowser;
        }

        if ( static_cast<std::wstring_view>( htmlCodeOrPath_ ).starts_with( L"file://" ) )
        {
            hr = pBrowser->Navigate( _bstr_t( htmlCodeOrPath_.c_str() ), &v, &v, &v, &v );
            qwr::error::CheckHR( hr, "Navigate" );
        }
        else
        {
            hr = pDocument->put_designMode( _bstr_t( L"on" ) );
            qwr::error::CheckHR( hr, "put_designMode" );

            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            qwr::final_action autoPsa( [pSaStrings]() {
                SafeArrayDestroy( pSaStrings );
            } );

            VARIANT* pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, reinterpret_cast<void**>( &pSaVar ) );
            qwr::error::CheckHR( hr, "SafeArrayAccessData" );

            _bstr_t bstr( htmlCodeOrPath_.c_str() );
            pSaVar->vt = VT_BSTR;
            pSaVar->bstrVal = bstr.Detach();
            hr = SafeArrayUnaccessData( pSaStrings );
            qwr::error::CheckHR( hr, "SafeArrayUnaccessData" );

            hr = pDocument->write( pSaStrings );
            qwr::error::CheckHR( hr, "write" );

            hr = pDocument->put_designMode( _bstr_t( L"off" ) );
            qwr::error::CheckHR( hr, "put_designMode" );

            hr = pDocument->close();
            qwr::error::CheckHR( hr, "close" );
        }

        wndIE.SetFocus();
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        return -1;
    }

    hookId_ = qwr::HookHandler::GetInstance().RegisterHook(
        [hIE, pThis = this]( int code, WPARAM wParam, LPARAM lParam ) {
            GetMsgProc( code, wParam, lParam, hIE, pThis );
        } );

    autoExit.cancel();
    return FALSE; // don't set focus to default control
}

LRESULT CDialogHtml::OnDestroyDialog()
{
    if ( hookId_ )
    {
        qwr::HookHandler::GetInstance().UnregisterHook( hookId_ );
        hookId_ = 0;
    }

    return 0;
}

void CDialogHtml::OnSize( UINT nType, CSize size )
{
    switch ( nType )
    {
    case SIZE_MAXIMIZED:
    case SIZE_RESTORED:
    {
        CAxWindow wndIE = static_cast<HWND>( GetDlgItem( IDC_IE ) );
        wndIE.ResizeClient( size.cx, size.cy );
        break;
    }
    default:
        break;
    }
}

void CDialogHtml::OnClose()
{
    isClosing_ = true;
    OnCloseCmd( 0, IDCANCEL, nullptr );
}

void CDialogHtml::OnCloseCmd( WORD, WORD wID, HWND )
{
    if ( !isClosing_ )
    { // e.g. pressed RETURN
        return;
    }

    EndDialog( wID );
}

void CDialogHtml::OnBeforeNavigate2( IDispatch*, VARIANT* URL, VARIANT*,
                                     VARIANT*, VARIANT*, VARIANT*,
                                     VARIANT_BOOL* Cancel )
{
    if ( !Cancel || !URL )
    {
        return;
    }

    *Cancel = VARIANT_FALSE;

    try
    {
        _bstr_t url_b( *URL );
        for ( const auto& urlPrefix: std::initializer_list<std::wstring>{ L"http://", L"https://" } )
        {
            if ( url_b.length() > urlPrefix.length()
                 && !wmemcmp( url_b.GetBSTR(), urlPrefix.c_str(), urlPrefix.length() ) )
            {
                if ( Cancel )
                {
                    *Cancel = VARIANT_TRUE;
                    return;
                }
            }
        }
    }
    catch ( const _com_error& )
    {
    }
}

void CDialogHtml::OnTitleChange( BSTR title )
{
    try
    {
        SetWindowText( static_cast<wchar_t*>( static_cast<_bstr_t>( title ) ) );
    }
    catch ( const _com_error& )
    {
    }
}

void __stdcall CDialogHtml::OnWindowClosing( VARIANT_BOOL, VARIANT_BOOL* Cancel )
{
    EndDialog( IDOK );
    if ( Cancel )
    {
        *Cancel = VARIANT_TRUE;
        return;
    }
}

STDMETHODIMP CDialogHtml::moveTo( LONG x, LONG y )
{
    if ( RECT rect; GetWindowRect( &rect ) )
    {
        MoveWindow( x, y, ( rect.right - rect.left ), ( rect.bottom - rect.top ) );
    }
    return S_OK;
}

STDMETHODIMP CDialogHtml::moveBy( LONG x, LONG y )
{
    if ( RECT rect; GetWindowRect( &rect ) )
    {
        MoveWindow( rect.left + x, rect.top + y, ( rect.right - rect.left ), ( rect.bottom - rect.top ) );
    }
    return S_OK;
}

STDMETHODIMP CDialogHtml::resizeTo( LONG x, LONG y )
{
    if ( RECT windowRect, clientRect; GetWindowRect( &windowRect ) && GetClientRect( &clientRect ) )
    {
        const LONG clientW = x - ( ( windowRect.right - windowRect.left ) - clientRect.right );
        const LONG clientH = y - ( ( windowRect.bottom - windowRect.top ) - clientRect.bottom );
        ResizeClient( clientW, clientH );
    }
    return S_OK;
}

STDMETHODIMP CDialogHtml::resizeBy( LONG x, LONG y )
{
    if ( RECT clientRect; GetClientRect( &clientRect ) )
    {
        const LONG clientW = x + clientRect.right;
        const LONG clientH = y + clientRect.bottom;
        ResizeClient( clientW, clientH );
    }

    return S_OK;
}

STDMETHODIMP CDialogHtml::ShowContextMenu( DWORD dwID, POINT* ppt, IUnknown* pcmdtReserved, IDispatch* pdispReserved )
{
    if ( dwID == CONTEXT_MENU_TEXTSELECT || dwID == CONTEXT_MENU_CONTROL )
    { // always show context menu for text editors
        return S_FALSE;
    }

    if ( !pDefaultUiHandler_ || !isContextMenuEnabled_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->ShowContextMenu( dwID, ppt, pcmdtReserved, pdispReserved );
}

STDMETHODIMP CDialogHtml::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    HRESULT hr = pDefaultUiHandler_->GetHostInfo( pInfo );
    if ( SUCCEEDED( hr ) && pInfo )
    {
        if ( !isFormSelectionEnabled_ )
        {
            pInfo->dwFlags |= DOCHOSTUIFLAG_DIALOG;
        }
        if ( !isScrollEnabled_ )
        {
            pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
        }
    }

    return hr;
}

STDMETHODIMP CDialogHtml::ShowUI( DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->ShowUI( dwID, pActiveObject, pCommandTarget, pFrame, pDoc );
}

STDMETHODIMP CDialogHtml::HideUI()
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->HideUI();
}

STDMETHODIMP CDialogHtml::UpdateUI()
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->UpdateUI();
}

STDMETHODIMP CDialogHtml::EnableModeless( BOOL fEnable )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->EnableModeless( fEnable );
}

STDMETHODIMP CDialogHtml::OnDocWindowActivate( BOOL fActivate )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->OnDocWindowActivate( fActivate );
}

STDMETHODIMP CDialogHtml::OnFrameWindowActivate( BOOL fActivate )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->OnFrameWindowActivate( fActivate );
}

STDMETHODIMP CDialogHtml::ResizeBorder( LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fRameWindow )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->ResizeBorder( prcBorder, pUIWindow, fRameWindow );
}

STDMETHODIMP CDialogHtml::TranslateAccelerator( LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    auto isSupportedHotKey = []( UINT wm, UINT vk ) -> bool {
        if ( wm != WM_KEYDOWN && wm != WM_KEYUP
             && wm != WM_SYSKEYDOWN && wm != WM_SYSKEYUP )
        {
            return false;
        }

        const bool isCtrlPressed = HIBYTE( GetKeyState( VK_CONTROL ) );
        const bool isShiftPressed = HIBYTE( GetKeyState( VK_SHIFT ) );
        const bool isAltPressed = HIBYTE( GetKeyState( VK_MENU ) );

        constexpr std::array allowedCtrlKeys{
            0x41, // A
            0x43, // C
            0x56, // V
            0x58, // X
            0x59, // Y
            0x5A  // Z
        };
        return ( isCtrlPressed && !isShiftPressed && !isAltPressed
                 && std::cend( allowedCtrlKeys ) != ranges::find( allowedCtrlKeys, vk ) );
    };

    if ( isSupportedHotKey( lpMsg->message, lpMsg->wParam ) )
    {
        return pDefaultUiHandler_->TranslateAccelerator( lpMsg, pguidCmdGroup, nCmdID );
    }
    else
    {
        if ( WM_KEYDOWN == lpMsg->message && VK_ESCAPE == lpMsg->wParam )
        { // Restore default dialog behaviour
            isClosing_ = true;
        }
        return S_FALSE;
    }
}

STDMETHODIMP CDialogHtml::GetOptionKeyPath( LPOLESTR* pchKey, DWORD dw )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->GetOptionKeyPath( pchKey, dw );
}

STDMETHODIMP CDialogHtml::GetDropTarget( IDropTarget* pDropTarget, IDropTarget** ppDropTarget )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->GetDropTarget( pDropTarget, ppDropTarget );
}

STDMETHODIMP CDialogHtml::GetExternal( IDispatch** ppDispatch )
{
    assert( ppDispatch );
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    if ( ppDispatch && pExternal_ )
    {
        pExternal_.AddRef();
        *ppDispatch = pExternal_;
        return S_OK;
    }

    return pDefaultUiHandler_->GetExternal( ppDispatch );
}

STDMETHODIMP CDialogHtml::TranslateUrl( DWORD dwTranslate, LPWSTR pchURLIn, LPWSTR* ppchURLOut )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->TranslateUrl( dwTranslate, pchURLIn, ppchURLOut );
}

STDMETHODIMP CDialogHtml::FilterDataObject( IDataObject* pDO, IDataObject** ppDORet )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->FilterDataObject( pDO, ppDORet );
}

ULONG STDMETHODCALLTYPE CDialogHtml::AddRef()
{
    return 0;
}

ULONG STDMETHODCALLTYPE CDialogHtml::Release()
{
    return 0;
}

void CDialogHtml::ParseOptions( JS::HandleValue options )
{
    assert( pJsCtx_ );

    if ( options.isNullOrUndefined() )
    {
        return;
    }

    if ( !options.isObject() )
    {
        throw qwr::QwrException( "options argument is not an object" );
    }

    JS::RootedObject jsObject( pJsCtx_, &options.toObject() );

    width_ = GetOptionalProperty<uint32_t>( pJsCtx_, jsObject, "width" );
    height_ = GetOptionalProperty<uint32_t>( pJsCtx_, jsObject, "height" );
    x_ = GetOptionalProperty<int32_t>( pJsCtx_, jsObject, "x" );
    y_ = GetOptionalProperty<int32_t>( pJsCtx_, jsObject, "y" );
    isCentered_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "center" ).value_or( true );
    isContextMenuEnabled_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "context_menu" ).value_or( false );
    isFormSelectionEnabled_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "selection" ).value_or( false );
    isResizable_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "resizable" ).value_or( false );
    isScrollEnabled_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "scroll" ).value_or( false );

    bool hasProperty;
    if ( !JS_HasProperty( pJsCtx_, jsObject, "data", &hasProperty ) )
    {
        throw JsException();
    }

    if ( hasProperty )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        if ( !JS_GetProperty( pJsCtx_, jsObject, "data", &jsValue ) )
        {
            throw JsException();
        }

        _variant_t data;
        convert::com::JsToVariant( pJsCtx_, jsValue, *data.GetAddress() );

        pExternal_.Attach( new com::ComPtrImpl<com::HostExternal>( data ) );
    }
}

void CDialogHtml::SetOptions()
{
    if ( !isResizable_ )
    {
        ModifyStyle( WS_THICKFRAME, WS_BORDER, SWP_FRAMECHANGED ); ///< ignore return value, since we don't really care
    }

    if ( width_ || height_ )
    {
        resizeTo( width_.value_or( 250 ), height_.value_or( 100 ) ); ///< ignore return value
    }

    // Center only after we know the size
    if ( isCentered_ )
    {
        (void)CenterWindow();
    }

    if ( x_ || y_ )
    {
        moveTo( x_.value_or( 0 ), y_.value_or( 0 ) ); ///< ignore return value
    }

    {
        const auto w_fb2k_path = qwr::path::Foobar2000() / L"foobar2000.exe";
        SHFILEINFO shfi{};
        SHGetFileInfo( w_fb2k_path.c_str(), 0, &shfi, sizeof( SHFILEINFO ), SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_SMALLICON );
        if ( shfi.hIcon )
        {
            hIcon_ = shfi.hIcon;
            SetIcon( hIcon_, FALSE );
        }
    }
}

void CDialogHtml::GetMsgProc( int, WPARAM, LPARAM lParam, HWND hParent, CDialogHtml* pParent )
{
    if ( auto pMsg = reinterpret_cast<LPMSG>( lParam );
         pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST )
    { // Only react to keypress events
        for ( HWND tmpHwnd = pMsg->hwnd;
              tmpHwnd && ( ::GetWindowLong( tmpHwnd, GWL_STYLE ) & WS_CHILD );
              tmpHwnd = ::GetParent( tmpHwnd ) )
        {
            if ( tmpHwnd == hParent )
            {
                CDialogHtml* pThis = pParent;
                if ( pThis && pThis->pOleInPlaceHandler_ && S_OK == pThis->pOleInPlaceHandler_->TranslateAccelerator( pMsg ) )
                {
                    pMsg->message = WM_NULL;
                }
                break;
            }
        }
    }
}

} // namespace smp::ui
