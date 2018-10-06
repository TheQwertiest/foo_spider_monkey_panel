//-----------------------------------------------------------------------------
// Copyright (c) 2000-04 Mike Morearty <mike@morearty.com>
// Original source and docs: http://www.morearty.com/code/dispatch
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
	template <class DispatchItem>
	CDispatchVariant Get(DispatchItem property);

	template <class DispatchItem>
	CDispatchVariant Get(DispatchItem property,
							const _variant_t& arg1);

	template <class DispatchItem>
	CDispatchVariant Get(DispatchItem property,
							const _variant_t& arg1,
							const _variant_t& arg2);

	// Set: set a property's value
	template <class DispatchItem>
	void Set(DispatchItem property, const _variant_t& value);

	// SetRef: set a reference to a property's value
	template <class DispatchItem>
	void SetRef(DispatchItem property, const _variant_t& value);

	// Invoke: invoke a method
	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3,
							const _variant_t& arg4);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3,
							const _variant_t& arg4,
							const _variant_t& arg5);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3,
							const _variant_t& arg4,
							const _variant_t& arg5,
							const _variant_t& arg6);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3,
							const _variant_t& arg4,
							const _variant_t& arg5,
							const _variant_t& arg6,
							const _variant_t& arg7);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3,
							const _variant_t& arg4,
							const _variant_t& arg5,
							const _variant_t& arg6,
							const _variant_t& arg7,
							const _variant_t& arg8);

	template <class DispatchItem>
	CDispatchVariant Invoke(DispatchItem method,
							const _variant_t& arg1,
							const _variant_t& arg2,
							const _variant_t& arg3,
							const _variant_t& arg4,
							const _variant_t& arg5,
							const _variant_t& arg6,
							const _variant_t& arg7,
							const _variant_t& arg8,
							const _variant_t& arg9);

protected:
	void InvokeHelper(DISPID dispatchItem,
					  const VARIANT* params,
					  UINT cParams,
					  WORD invokeType,
					  VARIANT* result)
	{
		IDispatch* disp = *(Derived*)this;
		DISPPARAMS dispparams = { const_cast<VARIANT*>(params), 0, cParams, 0 };
		HRESULT hr;
		DISPID dispidSet;
		EXCEPINFO excepInfo;
		SecureZeroMemory(&excepInfo, sizeof(EXCEPINFO));

		if (invokeType == DISPATCH_PROPERTYPUT ||
			invokeType == DISPATCH_PROPERTYPUTREF)
		{
			dispidSet = DISPID_PROPERTYPUT;

			dispparams.cNamedArgs = 1;
			dispparams.rgdispidNamedArgs = &dispidSet;
		}

		// A hard-coded assumption that "result" does NOT already
		// contain a valid variant!
		if (result)
			V_VT(result) = VT_EMPTY;

		hr = disp->Invoke(dispatchItem, IID_NULL, LOCALE_SYSTEM_DEFAULT,
			invokeType, &dispparams, result, &excepInfo, NULL);
		if (FAILED(hr))
		{
			if (hr == DISP_E_EXCEPTION)
			{
				if (excepInfo.wCode != 0)
					hr = _com_error::WCodeToHRESULT(excepInfo.wCode);
				else if (excepInfo.scode != 0)
					hr = excepInfo.scode;
			}
			_com_raise_error(hr);
		}
	}

	// dispatchItem is (wchar_t*) -- convert it to a DISPID
	void InvokeHelper(LPCOLESTR dispatchItem,
					  const VARIANT* params,
                      UINT cParams,
					  WORD invokeType,
					  VARIANT* result)
	{
		IDispatch* disp = *(Derived*)this;
		if (!disp)
			throw _com_error(E_POINTER);

		DISPID dispid;
		HRESULT hr = disp->GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&dispatchItem), 1,
			LOCALE_SYSTEM_DEFAULT, &dispid);
		if (FAILED(hr))
		{
			if (hr == DISP_E_UNKNOWNNAME && invokeType == DISPATCH_PROPERTYGET)
			{
				if (result)
					V_VT(result) = VT_EMPTY;
				return;
			}
			else
			{
				_com_raise_error(hr);
			}
		}

		// call the DISPID overload of InvokeHelper()
		InvokeHelper(dispid, params, cParams, invokeType, result);
	}

