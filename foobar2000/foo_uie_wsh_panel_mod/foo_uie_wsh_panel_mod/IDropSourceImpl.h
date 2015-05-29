#pragma once

#include <OleIdl.h>
#include <ShlObj.h>
#include <comdef.h>
#include <atlbase.h>


class IDropSourceImpl : public IDropSource
{
public:
	// IDropSource
	STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
	{
		// When escape key is pressed or mouse L&R button are pressed, end draging
		if (fEscapePressed || (grfKeyState & (MK_LBUTTON | MK_RBUTTON)) == (MK_LBUTTON | MK_RBUTTON))
			return DRAGDROP_S_CANCEL;

		if ((grfKeyState & (MK_LBUTTON | MK_RBUTTON)) == 0)
			return DRAGDROP_S_DROP;

		return S_OK;
	}

	STDMETHODIMP GiveFeedback(DWORD dwEffect)
	{
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}
};
