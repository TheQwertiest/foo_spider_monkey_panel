// Original source and docs: http://www.morearty.com/code/dispatch
// Copyright (c) 2000-2004 Mike Morearty <mike@morearty.com>
// See THIRD_PARTY_NOTICES.md for full license text.

//-----------------------------------------------------------------------------
//
// CDispatchPtr helper class
//
// Usage of this class:
//
// CDispatchPtr is a wrapper for an IDispatch pointer.  It inherits from, and
// thus shares capabilities with, the C runtime's IDispatchPtr class.
//
// What CDispatchPtr adds is member functions to make it very easy to get/set
// properties and invoke methods via IDispatch.  Some examples:
//
//		CDispatchPtr htmlDoc; // assume this points to an IE HTML document
//
//		_bstr_t title = htmlDoc.Get("title");
//		htmlDoc.Set("title", "new title");
//		htmldoc.Get("body").Get("firstChild").Invoke(
//						"insertAdjacentText", "afterBegin", "hello world");
//		_bstr_t html = htmldoc.Get("body").Get("innerHTML");
//		long height = htmldoc.Get("body").Get("clientHeight");
//		CDispatchPtr body = htmlDoc.Get("body");
//-----------------------------------------------------------------------------

#pragma once

#include <comdef.h>

class CDispatchVariant;

template <class Derived>
class CDispatchFunctions
{
public:
    // Throughout this class, for any function which is templated on type
    // <DispatchItem>, the DispatchItem identifies one method or property
    // of an object, and the value passed for the DispatchItem should be
    // either a (char*), a (wchar_t*), or a DISPID.

    // Get: get a property's value
    template <class DispatchItem, typename... Args>
    CDispatchVariant Get( DispatchItem property, const Args&... args );

    // Set: set a property's value
    template <class DispatchItem>
    void Set( DispatchItem property, const _variant_t& value );

    // SetRef: set a reference to a property's value
    template <class DispatchItem>
    void SetRef( DispatchItem property, const _variant_t& value );

    // Invoke: invoke a method
    template <class DispatchItem, typename... Args>
    CDispatchVariant Invoke( DispatchItem method, const Args&... args );

protected:
    template <class DispatchItem, typename... Args>
    CDispatchVariant InvokeWrapper( DispatchItem method, WORD invokeType, const Args&... args );

    void InvokeHelper( DISPID dispatchItem,
                       const VARIANT* params,
                       UINT cParams,
                       WORD invokeType,
                       VARIANT* result )
    {
        IDispatch* disp = *(Derived*)this;
        DISPPARAMS dispparams = { const_cast<VARIANT*>( params ), 0, cParams, 0 };
        HRESULT hr;
        DISPID dispidSet;
        EXCEPINFO excepInfo;
        SecureZeroMemory( &excepInfo, sizeof( EXCEPINFO ) );

        if ( invokeType == DISPATCH_PROPERTYPUT || invokeType == DISPATCH_PROPERTYPUTREF )
        {
            dispidSet = DISPID_PROPERTYPUT;

            dispparams.cNamedArgs = 1;
            dispparams.rgdispidNamedArgs = &dispidSet;
        }

        // A hard-coded assumption that "result" does NOT already
        // contain a valid variant!
        if ( result )
            V_VT( result ) = VT_EMPTY;

        hr = disp->Invoke( dispatchItem, IID_NULL, LOCALE_SYSTEM_DEFAULT, invokeType, &dispparams, result, &excepInfo, NULL );
        if ( FAILED( hr ) )
        {
            if ( hr == DISP_E_EXCEPTION )
            {
                if ( excepInfo.wCode != 0 )
                    hr = _com_error::WCodeToHRESULT( excepInfo.wCode );
                else if ( excepInfo.scode != 0 )
                    hr = excepInfo.scode;
            }
            _com_raise_error( hr );
        }
    }