#ifndef _UNICODE
	// dispatchItem is an Ansi LPSTR  -- convert it to an LPOLESTR
	void InvokeHelper(LPCSTR dispatchItem,
					  const VARIANT* params,
                      UINT cParams,
					  WORD invokeType,
					  VARIANT* result)
	{
		OLECHAR nameBuff[256]; // try to avoid doing an allocation
		LPOLESTR wideName;

		int cch = lstrlen(dispatchItem) + 1;
		if (cch <= sizeof(nameBuff) / sizeof(OLECHAR))
			wideName = nameBuff;
		else
		{
			// dispatch item name is longer than our fixed-size buffer; allocate.
			// Do NOT use alloca() [or ATL's A2W(), which uses alloca()], because
			// this is function may be inlined (although that's not likely), and
			// that could cause a stack overflow if this function is called from
			// within a loop

			wideName = new OLECHAR[cch]; // cch may be just a bit bigger than necessary
			if (wideName == NULL)
				_com_raise_error(E_OUTOFMEMORY);
		}

		wideName[0] = '\0';
		MultiByteToWideChar(CP_ACP, 0, dispatchItem, -1, wideName, cch);

		// call the LPOLESTR overload of InvokeHelper()
		InvokeHelper(wideName, params, cParams, invokeType, result);

		if (wideName != nameBuff)
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

class CDispatchVariant :
	public _variant_t,
	public CDispatchFunctions<CDispatchVariant>
{
public:
	// constructors -- just copied (with slight modifications) from _variant_t

	CDispatchVariant() throw() : _variant_t() { }

	CDispatchVariant(const VARIANT& varSrc) : _variant_t(varSrc) { }
	CDispatchVariant(const VARIANT* pSrc) : _variant_t(pSrc) { }
	CDispatchVariant(const _variant_t& varSrc) : _variant_t(varSrc) { }

	CDispatchVariant(VARIANT& varSrc, bool fCopy) : _variant_t(varSrc, fCopy) { }

	CDispatchVariant(IDispatch* pSrc, bool fAddRef = true) throw() : _variant_t(pSrc, fAddRef) { }
	CDispatchVariant(IUnknown* pSrc, bool fAddRef = true) throw() : _variant_t(pSrc, fAddRef) { }

	// operator=() -- just copied (with slight modifications) from _variant_t

	_variant_t& operator=(const VARIANT& varSrc)
		{ return _variant_t::operator=(varSrc); }
	_variant_t& operator=(const VARIANT* pSrc)
		{ return _variant_t::operator=(pSrc); }
	_variant_t& operator=(const _variant_t& varSrc)
		{ return _variant_t::operator=(varSrc); }

	_variant_t& operator=(IDispatch* pSrc)
		{ return _variant_t::operator=(pSrc); }
	_variant_t& operator=(IUnknown* pSrc)
		{ return _variant_t::operator=(pSrc); }

	// operator->()

	IDispatch* operator->() const
		{ return (IDispatch*)*this; }

	// operator bool()

	operator bool() const
	{
		if (vt == VT_DISPATCH)
			return (pdispVal != NULL);
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

class CDispatchPtr :
	public IDispatchPtr,
	public CDispatchFunctions<CDispatchPtr>
{
public:
	// constructors -- just copied (with slight modifications) from _com_ptr_t

	template<typename _InterfacePtr> CDispatchPtr(const _InterfacePtr& p) : IDispatchPtr(p) { }
	CDispatchPtr() throw() { }
	CDispatchPtr(int null) : IDispatchPtr(null) { }
	CDispatchPtr(Interface* pInterface) throw() : IDispatchPtr(pInterface) { }
	CDispatchPtr(Interface* pInterface, bool fAddRef) throw() : IDispatchPtr(pInterface, fAddRef) { }
	explicit CDispatchPtr(const CLSID& clsid, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
		: IDispatchPtr(clsid, pOuter, dwClsContext) { }
	explicit CDispatchPtr(LPOLESTR str, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
		: IDispatchPtr(str, pOuter, dwClsContext) { }
	explicit CDispatchPtr(LPCSTR str, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
		: IDispatchPtr(str, pOuter, dwClsContext) { }

	// operator=() -- just copied (with slight modifications) from _com_ptr_t

	template<typename _InterfacePtr> CDispatchPtr& operator=(const _InterfacePtr& p)
		{ IDispatchPtr::operator=(p); return *this; }
	CDispatchPtr& operator=(Interface* pInterface) throw()
		{ IDispatchPtr::operator=(pInterface); return *this; }
	CDispatchPtr& operator=(int null)
		{ IDispatchPtr::operator=(null); return *this; }
};


//-----------------------------------------------------------------------------
// Inline functions from CDispatchFunctions
//-----------------------------------------------------------------------------

// Get: get a property's value
template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Get(DispatchItem property)
{
	VARIANT result;
	InvokeHelper(property, NULL, 0, DISPATCH_PROPERTYGET, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Get(DispatchItem property,
						const _variant_t& arg1)
{
	VARIANT result;
	InvokeHelper(property, &arg1, 1, DISPATCH_PROPERTYGET, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Get(DispatchItem property,
						const _variant_t& arg1,
						const _variant_t& arg2)
{
	VARIANT result;
	VARIANT args[2];

	args[0] = arg2;
	args[1] = arg1;
	InvokeHelper(property, args, 2, DISPATCH_PROPERTYGET, &result);
	return result;
}

// Set: set a property's value
template <class Derived>
template <class DispatchItem>
void CDispatchFunctions<Derived>::Set(DispatchItem property, const _variant_t& value)
{
	InvokeHelper(property, &value, 1, DISPATCH_PROPERTYPUT, NULL);
}

// SetRef: set a reference to a property's value
template <class Derived>
template <class DispatchItem>
void CDispatchFunctions<Derived>::SetRef(DispatchItem property, const _variant_t& value)
{
	InvokeHelper(property, &value, 1, DISPATCH_PROPERTYPUTREF, NULL);
}

// Invoke: invoke a method
template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method)
{
	VARIANT result;
	InvokeHelper(method, NULL, 0, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1)
{
	VARIANT result;
	InvokeHelper(method, &arg1, 1, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2)
{
	VARIANT result;
	VARIANT args[2];

	args[0] = arg2;
	args[1] = arg1;

	InvokeHelper(method, args, 2, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3)
{
	VARIANT result;
	VARIANT args[3];

	args[0] = arg3;
	args[1] = arg2;
	args[2] = arg1;

	InvokeHelper(method, args, 3, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3,
						const _variant_t& arg4)
{
	VARIANT result;
	VARIANT args[4];

	args[0] = arg4;
	args[1] = arg3;
	args[2] = arg2;
	args[3] = arg1;

	InvokeHelper(method, args, 4, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3,
						const _variant_t& arg4,
						const _variant_t& arg5)
{
	VARIANT result;
	VARIANT args[5];

	args[0] = arg5;
	args[1] = arg4;
	args[2] = arg3;
	args[3] = arg2;
	args[4] = arg1;

	InvokeHelper(method, args, 5, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3,
						const _variant_t& arg4,
						const _variant_t& arg5,
						const _variant_t& arg6)
{
	VARIANT result;
	VARIANT args[6];

	args[0] = arg6;
	args[1] = arg5;
	args[2] = arg4;
	args[3] = arg3;
	args[4] = arg2;
	args[5] = arg1;

	InvokeHelper(method, args, 6, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3,
						const _variant_t& arg4,
						const _variant_t& arg5,
						const _variant_t& arg6,
						const _variant_t& arg7)
{
	VARIANT result;
	VARIANT args[7];

	args[0] = arg7;
	args[1] = arg6;
	args[2] = arg5;
	args[3] = arg4;
	args[4] = arg3;
	args[5] = arg2;
	args[6] = arg1;

	InvokeHelper(method, args, 7, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3,
						const _variant_t& arg4,
						const _variant_t& arg5,
						const _variant_t& arg6,
						const _variant_t& arg7,
						const _variant_t& arg8)
{
	VARIANT result;
	VARIANT args[8];

	args[0] = arg8;
	args[1] = arg7;
	args[2] = arg6;
	args[3] = arg5;
	args[4] = arg4;
	args[5] = arg3;
	args[6] = arg2;
	args[7] = arg1;

	InvokeHelper(method, args, 8, DISPATCH_METHOD, &result);
	return result;
}

template <class Derived>
template <class DispatchItem>
CDispatchVariant CDispatchFunctions<Derived>::Invoke(DispatchItem method,
						const _variant_t& arg1,
						const _variant_t& arg2,
						const _variant_t& arg3,
						const _variant_t& arg4,
						const _variant_t& arg5,
						const _variant_t& arg6,
						const _variant_t& arg7,
						const _variant_t& arg8,
						const _variant_t& arg9)
{
	VARIANT result;
	VARIANT args[9];

	args[0] = arg9;
	args[1] = arg8;
	args[2] = arg7;
	args[3] = arg6;
	args[4] = arg5;
	args[5] = arg4;
	args[6] = arg3;
	args[7] = arg2;
	args[8] = arg1;

	InvokeHelper(method, args, 9, DISPATCH_METHOD, &result);
	return result;
}
