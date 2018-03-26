#pragma once

#include "script_interface_impl.h"

#include <OleIdl.h>
#include <ShlObj.h>

class IDropSourceImpl : public IDropSource
{
public:
	IDropSourceImpl()
	{
		m_refCount = 0;
		m_dwLastEffect = DROPEFFECT_NONE;
	};

	virtual ~IDropSourceImpl()
	{
	};

	// IDropSource
	STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
	{
		if (fEscapePressed || (grfKeyState & MK_RBUTTON) || (grfKeyState & MK_MBUTTON))
		{
			return DRAGDROP_S_CANCEL;
		}

		if (!(grfKeyState & MK_LBUTTON))
		{
			return m_dwLastEffect == DROPEFFECT_NONE ? DRAGDROP_S_CANCEL : DRAGDROP_S_DROP;
		}

		return S_OK;
	}

	STDMETHODIMP GiveFeedback(DWORD dwEffect)
	{
		m_dwLastEffect = dwEffect;
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement(&m_refCount);
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		LONG rv = InterlockedDecrement(&m_refCount);
		if (!rv)
		{
			delete this;
		}
		return rv;
	}

private:
	long m_refCount;
	DWORD m_dwLastEffect;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropSource)
		COM_QI_ENTRY(IDropSource)
	END_COM_QI_IMPL()
};