    // dispatchItem is (wchar_t*) -- convert it to a DISPID
    void InvokeHelper( LPCOLESTR dispatchItem,
                       const VARIANT* params,
                       UINT cParams,
                       WORD invokeType,
                       VARIANT* result )
    {
        IDispatch* disp = *(Derived*)this;
        if ( !disp )
            throw _com_error( E_POINTER );

        DISPID dispid;
        HRESULT hr = disp->GetIDsOfNames( IID_NULL, const_cast<LPOLESTR*>( &dispatchItem ), 1, LOCALE_SYSTEM_DEFAULT, &dispid );
        if ( FAILED( hr ) )
        {
            if ( hr == DISP_E_UNKNOWNNAME && invokeType == DISPATCH_PROPERTYGET )
            {
                if ( result )
                    V_VT( result ) = VT_EMPTY;
                return;
            }
            else
            {
                _com_raise_error( hr );
            }
        }

        // call the DISPID overload of InvokeHelper()
        InvokeHelper( dispid, params, cParams, invokeType, result );
    }

#ifndef _UNICODE
    // dispatchItem is an Ansi LPSTR  -- convert it to an LPOLESTR
    void InvokeHelper( LPCSTR dispatchItem,
                       const VARIANT* params,
                       UINT cParams,
                       WORD invokeType,
                       VARIANT* result )
    {
        OLECHAR nameBuff[256]; // try to avoid doing an allocation
        LPOLESTR wideName;

        int cch = lstrlen( dispatchItem ) + 1;
        if ( cch <= sizeof( nameBuff ) / sizeof( OLECHAR ) )
            wideName = nameBuff;
        else
        {
            // dispatch item name is longer than our fixed-size buffer; allocate.
            // Do NOT use alloca() [or ATL's A2W(), which uses alloca()], because
            // this is function may be inlined (although that's not likely), and
            // that could cause a stack overflow if this function is called from
            // within a loop

            wideName = new OLECHAR[cch]; // cch may be just a bit bigger than necessary
            if ( wideName == NULL )
                _com_raise_error( E_OUTOFMEMORY );
        }

        wideName[0] = '\0';
        MultiByteToWideChar( CP_ACP, 0, dispatchItem, -1, wideName, cch );

        // call the LPOLESTR overload of InvokeHelper()
        InvokeHelper( wideName, params, cParams, invokeType, result );

        if ( wideName != nameBuff )
            delete[] wideName;
    }
#endif
};

//-----------------------------------------------------------------------------
// CDispatchVariant - a helper class for internal use, usually not needed by
// users of this code, but possibly useful on occasion.  This subclasses
// _variant_t and provides it with the Get/Set/SetRef/Invoke methods.  It only
// makes sense to do that if the _variant_t contains, or can be cast to, an
// IDispatch pointer -- that is, V_VT(variant) == VT_DISPATCH.
//
// In the Get/Set/SetRef/Invoke methods from CDispatchFunctions, the expression
// "*(Derived*)this" is used where an (IDispatch*) is needed.  In the case
// of CDispatchVariant, that expression will succeed if the variant contains
// (or can be cast to) and (IDispatch*), and will throw a _com_error object
// (with appropriate error code) if not.
//-----------------------------------------------------------------------------

class CDispatchVariant : public _variant_t
    , public CDispatchFunctions<CDispatchVariant>
{
public:
    // constructors -- just copied (with slight modifications) from _variant_t

    CDispatchVariant() throw()
        : _variant_t()
    {
    }

    CDispatchVariant( const VARIANT& varSrc )
        : _variant_t( varSrc )
    {
    }
    CDispatchVariant( const VARIANT* pSrc )
        : _variant_t( pSrc )
    {
    }
    CDispatchVariant( const _variant_t& varSrc )
        : _variant_t( varSrc )
    {
    }

    CDispatchVariant( VARIANT& varSrc, bool fCopy )
        : _variant_t( varSrc, fCopy )
    {
    }

    CDispatchVariant( IDispatch* pSrc, bool fAddRef = true ) throw()
        : _variant_t( pSrc, fAddRef )
    {
    }
    CDispatchVariant( IUnknown* pSrc, bool fAddRef = true ) throw()
        : _variant_t( pSrc, fAddRef )
    {
    }

    // operator=() -- just copied (with slight modifications) from _variant_t

    _variant_t& operator=( const VARIANT& varSrc )
    {
        return _variant_t::operator=( varSrc );
    }
    _variant_t& operator=( const VARIANT* pSrc )
    {
        return _variant_t::operator=( pSrc );
    }
    _variant_t& operator=( const _variant_t& varSrc )
    {
        return _variant_t::operator=( varSrc );
    }

    _variant_t& operator=( IDispatch* pSrc )
    {
        return _variant_t::operator=( pSrc );
    }
    _variant_t& operator=( IUnknown* pSrc )
    {
        return _variant_t::operator=( pSrc );
    }

    // operator->()

    IDispatch* operator->() const
    {
        return (IDispatch*)*this;
    }

    // operator bool()

    operator bool() const
    {
        if ( vt == VT_DISPATCH )
            return ( pdispVal != NULL );
        else
            return _variant_t::operator bool();
    }
};

