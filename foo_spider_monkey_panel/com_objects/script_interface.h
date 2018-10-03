#pragma once
#pragma warning(disable:4467)

[module(name = "foo_spider_monkey_panel", version = "0.9")];

extern ITypeLibPtr g_typelib;

[
	dual,
	object,
	pointer_default(unique),
	library_block,
	uuid("2e0bae19-3afe-473a-a703-0feb2d714655")
]
__interface IDisposable : IDispatch
{
	STDMETHOD(Dispose)();
};

[
    object,
    dual,
    pointer_default( unique ),
    library_block,
    uuid( "0A72A7F4-024C-4DAB-92BE-5F6853294E44" )
]
__interface IWrappedJs : IDispatch
{
    [id( DISPID_VALUE )] STDMETHOD(ExecuteValue)([optional] VARIANT arg1, [optional] VARIANT arg2,
                                                  [optional] VARIANT arg3, [optional] VARIANT arg4,
                                                  [optional] VARIANT arg5, [optional] VARIANT arg6,
                                                  [optional] VARIANT arg7,
                                                  [out, retval]VARIANT* result );
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("d53e81cd-0157-4cfe-a618-1F88d48dc0b8")
]
__interface IWSHUtils : IDispatch
{
	STDMETHOD(GetWndByClass)(BSTR class_name, [out, retval]__interface IWindow** pp);
	STDMETHOD(GetWndByHandle)(UINT window_id, [out, retval]__interface IWindow** pp);
	STDMETHOD(CloseWnd)(IWindow* wnd);
	STDMETHOD(ReleaseCapture)();
};
_COM_SMARTPTR_TYPEDEF(IWSHUtils, __uuidof(IWSHUtils));

//PLUS VERSION
#ifdef IsMinimized
#	undef IsMinimized
#endif
#ifdef IsMaximized
#	undef IsMaximized
#endif

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("2f691b02-bd48-4dee-ad75-443b4c8ab461")
]
__interface IWindow
{
	[propget] STDMETHOD(ID)([out, retval]UINT* pp);
	[propget] STDMETHOD(ClassName)([out, retval]BSTR* className);
	[propget] STDMETHOD(Left)([out, retval]INT* p);
	[propget] STDMETHOD(Top)([out, retval]INT* p);
	[propget] STDMETHOD(Width)([out, retval]INT* p);
	[propget] STDMETHOD(Height)([out, retval]INT* p);
	[propget] STDMETHOD(Style)([out, retval]INT* p);
	[propget] STDMETHOD(ExStyle)([out, retval]INT* p);
	[propget] STDMETHOD(Caption)([out, retval]BSTR* pp);
	[propput] STDMETHOD(Left)(INT left);
	[propput] STDMETHOD(Top)(INT top);
	[propput] STDMETHOD(Width)(INT width);
	[propput] STDMETHOD(Height)(INT height);
	[propput] STDMETHOD(Style)(INT style);
	[propput] STDMETHOD(ExStyle)(INT style);
	[propput] STDMETHOD(Caption)(BSTR title);

	STDMETHOD(GetChild)(BSTR caption, BSTR class_name, UINT index, [out, retval] IWindow** pp);
	STDMETHOD(GetChildWithSameProcess)(IWindow* searchWnd, BSTR caption, BSTR class_name, [out, retval] IWindow** pp);
	STDMETHOD(GetAncestor)([defaultvalue(1)]UINT flag, [out, retval] IWindow** pp);
	STDMETHOD(SetParent)(IWindow* p);
	STDMETHOD(SendMsg)(UINT msg, INT wp, INT lp);
	STDMETHOD(Show)(UINT flag);
	STDMETHOD(Move)(UINT x, UINT y, UINT w, UINT h, [defaultvalue(0)] VARIANT_BOOL redraw);
	STDMETHOD(IsVisible)([out, retval] VARIANT_BOOL* p);
	STDMETHOD(IsMinimized)([out, retval] VARIANT_BOOL* p);
	STDMETHOD(IsMaximized)([out, retval] VARIANT_BOOL* p);
	STDMETHOD(ShowCaret)(void);
	STDMETHOD(SetCaretPos)(INT x, INT y);
	STDMETHOD(HideCaret)(void);
	STDMETHOD(CreateCaret)(INT width, INT height);
	STDMETHOD(DestroyCaret)(void);
};
