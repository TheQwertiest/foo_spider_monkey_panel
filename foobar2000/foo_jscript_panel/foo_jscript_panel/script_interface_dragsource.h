#pragma once

#include <ObjBase.h>

[
	dual,
	object,
	pointer_default(unique),
	library_block,
	uuid("bf48b127-17c6-4fce-84ac-f27e59422c24")
]
__interface IDragSourceObject : IDispatch
{
	STDMETHOD(StartDrag)(__interface IDataTransferObject * dataTransfer);
};
