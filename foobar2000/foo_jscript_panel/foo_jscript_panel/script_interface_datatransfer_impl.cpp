#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_datatransfer_impl.h"


struct NameToValueTable
{
	const wchar_t * name;
	unsigned value;
};

_COM_SMARTPTR_TYPEDEF(IDragSourceHelper, IID_IDragSourceHelper);


DataTransferObject::DataTransferObject() : m_dropEffect(DROPEFFECT_MOVE), m_effectAllowed(~0)
{

}

STDMETHODIMP DataTransferObject::get_DropEffect(BSTR * outDropEffect)
{
	TRACK_FUNCTION();

	if (!outDropEffect) return E_POINTER;

	switch (m_dropEffect) 
	{
	case DROPEFFECT_COPY:
		*outDropEffect = SysAllocString(L"copy");
		break;
	case DROPEFFECT_LINK:
		*outDropEffect = SysAllocString(L"link");
		break;
	case DROPEFFECT_MOVE:
		*outDropEffect = SysAllocString(L"move");
		break;
	default:
		*outDropEffect = SysAllocString(L"none");
		m_dropEffect = DROPEFFECT_NONE;
		break;
	}

	return S_OK;
}

STDMETHODIMP DataTransferObject::put_DropEffect(BSTR dropEffect)
{
	TRACK_FUNCTION();

	if (!dropEffect) return E_INVALIDARG;

	const NameToValueTable dropEffectTable[] = 
	{
		{ L"copy", DROPEFFECT_COPY },
		{ L"link", DROPEFFECT_LINK },
		{ L"move", DROPEFFECT_MOVE },
		{ L"none", DROPEFFECT_NONE },
	};

	for (int i = 0; i < _countof(dropEffectTable); i++)
	{
		if (wcscmp(dropEffectTable[i].name, dropEffect) == 0)
		{
			m_dropEffect = dropEffectTable[i].value;
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

STDMETHODIMP DataTransferObject::get_EffectAllowed(BSTR * outEffectAllowed)
{
	TRACK_FUNCTION();

	if (!outEffectAllowed) return E_POINTER;

	switch (m_effectAllowed)
	{
	case DROPEFFECT_COPY:
		*outEffectAllowed = SysAllocString(L"copy");
		break;
	case DROPEFFECT_LINK:
		*outEffectAllowed = SysAllocString(L"link");
		break;
	case DROPEFFECT_MOVE:
		*outEffectAllowed = SysAllocString(L"move");
		break;
	case DROPEFFECT_COPY | DROPEFFECT_LINK:
		*outEffectAllowed = SysAllocString(L"copyLink");
		break;
	case DROPEFFECT_COPY | DROPEFFECT_MOVE:
		*outEffectAllowed = SysAllocString(L"copyMove");
		break;
	case DROPEFFECT_LINK | DROPEFFECT_MOVE:
		*outEffectAllowed = SysAllocString(L"linkMove");
		break;
	case ~0:
		*outEffectAllowed = SysAllocString(L"all");
		break;
	case DROPEFFECT_NONE:
	default:
		*outEffectAllowed = SysAllocString(L"none");
		break;
	}

	return S_OK;
}

STDMETHODIMP DataTransferObject::put_EffectAllowed(BSTR effectAllowed)
{
	TRACK_FUNCTION();

	if (!effectAllowed) return E_INVALIDARG;

	const NameToValueTable effectAllowedTable[] = {
		{ L"copy", DROPEFFECT_COPY },
		{ L"link", DROPEFFECT_LINK },
		{ L"move", DROPEFFECT_MOVE },
		{ L"copyLink", DROPEFFECT_COPY | DROPEFFECT_LINK },
		{ L"copyMove", DROPEFFECT_COPY | DROPEFFECT_MOVE },
		{ L"linkMove", DROPEFFECT_LINK | DROPEFFECT_MOVE },
		{ L"all", ~0},
		{ L"none", DROPEFFECT_NONE},
	};

	for (int i = 0; i < _countof(effectAllowedTable); i++)
	{
		if (wcscmp(effectAllowedTable[i].name, effectAllowed) == 0)
		{
			m_effectAllowed = effectAllowedTable[i].value;
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

STDMETHODIMP DataTransferObject::get_Types(VARIANT * outTypes)
{
	TRACK_FUNCTION();

	if(!outTypes) return E_POINTER;
	return S_OK;
}

STDMETHODIMP DataTransferObject::ClearData(BSTR type)
{
	TRACK_FUNCTION();

	if (!type) return E_INVALIDARG;
	return S_OK;
}

STDMETHODIMP DataTransferObject::SetData(BSTR type, BSTR data)
{
	TRACK_FUNCTION();

	if (!type || !data) return E_INVALIDARG;
	return S_OK;
}

STDMETHODIMP DataTransferObject::GetData(BSTR type, VARIANT * outData)
{
	TRACK_FUNCTION();

	if (!outData) return E_POINTER;
	if (!type) return E_INVALIDARG;
	return S_OK;
}

STDMETHODIMP DataTransferObject::GetMetadbHandles(IDispatch * callback)
{
	TRACK_FUNCTION();

	if (!callback) return E_INVALIDARG;

	//callback->Invoke(DISPID_VALUE)

	return S_OK;
}

STDMETHODIMP DataTransferObject::SetDragImage(__interface IGdiBitmap * bitmap, int x, int y)
{
	TRACK_FUNCTION();

	if (!bitmap) return E_INVALIDARG;

	IDragSourceHelperPtr dragSourceHelper;
	if (dragSourceHelper.CreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER) && dragSourceHelper)
	{
		Gdiplus::Bitmap * pBitmap = NULL;
		HBITMAP hBitmap = NULL;
		bitmap->get__ptr((void**)&pBitmap);
		if (!pBitmap) return E_INVALIDARG;

		pBitmap->GetHBITMAP(Gdiplus::Color::White, &hBitmap);
		if (!hBitmap) return E_INVALIDARG;

		SHDRAGIMAGE sdi;
		sdi.crColorKey          = 0xffffffff;
		sdi.hbmpDragImage       = hBitmap;
		sdi.ptOffset.x          = x;
		sdi.ptOffset.y          = y;
		sdi.sizeDragImage.cx    = pBitmap->GetWidth();
		sdi.sizeDragImage.cy    = pBitmap->GetHeight();

		dragSourceHelper->InitializeFromBitmap(&sdi, m_dataObject);
	}

	return S_OK;
}
