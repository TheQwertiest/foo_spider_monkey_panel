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
	// Properties
	STDMETHODIMP get_DropEffect(BSTR * outDropEffect);
	STDMETHODIMP put_DropEffect(BSTR dropEffect);
	STDMETHODIMP get_EffectAllowed(BSTR * outEffectAllowed);
	STDMETHODIMP put_EffectAllowed(BSTR effectAllowed);
	STDMETHODIMP get_Types(VARIANT * outTypes);

	// Methods
	STDMETHODIMP ClearData(BSTR type);
	STDMETHODIMP SetData(BSTR type, BSTR data);
	STDMETHODIMP GetData(BSTR type, VARIANT * outData);
	STDMETHODIMP GetMetadbHandles(IDispatch * callback);
	STDMETHODIMP SetDragImage(__interface IGdiBitmap * bitmap, int x, int y);
};
