#include "stdafx.h"
#include "com_array.h"


namespace helpers
{

	bool com_array_reader::convert(VARIANT * pVarSrc)
	{
		reset();

		if (!pVarSrc) return false;

		if ((pVarSrc->vt & VT_ARRAY) && pVarSrc->parray)
		{
			return (SUCCEEDED(SafeArrayCopy(pVarSrc->parray, &m_psa)));
		}
		else if ((pVarSrc->vt & VT_TYPEMASK) == VT_DISPATCH)
		{
			IDispatch * pdisp = pVarSrc->pdispVal;

			if (pVarSrc->vt & VT_BYREF)
			{
				pdisp = *(pVarSrc->ppdispVal);
			}

			if (pdisp)
			{
				// {3EEF9758-35FC-11D1-8CE4-00C04FC2B092}
				//const GUID  guid_array_instance = 
				//{ 0x3eef9758, 0x35fc, 0x11d1, { 0x8c, 0xe4, 0x00, 0xc0, 0x4f, 0xc2, 0xb0, 0x92 } };

				return convert_jsarray(pdisp);
			}
		}

		return false;
	}

	void com_array_reader::calc_bound()
	{
		if (FAILED(SafeArrayGetLBound(m_psa, 1, &m_lbound)))
			goto error_get_bound;

		if (FAILED(SafeArrayGetUBound(m_psa, 1, &m_ubound)))
			goto error_get_bound;

		return;

error_get_bound:
		m_ubound = -1;
		m_lbound = 0;
	}

	bool com_array_reader::convert_jsarray(IDispatch * pdisp)
	{
		if (!pdisp) return false;

		DISPPARAMS params = {0};
		_variant_t ret;

		DISPID id_length;
		LPOLESTR slength = L"length";

		if (FAILED(pdisp->GetIDsOfNames(IID_NULL, &slength, 1, LOCALE_USER_DEFAULT, &id_length)))
			return false;

		if (FAILED(pdisp->Invoke(id_length, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
			&params, &ret, NULL, NULL)))
			return false;

		if (FAILED(VariantChangeType(&ret, &ret, 0, VT_I4)))
			return false;

		m_lbound = 0;
		m_ubound = ret.lVal - 1;

		SAFEARRAY * psa = SafeArrayCreateVector(VT_VARIANT, 0, get_count());

		if (!psa) goto cleanup_and_return;

		for (long i = m_lbound; i <= m_ubound; ++i)
		{
			DISPID dispid = 0;
			DISPPARAMS params = {0};
			wchar_t buf[33];
			LPOLESTR name = buf;
			_variant_t element;
			HRESULT hr = S_OK;

			_itow_s(i, buf, 10);

			if (SUCCEEDED(hr)) hr = pdisp->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
			if (SUCCEEDED(hr)) hr = pdisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, 
				&params, &element, NULL, NULL);

			if (FAILED(hr)) goto cleanup_and_return;
			if (FAILED(SafeArrayPutElement(psa, &i, &element))) goto cleanup_and_return;
		}

		m_psa = psa;
		return true;

cleanup_and_return:
		reset();
		SafeArrayDestroy(psa); 
		return false;
	}
}