//-----------------------------------------------------------------------------
// CDispatchPtr - the main class
//
// Use this class wherever you would have used IDispatchPtr.  Then, to get/set
// properties and invoke methods, use the Get, Set, and Invoke members
// inherited from CDispatchFunctions.
//-----------------------------------------------------------------------------

class CDispatchPtr
    : public IDispatchPtr
    , public CDispatchFunctions<CDispatchPtr>
{
public:
    // constructors -- just copied (with slight modifications) from _com_ptr_t

    template <typename _InterfacePtr>
    CDispatchPtr( const _InterfacePtr& p )
        : IDispatchPtr( p )
    {
    }
    CDispatchPtr() throw()
    {
    }
    CDispatchPtr( int null )
        : IDispatchPtr( null )
    {
    }
    CDispatchPtr( Interface* pInterface ) throw()
        : IDispatchPtr( pInterface )
    {
    }
    CDispatchPtr( Interface* pInterface, bool fAddRef ) throw()
        : IDispatchPtr( pInterface, fAddRef )
    {
    }
    explicit CDispatchPtr( const CLSID& clsid, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL )
        : IDispatchPtr( clsid, pOuter, dwClsContext )
    {
    }
    explicit CDispatchPtr( LPOLESTR str, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL )
        : IDispatchPtr( str, pOuter, dwClsContext )
    {
    }
    explicit CDispatchPtr( LPCSTR str, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL )
        : IDispatchPtr( str, pOuter, dwClsContext )
    {
    }

    // operator=() -- just copied (with slight modifications) from _com_ptr_t

    template <typename _InterfacePtr>
    CDispatchPtr& operator=( const _InterfacePtr& p )
    {
        IDispatchPtr::operator=( p );
        return *this;
    }
    CDispatchPtr& operator=( Interface* pInterface ) throw()
    {
        IDispatchPtr::operator=( pInterface );
        return *this;
    }
    CDispatchPtr& operator=( int null )
    {
        IDispatchPtr::operator=( null );
        return *this;
    }
};

//-----------------------------------------------------------------------------
// Inline functions from CDispatchFunctions
//-----------------------------------------------------------------------------

// Get: get a property's value
template <class Derived>
template <class DispatchItem, typename... Args>
CDispatchVariant CDispatchFunctions<Derived>::Get( DispatchItem property, const Args&... args )
{
    return InvokeWrapper( property, DISPATCH_PROPERTYGET, args... );
}

// Set: set a property's value
template <class Derived>
template <class DispatchItem>
void CDispatchFunctions<Derived>::Set( DispatchItem property, const _variant_t& value )
{
    InvokeHelper( property, &value, 1, DISPATCH_PROPERTYPUT, NULL );
}

// SetRef: set a reference to a property's value
template <class Derived>
template <class DispatchItem>
void CDispatchFunctions<Derived>::SetRef( DispatchItem property, const _variant_t& value )
{
    InvokeHelper( property, &value, 1, DISPATCH_PROPERTYPUTREF, NULL );
}

// Invoke: invoke a method
template <class Derived>
template <class DispatchItem, typename... Args>
CDispatchVariant CDispatchFunctions<Derived>::Invoke( DispatchItem method, const Args&... args )
{
    return InvokeWrapper( method, DISPATCH_METHOD, args... );
}

template <class Derived>
template <class DispatchItem, typename... Args>
CDispatchVariant CDispatchFunctions<Derived>::InvokeWrapper( DispatchItem method, WORD invokeType, const Args&... args )
{
    constexpr size_t argCount = sizeof...( Args );

    VARIANT result;
    if constexpr ( !argCount )
    {
        InvokeHelper( method, nullptr, argCount, invokeType, &result );
    }
    else
    {
        VARIANT args_v[argCount];
        { // Reverse assign
            auto it = std::rbegin( args_v );
            ( ( *it++ = args ), ... );
        }

        InvokeHelper( method, args_v, argCount, invokeType, &result );
    }
    return result;
}
