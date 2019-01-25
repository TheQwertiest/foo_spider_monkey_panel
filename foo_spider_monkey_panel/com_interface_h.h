

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 06:14:07 2038
 */
/* Compiler settings for com_objects\com_interface.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __com_interface_h_h__
#define __com_interface_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDisposable_FWD_DEFINED__
#define __IDisposable_FWD_DEFINED__
typedef interface IDisposable IDisposable;

#endif 	/* __IDisposable_FWD_DEFINED__ */


#ifndef __IWrappedJs_FWD_DEFINED__
#define __IWrappedJs_FWD_DEFINED__
typedef interface IWrappedJs IWrappedJs;

#endif 	/* __IWrappedJs_FWD_DEFINED__ */


#ifndef __IHostExternal_FWD_DEFINED__
#define __IHostExternal_FWD_DEFINED__
typedef interface IHostExternal IHostExternal;

#endif 	/* __IHostExternal_FWD_DEFINED__ */


/* header files for imported files */
#include "docobj.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __foo_spider_monkey_panel_LIBRARY_DEFINED__
#define __foo_spider_monkey_panel_LIBRARY_DEFINED__

/* library foo_spider_monkey_panel */
/* [uuid][version] */ 


EXTERN_C const IID LIBID_foo_spider_monkey_panel;

#ifndef __IDisposable_INTERFACE_DEFINED__
#define __IDisposable_INTERFACE_DEFINED__

/* interface IDisposable */
/* [uuid][custom][unique][object][dual] */ 


EXTERN_C const IID IID_IDisposable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e0bae19-3afe-473a-a703-0feb2d714655")
    IDisposable : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Dispose( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDisposableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDisposable * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDisposable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDisposable * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDisposable * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDisposable * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDisposable * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDisposable * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Dispose )( 
            IDisposable * This);
        
        END_INTERFACE
    } IDisposableVtbl;

    interface IDisposable
    {
        CONST_VTBL struct IDisposableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDisposable_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDisposable_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDisposable_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDisposable_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IDisposable_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IDisposable_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IDisposable_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IDisposable_Dispose(This)	\
    ( (This)->lpVtbl -> Dispose(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDisposable_INTERFACE_DEFINED__ */


#ifndef __IWrappedJs_INTERFACE_DEFINED__
#define __IWrappedJs_INTERFACE_DEFINED__

/* interface IWrappedJs */
/* [uuid][custom][unique][dual][object] */ 


EXTERN_C const IID IID_IWrappedJs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0A72A7F4-024C-4DAB-92BE-5F6853294E44")
    IWrappedJs : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecuteValue( 
            /* [optional] */ VARIANT arg1,
            /* [optional] */ VARIANT arg2,
            /* [optional] */ VARIANT arg3,
            /* [optional] */ VARIANT arg4,
            /* [optional] */ VARIANT arg5,
            /* [optional] */ VARIANT arg6,
            /* [optional] */ VARIANT arg7,
            /* [retval][out] */ VARIANT *result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IWrappedJsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWrappedJs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWrappedJs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWrappedJs * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWrappedJs * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWrappedJs * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWrappedJs * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWrappedJs * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteValue )( 
            IWrappedJs * This,
            /* [optional] */ VARIANT arg1,
            /* [optional] */ VARIANT arg2,
            /* [optional] */ VARIANT arg3,
            /* [optional] */ VARIANT arg4,
            /* [optional] */ VARIANT arg5,
            /* [optional] */ VARIANT arg6,
            /* [optional] */ VARIANT arg7,
            /* [retval][out] */ VARIANT *result);
        
        END_INTERFACE
    } IWrappedJsVtbl;

    interface IWrappedJs
    {
        CONST_VTBL struct IWrappedJsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWrappedJs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IWrappedJs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IWrappedJs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IWrappedJs_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IWrappedJs_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IWrappedJs_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IWrappedJs_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IWrappedJs_ExecuteValue(This,arg1,arg2,arg3,arg4,arg5,arg6,arg7,result)	\
    ( (This)->lpVtbl -> ExecuteValue(This,arg1,arg2,arg3,arg4,arg5,arg6,arg7,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IWrappedJs_INTERFACE_DEFINED__ */


#ifndef __IHostExternal_INTERFACE_DEFINED__
#define __IHostExternal_INTERFACE_DEFINED__

/* interface IHostExternal */
/* [uuid][custom][unique][dual][object] */ 


EXTERN_C const IID IID_IHostExternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98D9779A-4398-47D3-8CA4-B4C8BDE60526")
    IHostExternal : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_dialogArguments( 
            /* [retval][out] */ VARIANT *pData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostExternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostExternal * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostExternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostExternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IHostExternal * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IHostExternal * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IHostExternal * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IHostExternal * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_dialogArguments )( 
            IHostExternal * This,
            /* [retval][out] */ VARIANT *pData);
        
        END_INTERFACE
    } IHostExternalVtbl;

    interface IHostExternal
    {
        CONST_VTBL struct IHostExternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostExternal_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostExternal_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostExternal_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostExternal_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IHostExternal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IHostExternal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IHostExternal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IHostExternal_get_dialogArguments(This,pData)	\
    ( (This)->lpVtbl -> get_dialogArguments(This,pData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostExternal_INTERFACE_DEFINED__ */

#endif /* __foo_spider_monkey_panel_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


