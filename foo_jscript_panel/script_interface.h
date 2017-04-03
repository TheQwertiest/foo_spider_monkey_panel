#pragma once
#pragma warning(disable:4467)

[module(name = "foo_jscript_panel", version = "1.8")];

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

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("77e72064-1fb6-4754-a076-1dc517a6787b")
]
__interface IGdiObj : IDisposable
{
	[propget] STDMETHOD(_ptr)([out]void** pp);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("6fa87441-9f53-4a3f-999a-19509e3c92d7")
]
__interface IGdiFont : IGdiObj
{
	[propget] STDMETHOD(HFont)([out, retval] UINT* p);
	[propget] STDMETHOD(Height)([out, retval] UINT* p);
	[propget] STDMETHOD(Name)([defaultvalue(LANG_NEUTRAL)] LANGID langId, [out, retval] BSTR* outName);
	[propget] STDMETHOD(Size)([out, retval] float* outSize);
	[propget] STDMETHOD(Style)([out, retval] INT* outStyle);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("22d1f519-5d6e-4d5c-80e3-8fde0d1b946b")
]
__interface IGdiRawBitmap : IDisposable
{
	[propget] STDMETHOD(Height)([out, retval] UINT* p);
	[propget] STDMETHOD(Width)([out, retval] UINT* p);
	[propget] STDMETHOD(_Handle)([out] HDC* p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("7efbd443-4f6f-4cb2-9eee-882b9b19cbf6")
]
__interface IGdiBitmap : IGdiObj
{
	STDMETHOD(ApplyAlpha)(BYTE alpha, [out, retval] IGdiBitmap** pp);
	STDMETHOD(ApplyMask)(IGdiBitmap* mask, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(BoxBlur)([range(1, 20)] int radius, [range(1, 20), defaultvalue(1)] int iteration);
	STDMETHOD(Clone)(float x, float y, float w, float h, [out, retval] IGdiBitmap** pp);
	STDMETHOD(CreateRawBitmap)([out, retval] IGdiRawBitmap** pp);
	STDMETHOD(GetColorScheme)(UINT count, [out, retval] VARIANT* outArray);
	STDMETHOD(GetGraphics)([out, retval] __interface IGdiGraphics** pp);
	STDMETHOD(ReleaseGraphics)(__interface IGdiGraphics* p);
	STDMETHOD(Resize)(UINT w, UINT h, [range(Gdiplus::InterpolationModeInvalid, Gdiplus::InterpolationModeHighQualityBicubic), defaultvalue(0)] INT interpolationMode, [out, retval] IGdiBitmap** pp);
	STDMETHOD(RotateFlip)([range(Gdiplus::RotateNoneFlipNone, Gdiplus::Rotate270FlipX)] UINT mode);
	STDMETHOD(SaveAs)(BSTR path, [defaultvalue("image/png")] BSTR format, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(StackBlur)([range(0, 255)] int radius);
	[propget] STDMETHOD(Height)([out, retval] UINT* p);
	[propget] STDMETHOD(Width)([out, retval] UINT* p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("452682d2-feef-4351-b642-e8949435086b")
]
__interface IMeasureStringInfo
{
	[propget] STDMETHOD(chars)([out, retval] int* p);
	[propget] STDMETHOD(height)([out, retval] float* p);
	[propget] STDMETHOD(lines)([out, retval] int* p);
	[propget] STDMETHOD(width)([out, retval] float* p);
	[propget] STDMETHOD(x)([out, retval] float* p);
	[propget] STDMETHOD(y)([out, retval] float* p);
};


[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("9d6e404f-5ba7-4470-88d5-eb5980dffc07")
]
__interface IGdiGraphics : IGdiObj
{
	STDMETHOD(CalcTextHeight)(BSTR str, IGdiFont* font, [out, retval] UINT* p);
	STDMETHOD(CalcTextWidth)(BSTR str, IGdiFont* font, [out, retval] UINT* p);
	STDMETHOD(DrawEllipse)(float x, float y, float w, float h, float line_width, VARIANT color);
	STDMETHOD(DrawImage)(IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, [defaultvalue(0)] float angle, [defaultvalue(255)] BYTE alpha);
	STDMETHOD(DrawLine)(float x1, float y1, float x2, float y2, float line_width, VARIANT color);
	STDMETHOD(DrawPolygon)(VARIANT color, float line_width, VARIANT points);
	STDMETHOD(DrawRect)(float x, float y, float w, float h, float line_width, VARIANT color);
	STDMETHOD(DrawRoundRect)(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT color);
	STDMETHOD(DrawString)(BSTR str, IGdiFont* font, VARIANT color, float x, float y, float w, float h, [defaultvalue(0)] int flags);
	STDMETHOD(EstimateLineWrap)(BSTR str, IGdiFont* font, int max_width, [out, retval] VARIANT* p);
	STDMETHOD(FillEllipse)(float x, float y, float w, float h, VARIANT color);
	STDMETHOD(FillGradRect)(float x, float y, float w, float h, float angle, VARIANT color1, VARIANT color2, [defaultvalue(1)] float focus);
	STDMETHOD(FillPolygon)(VARIANT color, [range(0, 1)] INT fillmode, VARIANT points);
	STDMETHOD(FillRoundRect)(float x, float y, float w, float h, float arc_width, float arc_height, VARIANT color);
	STDMETHOD(FillSolidRect)(float x, float y, float w, float h, VARIANT color);
	STDMETHOD(GdiAlphaBlend)(IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, [defaultvalue(255)] BYTE alpha);
	STDMETHOD(GdiDrawBitmap)(IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH);
	STDMETHOD(GdiDrawText)(BSTR str, IGdiFont* font, VARIANT color, int x, int y, int w, int h, [defaultvalue(0)] int format, [out, retval] VARIANT* p);
	STDMETHOD(MeasureString)(BSTR str, IGdiFont* font, float x, float y, float w, float h, [defaultvalue(0)] int flags, [out, retval] IMeasureStringInfo** pp);
	STDMETHOD(SetInterpolationMode)([range(Gdiplus::InterpolationModeInvalid, Gdiplus::InterpolationModeHighQualityBicubic)] int mode);
	STDMETHOD(SetSmoothingMode)([range(Gdiplus::SmoothingModeInvalid, Gdiplus::SmoothingModeAntiAlias)] int mode);
	STDMETHOD(SetTextRenderingHint)([range(Gdiplus::TextRenderingHintSystemDefault, Gdiplus::TextRenderingHintClearTypeGridFit)] UINT mode);
	[propput] STDMETHOD(_ptr)(void* p);
};

_COM_SMARTPTR_TYPEDEF(IGdiGraphics, __uuidof(IGdiGraphics));


[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("351e3e75-8f27-4afd-b7e0-5409cf8f4947")
]
__interface IGdiUtils : IDispatch
{
	STDMETHOD(CreateImage)(int w, int h, [out, retval] IGdiBitmap** pp);
	STDMETHOD(CreateStyleTextRender)([defaultvalue(0)] VARIANT_BOOL pngmode, [out, retval] __interface IStyleTextRender** pp);
	STDMETHOD(Font)(BSTR name, float pxSize, [defaultvalue(0)] int style, [out, retval] IGdiFont** pp);
	STDMETHOD(Image)(BSTR path, [out, retval] IGdiBitmap** pp);
	STDMETHOD(LoadImageAsync)(UINT window_id, BSTR path, [out, retval] UINT* p);
};

_COM_SMARTPTR_TYPEDEF(IGdiUtils, __uuidof(IGdiUtils));

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("50e12553-8908-4eca-8801-ead834cea6f0")
]
__interface IStyleTextRender : IDisposable
{
	STDMETHOD(DiffusedShadow)(VARIANT color, int thickness, int offset_x, int offset_y);
	STDMETHOD(DoubleOutLineText)(int text_color, int outline_color1, int outline_color2, int outline_width1, int outline_width2);
	STDMETHOD(EnableShadow)(VARIANT_BOOL enable);
	STDMETHOD(GlowText)(int text_color, int glow_color, int glow_width);
	STDMETHOD(OutLineText)(int text_color, int outline_color, int outline_width);
	STDMETHOD(RenderStringPoint)(IGdiGraphics* g, BSTR str, IGdiFont* font, int x, int y, [defaultvalue(0)] int flags, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(RenderStringRect)(IGdiGraphics* g, BSTR str, IGdiFont* font, int x, int y, int w, int h, [defaultvalue(0)] int flags, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(ResetShadow)();
	STDMETHOD(SetPngImage)(IGdiBitmap* img);
	STDMETHOD(SetShadowBackgroundColor)(VARIANT color, int width, int height);
	STDMETHOD(SetShadowBackgroundImage)(IGdiBitmap* img);
	STDMETHOD(Shadow)(VARIANT color, int thickness, int offset_x, int offset_y);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("7c39dcf1-4e41-4a61-b06b-fb52107e4409")
]
__interface IFbFileInfo : IDisposable
{
	STDMETHOD(InfoFind)(BSTR name, [out, retval] UINT* p);
	STDMETHOD(InfoName)(UINT idx, [out, retval] BSTR* pp);
	STDMETHOD(InfoValue)(UINT idx, [out, retval] BSTR* pp);
	STDMETHOD(MetaAdd)(BSTR name, BSTR value, [out, retval] UINT* p);
	STDMETHOD(MetaFind)(BSTR name, [out, retval] UINT* p);
	STDMETHOD(MetaInsertValue)(UINT idx, UINT vidx, BSTR value);
	STDMETHOD(MetaName)(UINT idx, [out, retval] BSTR* pp);
	STDMETHOD(MetaRemoveField)(BSTR name);
	STDMETHOD(MetaSet)(BSTR name, BSTR value);
	STDMETHOD(MetaValue)(UINT idx, UINT vidx, [out, retval] BSTR* pp);
	STDMETHOD(MetaValueCount)(UINT idx, [out, retval] UINT* p);
	[propget] STDMETHOD(InfoCount)([out, retval] UINT* p);
	[propget] STDMETHOD(MetaCount)([out, retval] UINT* p);
	[propget] STDMETHOD(_ptr)([out]void** pp);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("0e1980d3-916a-482e-af87-578bcb1a4a25")
]
__interface IFbMetadbHandle : IDisposable
{
	STDMETHOD(Compare)(IFbMetadbHandle* handle, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(GetFileInfo)([out, retval] IFbFileInfo** pp);
	[propget] STDMETHOD(FileSize)([out, retval] LONGLONG* p);
	[propget] STDMETHOD(Length)([out, retval] double* p);
	[propget] STDMETHOD(Path)([out, retval] BSTR* pp);
	[propget] STDMETHOD(RawPath)([out, retval] BSTR* pp);
	[propget] STDMETHOD(SubSong)([out, retval] UINT* p);
	[propget] STDMETHOD(_ptr)([out]void** pp);
	[vararg] STDMETHOD(UpdateFileInfoSimple)([satype(VARIANT)] SAFEARRAY* p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("64528708-ae09-49dd-8e8d-1417fe9a9f09")
]
__interface IFbMetadbHandleList : IDisposable
{
	STDMETHOD(Add)(IFbMetadbHandle* handle, [out, retval] UINT* p);
	STDMETHOD(AddRange)(IFbMetadbHandleList* handles);
	STDMETHOD(BSearch)(IFbMetadbHandle* handle, [out, retval] UINT* p);
	STDMETHOD(CalcTotalDuration)([out, retval] double* p);
	STDMETHOD(CalcTotalSize)([out, retval] LONGLONG* p);
	STDMETHOD(Clone)([out, retval] IFbMetadbHandleList** pp);
	STDMETHOD(Find)(IFbMetadbHandle* handle, [out, retval] UINT* p);
	STDMETHOD(Insert)(UINT index, IFbMetadbHandle* handle, [out, retval] UINT* outIndex);
	STDMETHOD(InsertRange)(UINT index, IFbMetadbHandleList* handles, [out, retval] UINT* outIndex);
	STDMETHOD(MakeDifference)(IFbMetadbHandleList* handles);
	STDMETHOD(MakeIntersection)(IFbMetadbHandleList* handles);
	STDMETHOD(MakeUnion)(IFbMetadbHandleList* handles);
	STDMETHOD(OrderByFormat)(__interface IFbTitleFormat* script, int direction);
	STDMETHOD(OrderByPath)();
	STDMETHOD(OrderByRelativePath)();
	STDMETHOD(Remove)(IFbMetadbHandle* handle);
	STDMETHOD(RemoveAll)();
	STDMETHOD(RemoveById)(UINT index);
	STDMETHOD(RemoveRange)(UINT from, UINT count);
	STDMETHOD(Sort)();
	[propget] STDMETHOD(Count)([out, retval] UINT* p);
	[propget] STDMETHOD(Item)(UINT index, [out, retval] IFbMetadbHandle** pp);
	[propget] STDMETHOD(_ptr)([out, retval] void** pp);
	[propput] STDMETHOD(Item)(UINT index, IFbMetadbHandle* handle);
	[vararg] STDMETHOD(UpdateFileInfoSimple)([satype(VARIANT)] SAFEARRAY* p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("998d8666-b446-4e92-8e8f-797d3cce4b7e")
]
__interface IFbTitleFormat : IDisposable
{
	STDMETHOD(Eval)([defaultvalue(0)] VARIANT_BOOL force, [out, retval] BSTR* pp);
	STDMETHOD(EvalWithMetadb)(IFbMetadbHandle* handle, [out, retval] BSTR* pp);
	[propget] STDMETHOD(_ptr)([out, retval] void** pp);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("1e9f95ae-63be-49dc-a395-ee386e8eb202")
]
__interface IMenuObj : IDisposable
{
	STDMETHOD(AppendMenuItem)(UINT flags, UINT item_id, BSTR text);
	STDMETHOD(AppendMenuSeparator)();
	STDMETHOD(AppendTo)(IMenuObj* parent, UINT flags, BSTR text);
	STDMETHOD(CheckMenuItem)(UINT id_or_pos, VARIANT_BOOL check, [defaultvalue(0)] VARIANT_BOOL bypos);
	STDMETHOD(CheckMenuRadioItem)(UINT first, UINT last, UINT check, [defaultvalue(0)] VARIANT_BOOL bypos);
	STDMETHOD(EnableMenuItem)(UINT id_or_pos, UINT enable, [defaultvalue(0)] VARIANT_BOOL bypos);
	STDMETHOD(TrackPopupMenu)(int x, int y, [defaultvalue(0)] UINT flags, [out, retval] UINT* item_id);
	[propget] STDMETHOD(ID)([out, retval] UINT* p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("0e1bc833-b9f8-44b1-8240-57fff04602ad")
]
__interface IContextMenuManager : IDisposable
{
	STDMETHOD(BuildMenu)(IMenuObj* p, int base_id, int max_id);
	STDMETHOD(ExecuteByID)(UINT id, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(InitContext)(VARIANT handles);
	STDMETHOD(InitNowPlaying)();
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("4a357221-1b75-4379-8de7-6a865bbfad10")
]
__interface IMainMenuManager : IDisposable
{
	STDMETHOD(BuildMenu)(IMenuObj* p, int base_id, int count);
	STDMETHOD(ExecuteByID)(UINT id, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(Init)(BSTR root_name);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("2d7436ad-6527-4154-a3c7-361ab8b88f5c")
]
__interface IFbProfiler : IDispatch
{
	STDMETHOD(Reset)();
	STDMETHOD(Print)();
	[propget] STDMETHOD(Time)([out, retval] INT* p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("1f40f9e1-c0fb-4021-80de-37c4d0a26f45")
]
__interface IFbUiSelectionHolder : IDisposable
{
	STDMETHOD(SetPlaylistSelectionTracking)();
	STDMETHOD(SetPlaylistTracking)();
	STDMETHOD(SetSelection)(IFbMetadbHandleList* handles);
};


[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("bae2e084-6545-4a17-9795-1496a4ee2741")
]
__interface IFbUtils : IDispatch
{
	STDMETHOD(AcquireUiSelectionHolder)([out, retval] IFbUiSelectionHolder** outHolder);
	STDMETHOD(AddDirectory)();
	STDMETHOD(AddFiles)();
	STDMETHOD(ClearPlaylist)();
	STDMETHOD(CreateContextMenuManager)([out, retval] IContextMenuManager** pp);
	STDMETHOD(CreateMainMenuManager)([out, retval] IMainMenuManager** pp);
	STDMETHOD(CreateProfiler)([defaultvalue("")] BSTR name, [out, retval] IFbProfiler** pp);
	STDMETHOD(Exit)();
	STDMETHOD(GetFocusItem)([defaultvalue(-1)] VARIANT_BOOL force, [out, retval] IFbMetadbHandle** pp);
	STDMETHOD(GetLibraryItems)([out, retval] IFbMetadbHandleList** outItems);
	STDMETHOD(GetLibraryRelativePath)(IFbMetadbHandle* handle, [out, retval] BSTR* p);
	STDMETHOD(GetNowPlaying)([out, retval] IFbMetadbHandle** pp);
	STDMETHOD(GetQueryItems)(IFbMetadbHandleList* items, BSTR query, [out, retval] IFbMetadbHandleList** pp);
	STDMETHOD(GetSelection)([out, retval] IFbMetadbHandle** pp);
	STDMETHOD(GetSelectionType)([out, retval] UINT* p);
	STDMETHOD(GetSelections)([defaultvalue(0)] UINT flags, [out, retval] IFbMetadbHandleList** pp);
	STDMETHOD(IsLibraryEnabled)([out, retval] VARIANT_BOOL* p);
	STDMETHOD(IsMetadbInMediaLibrary)(IFbMetadbHandle* handle, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(LoadPlaylist)();
	STDMETHOD(Next)();
	STDMETHOD(Pause)();
	STDMETHOD(Play)();
	STDMETHOD(PlayOrPause)();
	STDMETHOD(Prev)();
	STDMETHOD(Random)();
	STDMETHOD(RunContextCommand)(BSTR command, [defaultvalue(0)] UINT flags, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(RunContextCommandWithMetadb)(BSTR command, VARIANT handle, [defaultvalue(0)] UINT flags, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(RunMainMenuCommand)(BSTR command, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(SavePlaylist)();
	STDMETHOD(ShowConsole)();
	STDMETHOD(ShowLibrarySearchUI)(BSTR query);
	STDMETHOD(ShowPopupMessage)(BSTR msg, [defaultvalue(JSP_NAME)] BSTR title, [defaultvalue(0), range(0, 2)] int iconid);
	STDMETHOD(ShowPreferences)();
	STDMETHOD(Stop)();
	STDMETHOD(TitleFormat)(BSTR expression, [out, retval] IFbTitleFormat** pp);
	STDMETHOD(VolumeDown)();
	STDMETHOD(VolumeMute)();
	STDMETHOD(VolumeUp)();
	[propget] STDMETHOD(ComponentPath)([out, retval] BSTR* pp);
	[propget] STDMETHOD(CursorFollowPlayback)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(FoobarPath)([out, retval] BSTR* pp);
	[propget] STDMETHOD(IsPaused)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(IsPlaying)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(PlaybackFollowCursor)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(PlaybackLength)([out, retval] double* p);
	[propget] STDMETHOD(PlaybackTime)([out, retval] double* p);
	[propget] STDMETHOD(ProfilePath)([out, retval] BSTR* pp);
	[propget] STDMETHOD(StopAfterCurrent)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(Volume)([out, retval] float* p);
	[propput] STDMETHOD(CursorFollowPlayback)(VARIANT_BOOL p);
	[propput] STDMETHOD(PlaybackFollowCursor)(VARIANT_BOOL p);
	[propput] STDMETHOD(PlaybackTime)(double time);
	[propput] STDMETHOD(StopAfterCurrent)(VARIANT_BOOL p);
	[propput] STDMETHOD(Volume)(float value);
	[vararg] STDMETHOD(Trace)([satype(VARIANT)] SAFEARRAY* p);
};

_COM_SMARTPTR_TYPEDEF(IFbUtils, __uuidof(IFbUtils));

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("8a14d6a2-4582-4398-a6af-2206f2dabbbe")
]
__interface IThemeManager : IDisposable
{
	STDMETHOD(DrawThemeBackground)(IGdiGraphics* gr, int x, int y, int w, int h, [defaultvalue(0)] int clip_x, [defaultvalue(0)] int clip_y, [defaultvalue(0)] int clip_w, [defaultvalue(0)] int clip_h);
	STDMETHOD(IsThemePartDefined)(int partid, [defaultvalue(0)] int stateid, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(SetPartAndStateID)(int partid, int stateid);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("91830eda-b5f2-4061-9923-7880192a2734")
]
__interface IDropSourceAction : IDisposable
{
	STDMETHOD(ToPlaylist)();
	[propget] STDMETHOD(Parsable)([out, retval] VARIANT_BOOL* parsable);
	[propget] STDMETHOD(Playlist)([out, retval] int* id);
	[propget] STDMETHOD(ToSelect)([out, retval] VARIANT_BOOL* to_select);
	[propput] STDMETHOD(Parsable)(VARIANT_BOOL parsable);
	[propput] STDMETHOD(Playlist)(int id);
	[propput] STDMETHOD(ToSelect)(VARIANT_BOOL to_select);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("81e1f0c0-1dfe-4996-abd9-ba98dff69e4c")
]
__interface IFbWindow : IDispatch
{
	STDMETHOD(ClearInterval)(UINT intervalID);
	STDMETHOD(ClearTimeout)(UINT timeoutID);
	STDMETHOD(CreatePopupMenu)([out, retval] IMenuObj** pp);
	STDMETHOD(CreateThemeManager)(BSTR classid, [out, retval] IThemeManager** pp);
	STDMETHOD(CreateTooltip)([defaultvalue("Segoe UI")] BSTR name, [defaultvalue(12)] float pxSize, [defaultvalue(0)] INT style, [out, retval] __interface IFbTooltip** pp);
	STDMETHOD(GetBackgroundImage)([out, retval] IGdiBitmap** pp);
	STDMETHOD(GetColorCUI)(UINT type, [defaultvalue("")] BSTR guidstr, [out, retval] int* p);
	STDMETHOD(GetColorDUI)(UINT type, [out, retval] int* p);
	STDMETHOD(GetFontCUI)(UINT type, [defaultvalue("")] BSTR guidstr, [out, retval] IGdiFont** pp);
	STDMETHOD(GetFontDUI)(UINT type, [out, retval] IGdiFont** pp);
	STDMETHOD(GetProperty)(BSTR name, [optional] VARIANT defaultval, [out, retval] VARIANT* p);
	STDMETHOD(NotifyOthers)(BSTR name, VARIANT info);
	STDMETHOD(Reload)();
	STDMETHOD(Repaint)([defaultvalue(0)] VARIANT_BOOL force);
	STDMETHOD(RepaintRect)(LONG x, LONG y, LONG w, LONG h, [defaultvalue(0)] VARIANT_BOOL force);
	STDMETHOD(SetCursor)(UINT id);
	STDMETHOD(SetInterval)(IDispatch* func, INT delay, [out, retval] UINT* outIntervalID);
	STDMETHOD(SetProperty)(BSTR name, VARIANT val);
	STDMETHOD(SetTimeout)(IDispatch* func, INT delay, [out, retval] UINT* outTimeoutID);
	STDMETHOD(ShowConfigure)();
	STDMETHOD(ShowProperties)();
	[propget] STDMETHOD(DlgCode)([out, retval] UINT* p);
	[propget] STDMETHOD(Height)([out, retval] INT* p);
	[propget] STDMETHOD(ID)([out, retval] UINT* p);
	[propget] STDMETHOD(InstanceType)([out, retval] UINT* p);
	[propget] STDMETHOD(IsTransparent)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(IsVisible)([out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(MaxHeight)([out, retval] UINT* p);
	[propget] STDMETHOD(MaxWidth)([out, retval] UINT* p);
	[propget] STDMETHOD(MinHeight)([out, retval] UINT* p);
	[propget] STDMETHOD(MinWidth)([out, retval] UINT* p);
	[propget] STDMETHOD(Width)([out, retval] INT* p);
	[propput] STDMETHOD(DlgCode)(UINT code);
	[propput] STDMETHOD(MaxHeight)(UINT height);
	[propput] STDMETHOD(MaxWidth)(UINT width);
	[propput] STDMETHOD(MinHeight)(UINT height);
	[propput] STDMETHOD(MinWidth)(UINT width);
};

_COM_SMARTPTR_TYPEDEF(IFbWindow, __uuidof(IFbWindow));

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("d53e81cd-0157-4cfe-a618-1F88d48dc0b7")
]
__interface IJSUtils : IDispatch
{
	STDMETHOD(CheckComponent)(BSTR name, [defaultvalue(-1)] VARIANT_BOOL is_dll, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(CheckFont)(BSTR name, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(FileTest)(BSTR path, BSTR mode, [out, retval] VARIANT* p);
	STDMETHOD(FormatDuration)(double p, [out, retval] BSTR* pp);
	STDMETHOD(FormatFileSize)(LONGLONG p, [out, retval] BSTR* pp);
	STDMETHOD(GetAlbumArtAsync)(UINT window_id, IFbMetadbHandle* handle, [defaultvalue(0)] int art_id, [defaultvalue(-1)] VARIANT_BOOL need_stub, [defaultvalue(0)] VARIANT_BOOL only_embed, [defaultvalue(0)] VARIANT_BOOL no_load, [out, retval] UINT* p);
	STDMETHOD(GetAlbumArtEmbedded)(BSTR rawpath, [defaultvalue(0)] int art_id, [out, retval] IGdiBitmap** pp);
	STDMETHOD(GetAlbumArtV2)(IFbMetadbHandle* handle, [defaultvalue(0)] int art_id, [defaultvalue(-1)] VARIANT_BOOL need_stub, [out, retval] IGdiBitmap** pp);
	STDMETHOD(GetSysColor)(UINT index, [out, retval] int* p);
	STDMETHOD(GetSystemMetrics)(UINT index, [out, retval] int* p);
	STDMETHOD(Glob)(BSTR pattern, [defaultvalue(FILE_ATTRIBUTE_DIRECTORY)] UINT exc_mask, [defaultvalue(0xffffffff)] UINT inc_mask, [out, retval] VARIANT* p);
	STDMETHOD(IsKeyPressed)(UINT vkey, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(MapString)(BSTR str, UINT lcid, UINT flags, [out, retval] BSTR* pp);
	STDMETHOD(PathWildcardMatch)(BSTR pattern, BSTR str, [out, retval] VARIANT_BOOL* p);
	STDMETHOD(ReadINI)(BSTR filename, BSTR section, BSTR key, [optional] VARIANT defaultval, [out, retval] BSTR* pp);
	STDMETHOD(ReadTextFile)(BSTR filename, [defaultvalue(0)] UINT codepage, [out, retval] BSTR* pp);
	STDMETHOD(WriteINI)(BSTR filename, BSTR section, BSTR key, VARIANT val, [out, retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(Version)([out, retval] UINT* v);
};

_COM_SMARTPTR_TYPEDEF(IJSUtils, __uuidof(IJSUtils));
