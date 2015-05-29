#pragma once

#include <ObjBase.h>

[
	dual,
	object,
	pointer_default(unique),
	library_block,
	uuid("ec467423-55eb-4900-a90c-2f3ad371f0b2")
]
__interface IDataTransferObject : IDispatch
{
	// Properties
	[propget] STDMETHOD(DropEffect)([out,retval] BSTR * outDropEffect);
	[propput] STDMETHOD(DropEffect)(BSTR dropEffect);
	[propget] STDMETHOD(EffectAllowed)([out,retval] BSTR * outEffectAllowed);
	[propput] STDMETHOD(EffectAllowed)(BSTR effectAllowed);
	[propget] STDMETHOD(Types)([out,retval] VARIANT * outTypes);

	// Methods
	STDMETHOD(ClearData)([defaultvalue("")] BSTR type);
	STDMETHOD(SetData)(BSTR type, VARIANT data);
	STDMETHOD(GetData)(BSTR type, [out,retval] VARIANT * outData);
	STDMETHOD(SetDragImage)(__interface IGdiBitmap * bitmap, int x, int y);
};
