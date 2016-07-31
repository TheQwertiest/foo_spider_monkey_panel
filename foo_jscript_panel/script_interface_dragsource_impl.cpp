#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_datatransfer_impl.h"
#include "script_interface_dragsource_impl.h"


DragSourceObject::DragSourceObject()
{
}

STDMETHODIMP DragSourceObject::StartDrag(__interface IDataTransferObject * dataTransfer)
{
	TRACK_FUNCTION();

	//DoDragDrop();
	return S_OK;
}
