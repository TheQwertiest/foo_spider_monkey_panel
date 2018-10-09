#pragma once

#include "script_interface_impl.h"

#include <OleIdl.h>
#include <ShlObj.h>
#include <ObjSafe.h>

namespace smp::com
{

class IScriptableWebBrowser2
    : public IWebBrowser2
    , public IObjectSafety
{
    BEGIN_COM_QI_IMPL()
    COM_QI_ENTRY_MULTI( IUnknown, IWebBrowser2 )
    COM_QI_ENTRY_MULTI( IDispatch, IWebBrowser2 )
    COM_QI_ENTRY( IWebBrowser2 )
    COM_QI_ENTRY( IObjectSafety )
    END_COM_QI_IMPL()

public:
    IScriptableWebBrowser2( IWebBrowser2Ptr pBrowser )
        : pBrowser_( pBrowser )
    {
    }

    void FinalRelease(){};

    // IObjectSafety implementation
    STDMETHODIMP GetInterfaceSafetyOptions( REFIID riid,
                                            DWORD* pdwSupportedOptions,
                                            DWORD* pdwEnabledOptions )
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions = *pdwSupportedOptions;
        return S_OK;
    }
    STDMETHODIMP SetInterfaceSafetyOptions( REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions )
    {
        return S_OK;
    }

    // IWebBrowser2
    virtual STDMETHODIMP Navigate2(
        VARIANT* URL,
        VARIANT* Flags,
        VARIANT* TargetFrameName,
        VARIANT* PostData,
        VARIANT* Headers )
    {
        return pBrowser_->Navigate2( URL, Flags, TargetFrameName, PostData, Headers );
    };

    virtual STDMETHODIMP QueryStatusWB(
        OLECMDID cmdID,
        OLECMDF* pcmdf )
    {
        return pBrowser_->QueryStatusWB( cmdID, pcmdf );
    };

    virtual STDMETHODIMP ExecWB(
        OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt,
        VARIANT* pvaIn,
        VARIANT* pvaOut )
    {
        return pBrowser_->ExecWB( cmdID, cmdexecopt, pvaIn, pvaOut );
    };

    virtual STDMETHODIMP ShowBrowserBar(
        VARIANT* pvaClsid,
        VARIANT* pvarShow,
        VARIANT* pvarSize )
    {
        return pBrowser_->ShowBrowserBar( pvaClsid, pvarShow, pvarSize );
    };

    virtual STDMETHODIMP get_ReadyState(
        READYSTATE* plReadyState )
    {
        return pBrowser_->get_ReadyState( plReadyState );
    }

    virtual STDMETHODIMP get_Offline(
        VARIANT_BOOL* pbOffline )
    {
        return pBrowser_->get_Offline( pbOffline );
    }

    virtual STDMETHODIMP put_Offline(
        VARIANT_BOOL bOffline )
    {
        return pBrowser_->put_Offline( bOffline );
    }

    virtual STDMETHODIMP get_Silent(
        VARIANT_BOOL* pbSilent )
    {
        return pBrowser_->get_Silent( pbSilent );
    }

    virtual STDMETHODIMP put_Silent(
        VARIANT_BOOL bSilent )
    {
        return pBrowser_->put_Silent( bSilent );
    }

    virtual STDMETHODIMP get_RegisterAsBrowser(
        VARIANT_BOOL* pbRegister )
    {
        return pBrowser_->get_RegisterAsBrowser( pbRegister );
    }

    virtual STDMETHODIMP put_RegisterAsBrowser(
        VARIANT_BOOL bRegister )
    {
        return pBrowser_->put_RegisterAsBrowser( bRegister );
    }

    virtual STDMETHODIMP get_RegisterAsDropTarget(
        VARIANT_BOOL* pbRegister )
    {
        return pBrowser_->get_RegisterAsDropTarget( pbRegister );
    }

    virtual STDMETHODIMP put_RegisterAsDropTarget(
        VARIANT_BOOL bRegister )
    {
        return pBrowser_->put_RegisterAsDropTarget( bRegister );
    }

    virtual STDMETHODIMP get_TheaterMode(
        VARIANT_BOOL* pbRegister )
    {
        return pBrowser_->get_TheaterMode( pbRegister );
    }

    virtual STDMETHODIMP put_TheaterMode(
        VARIANT_BOOL bRegister )
    {
        return pBrowser_->put_TheaterMode( bRegister );
    }

