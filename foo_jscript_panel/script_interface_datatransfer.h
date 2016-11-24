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
	STDMETHOD(ClearData)([defaultvalue("")] BSTR type);
	STDMETHOD(GetData)(BSTR type, [out, retval] VARIANT* outData);
	STDMETHOD(SetData)(BSTR type, VARIANT data);
	STDMETHOD(SetDragImage)(__interface IGdiBitmap* bitmap, int x, int y);
	[propget] STDMETHOD(DropEffect)([out, retval] BSTR* outDropEffect);
	[propget] STDMETHOD(EffectAllowed)([out, retval] BSTR* outEffectAllowed);
	[propget] STDMETHOD(Types)([out, retval] VARIANT* outTypes);
	[propput] STDMETHOD(DropEffect)(BSTR dropEffect);
	[propput] STDMETHOD(EffectAllowed)(BSTR effectAllowed);
};
