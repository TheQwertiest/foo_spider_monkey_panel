#pragma once

#include "script_interface_datatransfer.h"
#include "com_tools.h"


class DataTransferObject : public IDispatchImpl3<IDataTransferObject>
{
private:
	unsigned m_dropEffect;
	unsigned m_effectAllowed;
	IDataObject * m_dataObject;

protected:
	DataTransferObject();

public:
	STDMETHODIMP ClearData(BSTR type);
	STDMETHODIMP GetData(BSTR type, VARIANT * outData);
	STDMETHODIMP GetMetadbHandles(IDispatch * callback);
	STDMETHODIMP SetData(BSTR type, BSTR data);
	STDMETHODIMP SetDragImage(__interface IGdiBitmap * bitmap, int x, int y);
	STDMETHODIMP get_DropEffect(BSTR * outDropEffect);
	STDMETHODIMP get_EffectAllowed(BSTR * outEffectAllowed);
	STDMETHODIMP get_Types(VARIANT * outTypes);
	STDMETHODIMP put_DropEffect(BSTR dropEffect);
	STDMETHODIMP put_EffectAllowed(BSTR effectAllowed);
};
