#pragma once

#include "script_interface_dragsource.h"
#include "com_tools.h"


class DragSourceObject : public IDispatchImpl3<IDragSourceObject>
{
protected:
	DragSourceObject();

public:
	STDMETHODIMP StartDrag(__interface IDataTransferObject * dataTransfer);
};
