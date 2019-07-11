#pragma once

#include <com_objects/com_interface.h>

#include <unordered_map>

// TODO: cleanup

namespace internal
{

class type_info_cache_holder
{
public:
    type_info_cache_holder();

    bool empty() throw();

    void init_from_typelib( ITypeLib* p_typeLib, const GUID& guid );

    // "Expose" some ITypeInfo related methods here
    HRESULT GetTypeInfo( UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo );
    HRESULT GetIDsOfNames( LPOLESTR* rgszNames, UINT cNames, MEMBERID* pMemId );
    HRESULT Invoke( PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr );

protected:
    std::unordered_map<ULONG, DISPID> m_cache;
    ITypeInfoPtr m_type_info;
};

} // namespace internal

extern ITypeLibPtr g_typelib;

//-- IUnknown ---
#define BEGIN_COM_QI_IMPL() \
	public:\
		STDMETHOD(QueryInterface)(REFIID riid, void ** ppv) { \
			if (!ppv) return E_INVALIDARG; \

// C2594: ambiguous conversions
#define COM_QI_ENTRY_MULTI(Ibase, Iimpl) \
		if (riid == __uuidof(Ibase)) { \
			*ppv = static_cast<Ibase *>(static_cast<Iimpl *>(this)); \
			goto qi_entry_done; \
		}

#define COM_QI_ENTRY(Iimpl) \
			COM_QI_ENTRY_MULTI(Iimpl, Iimpl);

#define END_COM_QI_IMPL() \
			*ppv = NULL; \
			return E_NOINTERFACE; \
		qi_entry_done: \
			reinterpret_cast<IUnknown*>(*ppv)->AddRef(); \
			return S_OK; \
		}	

//-- IDispatch --
template <class T>
class MyIDispatchImpl : public T
{
protected:
	static ::internal::type_info_cache_holder g_type_info_cache_holder;

	MyIDispatchImpl<T>()
	{
		if (g_type_info_cache_holder.empty() && g_typelib)
		{
			g_type_info_cache_holder.init_from_typelib(g_typelib, __uuidof(T));
		}
	}

    virtual ~MyIDispatchImpl<T>() = default;

	virtual void FinalRelease()
	{
	}

public:
	STDMETHOD(GetTypeInfoCount)(unsigned int* n)
	{
		if (!n) return E_INVALIDARG;
		*n = 1;
		return S_OK;
	}

	STDMETHOD(GetTypeInfo)(unsigned int i, LCID lcid, ITypeInfo** pp)
	{
		return g_type_info_cache_holder.GetTypeInfo(i, lcid, pp);
	}

	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** names, unsigned int cnames, LCID lcid, DISPID* dispids)
	{
		if (g_type_info_cache_holder.empty()) return E_UNEXPECTED;
		return g_type_info_cache_holder.GetIDsOfNames(names, cnames, dispids);
	}

	STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flag, DISPPARAMS* params, VARIANT* result, EXCEPINFO* excep, unsigned int* err)
	{
		if (g_type_info_cache_holder.empty()) return E_UNEXPECTED;
		return g_type_info_cache_holder.Invoke(this, dispid, flag, params, result, excep, err);
	}
};

template <class T> 
__declspec( selectany ) ::internal::type_info_cache_holder MyIDispatchImpl<T>::g_type_info_cache_holder;

//-- IDispatch impl -- [T] [IDispatch] [IUnknown]
template <class T>
class IDispatchImpl3 : public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
    IDispatchImpl3<T>() = default;
    virtual ~IDispatchImpl3<T>() = default;
};

//-- IDisposable impl -- [T] [IDisposable] [IDispatch] [IUnknown]
template <class T>
class IDisposableImpl4 : public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IDisposable)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
    IDisposableImpl4<T>() = default;
    virtual ~IDisposableImpl4() = default;

public:
	STDMETHODIMP Dispose()
	{
		this->FinalRelease();
		return S_OK;
	}
};

template <typename _Base, bool _AddRef = true>
class com_object_impl_t : public _Base
{
public:
    template <typename... Args>
    com_object_impl_t( Args&&... args )
        : _Base( std::forward<Args>( args )... )
    {
        if constexpr ( _AddRef )
        {
            ++m_dwRef;
        }
    }

	STDMETHODIMP_(ULONG) AddRef()
	{
		return ++m_dwRef;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		const ULONG n = --m_dwRef;
		if (!n)
		{
			this->FinalRelease();
			delete this;
		}
		return n;
	}

private:
    ~com_object_impl_t() override = default;

private:
    std::atomic<ULONG> m_dwRef = 0;
};
