#include "stdafx.h"
#include "com_tools.h"


bool name_to_id_cache::lookup(hash_type hash, DISPID* p_dispid) const
{
	DISPID dispId;
	if (!m_map.query(hash, dispId)) return false;
	(*p_dispid) = dispId;
	return true;
}

void name_to_id_cache::add(hash_type hash, DISPID dispid)
{
	m_map[hash] = dispid;
}

name_to_id_cache::hash_type name_to_id_cache::g_hash(const wchar_t* name)
{
	return LHashValOfName(LANG_NEUTRAL, name);
}


void type_info_cache_holder::init_from_typelib(ITypeLib * p_typeLib, const GUID & guid)
{
	p_typeLib->GetTypeInfoOfGuid(guid, &m_type_info);
}

HRESULT type_info_cache_holder::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
	if (empty()) return E_UNEXPECTED;
	if (!ppTInfo) return E_POINTER;
	if (iTInfo != 0) return DISP_E_BADINDEX;
	m_type_info->AddRef();
	(*ppTInfo) = m_type_info.GetInterfacePtr();
	return S_OK;
}

HRESULT type_info_cache_holder::GetIDsOfNames(LPOLESTR *rgszNames, UINT cNames, MEMBERID *pMemId)
{
	PFC_ASSERT(m_type_info != NULL);
	HRESULT hr = S_OK;
	for (unsigned i = 0; i < cNames && SUCCEEDED(hr); ++i) 
	{
		auto hash = name_to_id_cache::g_hash(rgszNames[i]);
		if (!m_cache.lookup(hash, &pMemId[i])) 
		{
			hr = m_type_info->GetIDsOfNames(&rgszNames[i], 1, &pMemId[i]);
			if (SUCCEEDED(hr)) 
			{
				m_cache.add(hash, pMemId[i]);
			}
		}
	}

	return hr;
}

HRESULT type_info_cache_holder::Invoke(PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	PFC_ASSERT(m_type_info != NULL);
	HRESULT hr = m_type_info->Invoke(pvInstance, memid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	PFC_ASSERT(hr != RPC_E_WRONG_THREAD);
	return hr;
}
