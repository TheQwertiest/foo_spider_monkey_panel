#pragma once

#include "dbgtrace.h"


//-- IUnknown ---
#define BEGIN_COM_QI_IMPL() \
	public:\
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv) { \
			if (!ppv) return E_INVALIDARG; \

// C2594: ambiguous conversions
#define COM_QI_ENTRY_MULTI(Ibase, Iimpl) \
		if (riid == __uuidof(Ibase)) { \
			*ppv = static_cast<Ibase *>(static_cast<Iimpl *>(this)); \
			goto qi_entry_done; \
		}

#define COM_QI_ENTRY(Iimpl) \
			COM_QI_ENTRY_MULTI(Iimpl, Iimpl);

#define COM_QI_ENTRY_COND(Iimpl, cond) \
			if (cond) COM_QI_ENTRY_MULTI(Iimpl, Iimpl);

#define END_COM_QI_IMPL() \
			*ppv = NULL; \
			return E_NOINTERFACE; \
		qi_entry_done: \
			reinterpret_cast<IUnknown*>(*ppv)->AddRef(); \
			return S_OK; \
		} \
	private:


class name_to_id_cache 
{
public:
	typedef ULONG hash_type;

	bool lookup(hash_type hash, DISPID* p_dispid) const;
	void add(hash_type hash, DISPID dispid);
	static hash_type g_hash(const wchar_t* name);

protected:
	typedef pfc::map_t<hash_type, DISPID> name_to_id_map;
	name_to_id_map m_map;
};

class type_info_cache_holder
{
public:
	type_info_cache_holder() : m_type_info(NULL)
	{
	}

	inline void set_type_info(ITypeInfo * type_info)
	{
		m_type_info = type_info;
	}

	inline bool valid() throw()
	{
		return m_type_info != NULL;
	}

	inline bool empty() throw()
	{
		return m_type_info == NULL;
	}

	inline ITypeInfo * get_ptr() throw()
	{
		return m_type_info;
	}

	void init_from_typelib(ITypeLib * p_typeLib, const GUID & guid);

public:
	// "Expose" some ITypeInfo related methods here
	HRESULT GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
	HRESULT GetIDsOfNames(LPOLESTR *rgszNames, UINT cNames, MEMBERID *pMemId);
	HRESULT Invoke(PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

protected:
	name_to_id_cache m_cache;
	ITypeInfoPtr m_type_info;
};

//-- IDispatch --
template<class T>
class MyIDispatchImpl: public T
{
protected:
	static type_info_cache_holder g_type_info_cache_holder;

	MyIDispatchImpl<T>()
	{
		if (g_type_info_cache_holder.empty() && g_typelib)
		{
			g_type_info_cache_holder.init_from_typelib(g_typelib, __uuidof(T));	
		}
	}

	virtual ~MyIDispatchImpl<T>()
	{
	}

	virtual void FinalRelease()
	{
	}

public:
	STDMETHOD(GetTypeInfoCount)(unsigned int * n)
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
		TRACK_THIS_DISPATCH_CALL(g_type_info_cache_holder.get_ptr(), dispid, flag);
		return g_type_info_cache_holder.Invoke(this, dispid, flag, params, result, excep, err);
	}
};

template<class T>
FOOGUIDDECL type_info_cache_holder MyIDispatchImpl<T>::g_type_info_cache_holder;


//-- IDispatch impl -- [T] [IDispatch] [IUnknown]
template<class T>
class IDispatchImpl3: public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
	IDispatchImpl3<T>() {}

	virtual ~IDispatchImpl3<T>() {}
};

//-- IDisposable impl -- [T] [IDisposable] [IDispatch] [IUnknown]
template<class T>
class IDisposableImpl4: public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IDisposable)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
	IDisposableImpl4<T>() {}

	virtual ~IDisposableImpl4() { }

public:
	STDMETHODIMP Dispose()
	{
		FinalRelease();
		return S_OK;
	}
};

template <typename _Base, bool _AddRef = true>
class com_object_impl_t :  public _Base
{
private:
	volatile LONG m_dwRef;

	inline ULONG AddRef_()
	{
		return InterlockedIncrement(&m_dwRef);
	}

	inline ULONG Release_()
	{
		return InterlockedDecrement(&m_dwRef);
	}

	inline void Construct_()
	{
		m_dwRef = 0; 
		if (_AddRef)
			AddRef_();
	}

	virtual ~com_object_impl_t()
	{
	}

public:
	STDMETHODIMP_(ULONG) AddRef()
	{
		return AddRef_();
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG n = Release_();
		if (n == 0)
		{
			FinalRelease();
			delete this;
		}
		return n;
	}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(com_object_impl_t, _Base, { Construct_(); })
};

template <class T>
class com_object_singleton_t
{
public:
	static inline T * instance()
	{
		if (!_instance)
		{
			insync(_cs);

			if (!_instance)
			{
				_instance = new com_object_impl_t<T, false>();
			}
		}

		return reinterpret_cast<T *>(_instance.GetInterfacePtr());
	}

private:
	static IDispatchPtr _instance;
	static critical_section _cs;

	PFC_CLASS_NOT_COPYABLE_EX(com_object_singleton_t)
};

template <class T>
FOOGUIDDECL IDispatchPtr com_object_singleton_t<T>::_instance;

template <class T>
FOOGUIDDECL critical_section com_object_singleton_t<T>::_cs;