    virtual STDMETHODIMP get_AddressBar(
        VARIANT_BOOL* Value )
    {
        return pBrowser_->get_AddressBar( Value );
    }

    virtual STDMETHODIMP put_AddressBar(
        VARIANT_BOOL Value )
    {
        return pBrowser_->put_AddressBar( Value );
    }

    virtual STDMETHODIMP get_Resizable(
        VARIANT_BOOL* Value )
    {
        return pBrowser_->get_Resizable( Value );
    }

    virtual STDMETHODIMP put_Resizable(
        VARIANT_BOOL Value )
    {
        return pBrowser_->put_Resizable( Value );
    }

    // IWebBrowserApp

    virtual STDMETHODIMP Quit( void )
    {
        return pBrowser_->Quit();
    }

    virtual STDMETHODIMP ClientToWindow(
        int* pcx,
        int* pcy )
    {
        return pBrowser_->ClientToWindow(
            pcx,
            pcy );
    }

    virtual STDMETHODIMP PutProperty(
        BSTR Property,
        VARIANT vtValue )
    {
        return pBrowser_->PutProperty(
            Property,
            vtValue );
    }

    virtual STDMETHODIMP GetProperty(
        BSTR Property,
        VARIANT* pvtValue )
    {
        return pBrowser_->GetProperty(
            Property,
            pvtValue );
    }

    virtual STDMETHODIMP get_Name(
        BSTR* Name )
    {
        return pBrowser_->get_Name(
            Name );
    }

    virtual STDMETHODIMP get_HWND(
        SHANDLE_PTR* pHWND )
    {
        return pBrowser_->get_HWND(
            pHWND );
    }

    virtual STDMETHODIMP get_FullName(
        BSTR* FullName )
    {
        return pBrowser_->get_FullName(
            FullName );
    }

    virtual STDMETHODIMP get_Path(
        BSTR* Path )
    {
        return pBrowser_->get_Path(
            Path );
    }

    virtual STDMETHODIMP get_Visible(
        VARIANT_BOOL* pBool )
    {
        return pBrowser_->get_Visible(
            pBool );
    }

    virtual STDMETHODIMP put_Visible(
        VARIANT_BOOL Value )
    {
        return pBrowser_->put_Visible(
            Value );
    }

    virtual STDMETHODIMP get_StatusBar(
        VARIANT_BOOL* pBool )
    {
        return pBrowser_->get_StatusBar(
            pBool );
    }

    virtual STDMETHODIMP put_StatusBar(
        VARIANT_BOOL Value )
    {
        return pBrowser_->put_StatusBar(
            Value );
    }

    virtual STDMETHODIMP get_StatusText(
        BSTR* StatusText )
    {
        return pBrowser_->get_StatusText(
            StatusText );
    }

    virtual STDMETHODIMP put_StatusText(
        BSTR StatusText )
    {
        return pBrowser_->put_StatusText(
            StatusText );
    }

    virtual STDMETHODIMP get_ToolBar(
        int* Value )
    {
        return pBrowser_->get_ToolBar(
            Value );
    }

    virtual STDMETHODIMP put_ToolBar(
        int Value )
    {
        return pBrowser_->put_ToolBar(
            Value );
    }

    virtual STDMETHODIMP get_MenuBar(
        VARIANT_BOOL* Value )
    {
        return pBrowser_->get_MenuBar(
            Value );
    }

    virtual STDMETHODIMP put_MenuBar(
        VARIANT_BOOL Value )
    {
        return pBrowser_->put_MenuBar(
            Value );
    }

    virtual STDMETHODIMP get_FullScreen(
        VARIANT_BOOL* pbFullScreen )
    {
        return pBrowser_->get_FullScreen(
            pbFullScreen );
    }

    virtual STDMETHODIMP put_FullScreen(
        VARIANT_BOOL bFullScreen )
    {
        return pBrowser_->put_FullScreen(
            bFullScreen );
    }

    // IWebBrowser

    virtual STDMETHODIMP GoBack( void )
    {
        return pBrowser_->GoBack();
    }

    virtual STDMETHODIMP GoForward( void )
    {
        return pBrowser_->GoForward();
    }

    virtual STDMETHODIMP GoHome( void )
    {
        return pBrowser_->GoHome();
    }

    virtual STDMETHODIMP GoSearch( void )
    {
        return pBrowser_->GoSearch();
    }

