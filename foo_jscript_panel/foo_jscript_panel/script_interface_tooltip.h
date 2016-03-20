#pragma once

#include <ObjBase.h>

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("4ff021ab-17bc-43de-9dbe-2d0edec1e095")
]
__interface IFbTooltip: IDisposable
{
	[propget] STDMETHOD(Text)([out,retval] BSTR * pp);
	[propput] STDMETHOD(Text)(BSTR text);
	[propput] STDMETHOD(TrackActivate)(VARIANT_BOOL activate);
	STDMETHOD(Activate)();
	STDMETHOD(Deactivate)();
	STDMETHOD(SetMaxWidth)(int width);
	STDMETHOD(GetDelayTime)(int type, [out,retval] INT * p);
	STDMETHOD(SetDelayTime)(int type, int time);
	STDMETHOD(TrackPosition)(int x, int y);
};

