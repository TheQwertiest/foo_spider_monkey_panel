#pragma once

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("4ff021ab-17bc-43de-9dbe-2d0edec1e095")
]
__interface IFbTooltip : IDisposable
{
	STDMETHOD(Activate)();
	STDMETHOD(Deactivate)();
	STDMETHOD(GetDelayTime)(int type, [out, retval] INT* p);
	STDMETHOD(SetDelayTime)(int type, int time);
	STDMETHOD(SetMaxWidth)(int width);
	STDMETHOD(TrackPosition)(int x, int y);
	[propget] STDMETHOD(Text)([out, retval] BSTR* pp);
	[propput] STDMETHOD(Text)(BSTR text);
	[propput] STDMETHOD(TrackActivate)(VARIANT_BOOL activate);
};