    virtual STDMETHODIMP Navigate(
        BSTR URL,
        VARIANT* Flags,
        VARIANT* TargetFrameName,
        VARIANT* PostData,
        VARIANT* Headers )
    {
        return pBrowser_->Navigate(
            URL,
            Flags,
            TargetFrameName,
            PostData,
            Headers );
    }

    virtual STDMETHODIMP Refresh( void )
    {
        return pBrowser_->Refresh();
    }

    virtual STDMETHODIMP Refresh2(
        VARIANT* Level )
    {
        return pBrowser_->Refresh2(
            Level );
    }

    virtual STDMETHODIMP Stop( void )
    {
        return pBrowser_->Stop();
    }

    virtual STDMETHODIMP get_Application(
        IDispatch** ppDisp )
    {
        return pBrowser_->get_Application(
            ppDisp );
    }

    virtual STDMETHODIMP get_Parent(
        IDispatch** ppDisp )
    {
        return pBrowser_->get_Parent(
            ppDisp );
    }

    virtual STDMETHODIMP get_Container(
        IDispatch** ppDisp )
    {
        return pBrowser_->get_Container(
            ppDisp );
    }

    virtual STDMETHODIMP get_Document(
        IDispatch** ppDisp )
    {
        return pBrowser_->get_Document(
            ppDisp );
    }

    virtual STDMETHODIMP get_TopLevelContainer(
        VARIANT_BOOL* pBool )
    {
        return pBrowser_->get_TopLevelContainer(
            pBool );
    }

    virtual STDMETHODIMP get_Type(
        BSTR* Type )
    {
        return pBrowser_->get_Type(
            Type );
    }

    virtual STDMETHODIMP get_Left(
        long* pl )
    {
        return pBrowser_->get_Left(
            pl );
    }

    virtual STDMETHODIMP put_Left(
        long Left )
    {
        return pBrowser_->put_Left(
            Left );
    }

    virtual STDMETHODIMP get_Top(
        long* pl )
    {
        return pBrowser_->get_Top(
            pl );
    }

    virtual STDMETHODIMP put_Top(
        long Top )
    {
        return pBrowser_->put_Top(
            Top );
    }

    virtual STDMETHODIMP get_Width(
        long* pl )
    {
        return pBrowser_->get_Width(
            pl );
    }

    virtual STDMETHODIMP put_Width(
        long Width )
    {
        return pBrowser_->put_Width(
            Width );
    }

    virtual STDMETHODIMP get_Height(
        long* pl )
    {
        return pBrowser_->get_Height(
            pl );
    }

    virtual STDMETHODIMP put_Height(
        long Height )
    {
        return pBrowser_->put_Height(
            Height );
    }

    virtual STDMETHODIMP get_LocationName(
        BSTR* LocationName )
    {
        return pBrowser_->get_LocationName(
            LocationName );
    }

    virtual STDMETHODIMP get_LocationURL(
        BSTR* LocationURL )
    {
        return pBrowser_->get_LocationURL(
            LocationURL );
    }

    virtual STDMETHODIMP get_Busy(
        VARIANT_BOOL* pBool )
    {
        return pBrowser_->get_Busy(
            pBool );
    }

    virtual STDMETHODIMP GetTypeInfoCount(
        UINT* pctinfo )
    {
        return pBrowser_->GetTypeInfoCount(
            pctinfo );
    }

    virtual STDMETHODIMP GetTypeInfo(
        UINT iTInfo,
        LCID lcid,
        ITypeInfo** ppTInfo )
    {
        return pBrowser_->GetTypeInfo(
            iTInfo, lcid, ppTInfo );
    }

    virtual STDMETHODIMP GetIDsOfNames(
        REFIID riid,
        LPOLESTR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID* rgDispId )
    {
        return pBrowser_->GetIDsOfNames(
            riid, rgszNames, cNames, lcid, rgDispId );
    }

    virtual STDMETHODIMP Invoke(

        DISPID dispIdMember,

        REFIID riid,

        LCID lcid,
        WORD wFlags,

        DISPPARAMS* pDispParams,

        VARIANT* pVarResult,

        EXCEPINFO* pExcepInfo,

        UINT* puArgErr )
    {
        return pBrowser_->Invoke( dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
    }

private:
    IWebBrowser2Ptr pBrowser_;
};

} // namespace smp::com
