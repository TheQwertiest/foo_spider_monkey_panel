#pragma once

[module(name="foo_uie_wsh_panel_mod", version="1.8")];

extern ITypeLibPtr g_typelib;

//-- IDisposable --
[
	dual,
	object,
	pointer_default(unique),
	library_block,
	uuid("2e0bae19-3afe-473a-a703-0feb2d714655")
]
__interface IDisposable: IDispatch
{
	STDMETHOD(Dispose)();
};

//----------------------------------------------------------------------------
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("77e72064-1fb6-4754-a076-1dc517a6787b")
]
__interface IGdiObj: IDisposable
{
	[propget] STDMETHOD(_ptr)([out]void ** pp);
};


//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("6fa87441-9f53-4a3f-999a-19509e3c92d7")
]
__interface IGdiFont: IGdiObj
{
	[propget] STDMETHOD(HFont)([out,retval] UINT* p);
	[propget] STDMETHOD(Height)([out,retval] UINT* p);
	[propget] STDMETHOD(Name)([defaultvalue(LANG_NEUTRAL)] LANGID langId, [out,retval] BSTR * outName);
	[propget] STDMETHOD(Size)([out,retval] float * outSize);
	[propget] STDMETHOD(Style)([out,retval] INT * outStyle);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("22d1f519-5d6e-4d5c-80e3-8fde0d1b946b")
]
__interface IGdiRawBitmap: IDisposable
{
	[propget] STDMETHOD(_Handle)([out] HDC * p);
	[propget] STDMETHOD(Width)([out,retval] UINT* p);
	[propget] STDMETHOD(Height)([out,retval] UINT* p);
	//STDMETHOD(GetBitmap)([out,retval] __interface IGdiBitmap ** pp);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("7efbd443-4f6f-4cb2-9eee-882b9b19cbf6")
]
__interface IGdiBitmap: IGdiObj
{
	[propget] STDMETHOD(Width)([out,retval] UINT* p);
	[propget] STDMETHOD(Height)([out,retval] UINT* p);
	STDMETHOD(Clone)(float x, float y, float w, float h, [out,retval] IGdiBitmap ** pp);
	STDMETHOD(RotateFlip)([range(Gdiplus::RotateNoneFlipNone, Gdiplus::Rotate270FlipX)] UINT mode);
	STDMETHOD(ApplyAlpha)(BYTE alpha, [out,retval] IGdiBitmap ** pp);
	STDMETHOD(ApplyMask)(IGdiBitmap * mask, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(CreateRawBitmap)([out,retval] IGdiRawBitmap ** pp);
	STDMETHOD(GetGraphics)([out,retval] __interface IGdiGraphics ** pp);
	STDMETHOD(ReleaseGraphics)(__interface IGdiGraphics * p);
	STDMETHOD(BoxBlur)([range(1,20)] int radius, [range(1,20),defaultvalue(1)] int iteration);
	STDMETHOD(Resize)(UINT w, UINT h, [range(Gdiplus::InterpolationModeInvalid, Gdiplus::InterpolationModeHighQualityBicubic), defaultvalue(0)] INT interpolationMode, [out,retval] IGdiBitmap ** pp);
	STDMETHOD(GetColorScheme)(UINT count, [out,retval] VARIANT * outArray);
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
	[propget] STDMETHOD(x)([out,retval] float * p);
	[propget] STDMETHOD(y)([out,retval] float * p);
	[propget] STDMETHOD(width)([out,retval] float * p);
	[propget] STDMETHOD(height)([out,retval] float * p);
	[propget] STDMETHOD(lines)([out,retval] int * p);
	[propget] STDMETHOD(chars)([out,retval] int * p);
};


[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("9d6e404f-5ba7-4470-88d5-eb5980dffc07")
]
__interface IGdiGraphics: IGdiObj
{
	[propput] STDMETHOD(_ptr)(void * p);
	STDMETHOD(FillSolidRect)(float x, float y, float w, float h, VARIANT color);
	STDMETHOD(FillGradRect)(float x, float y, float w, float h, float angle, VARIANT color1, VARIANT color2, [defaultvalue(1.0)] float focus);
	STDMETHOD(FillRoundRect)(float x, float y, float w, float h, float arc_width, float arc_height, VARIANT color);
	STDMETHOD(FillEllipse)(float x, float y, float w, float h, VARIANT color);
	STDMETHOD(FillPolygon)(VARIANT color, [range(0, 1)]INT fillmode, VARIANT points);

	STDMETHOD(DrawLine)(float x1, float y1, float x2, float y2, float line_width, VARIANT color);
	STDMETHOD(DrawRect)(float x, float y, float w, float h, float line_width, VARIANT color);
	STDMETHOD(DrawRoundRect)(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT color);
	STDMETHOD(DrawEllipse)(float x, float y, float w, float h, float line_width, VARIANT color);
	STDMETHOD(DrawPolygon)(VARIANT color, float line_width, VARIANT points);

	STDMETHOD(DrawString)(BSTR str, IGdiFont* font, VARIANT color, float x, float y, float w, float h, [defaultvalue(0)] int flags);
	STDMETHOD(GdiDrawText)(BSTR str, IGdiFont * font, VARIANT color, int x, int y, int w, int h, [defaultvalue(0)] int format, [out,retval] VARIANT * p);
	STDMETHOD(DrawImage)(IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, [defaultvalue(0.0)]float angle, [defaultvalue(255)]BYTE alpha);
	STDMETHOD(GdiDrawBitmap)(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH);
	STDMETHOD(GdiAlphaBlend)(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, [defaultvalue(255)]BYTE alpha);
	//STDMETHOD(GdiTransparentBlt)(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, VARIANT color);
	STDMETHOD(MeasureString)(BSTR str, IGdiFont * font, float x, float y, float w, float h, [defaultvalue(0)] int flags, [out,retval] IMeasureStringInfo ** pp);
	STDMETHOD(CalcTextWidth)(BSTR str, IGdiFont * font, [out,retval] UINT * p);
	STDMETHOD(CalcTextHeight)(BSTR str, IGdiFont * font, [out,retval] UINT * p);
	STDMETHOD(EstimateLineWrap)(BSTR str, IGdiFont * font, int max_width, [out,retval] VARIANT * p);
	STDMETHOD(SetTextRenderingHint)([range(Gdiplus::TextRenderingHintSystemDefault, Gdiplus::TextRenderingHintClearTypeGridFit)] UINT mode);
	STDMETHOD(SetSmoothingMode)([range(Gdiplus::SmoothingModeInvalid, Gdiplus::SmoothingModeAntiAlias)] int mode);
	STDMETHOD(SetInterpolationMode)([range(Gdiplus::InterpolationModeInvalid, Gdiplus::InterpolationModeHighQualityBicubic)] int mode);
	//STDMETHOD(SetCompositingMode)([range(Gdiplus::CompositingModeSourceOver, Gdiplus::CompositingModeSourceCopy)] UINT mode);
	//STDMETHOD(SetCompositingQuality)([range(Gdiplus::CompositingQualityInvalid, Gdiplus::CompositingQualityAssumeLinear)] int mode);
};
_COM_SMARTPTR_TYPEDEF(IGdiGraphics, __uuidof(IGdiGraphics));

//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("351e3e75-8f27-4afd-b7e0-5409cf8f4947")
]
__interface IGdiUtils: IDispatch
{
	STDMETHOD(Font)(BSTR name, float pxSize, [defaultvalue(0)] int style, [out,retval] IGdiFont** pp);
	STDMETHOD(Image)(BSTR path, [out,retval] IGdiBitmap** pp);
	STDMETHOD(CreateImage)(int w, int h, [out,retval] IGdiBitmap** pp);
	STDMETHOD(CreateStyleTextRender)([defaultvalue(0)] VARIANT_BOOL pngmode, [out,retval] __interface IStyleTextRender ** pp);
	STDMETHOD(LoadImageAsync)(UINT window_id, BSTR path, [out,retval] UINT * p);
};
_COM_SMARTPTR_TYPEDEF(IGdiUtils, __uuidof(IGdiUtils));

//--
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("50e12553-8908-4eca-8801-ead834cea6f0")
]
__interface IStyleTextRender: IDisposable
{
	// Outline
	STDMETHOD(OutLineText)(int text_color, int outline_color, int outline_width);
	STDMETHOD(DoubleOutLineText)(int text_color, int outline_color1, int outline_color2, int outline_width1, int outline_width2);
	STDMETHOD(GlowText)(int text_color, int glow_color, int glow_width);
	// Shadow
	STDMETHOD(EnableShadow)(VARIANT_BOOL enable);
	STDMETHOD(ResetShadow)();
	STDMETHOD(Shadow)(VARIANT color, int thickness, int offset_x, int offset_y);
	STDMETHOD(DiffusedShadow)(VARIANT color, int thickness, int offset_x, int offset_y);
	STDMETHOD(SetShadowBackgroundColor)(VARIANT color, int width, int height);
	STDMETHOD(SetShadowBackgroundImage)(IGdiBitmap * img);
	// Render 
	STDMETHOD(RenderStringPoint)(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, [defaultvalue(0)] int flags, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(RenderStringRect)(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, int w, int h, [defaultvalue(0)] int flags, [out,retval] VARIANT_BOOL * p);
	// PNG Mode Only
	STDMETHOD(SetPngImage)(IGdiBitmap * img);
};

//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("7c39dcf1-4e41-4a61-b06b-fb52107e4409")
]
__interface IFbFileInfo: IDisposable
{
	[propget] STDMETHOD(_ptr)([out]void ** pp);
	[propget] STDMETHOD(MetaCount)([out,retval] UINT* p);
	STDMETHOD(MetaValueCount)(UINT idx, [out,retval] UINT* p);
	STDMETHOD(MetaName)(UINT idx, [out,retval] BSTR* pp);
	STDMETHOD(MetaValue)(UINT idx, UINT vidx, [out,retval] BSTR* pp);
	STDMETHOD(MetaFind)(BSTR name, [out,retval] UINT * p);
	STDMETHOD(MetaRemoveField)(BSTR name);
	STDMETHOD(MetaAdd)(BSTR name, BSTR value, [out,retval] UINT * p);
	STDMETHOD(MetaInsertValue)(UINT idx, UINT vidx, BSTR value);
	[propget] STDMETHOD(InfoCount)([out,retval] UINT* p);
	STDMETHOD(InfoName)(UINT idx, [out,retval] BSTR* pp);
	STDMETHOD(InfoValue)(UINT idx, [out,retval] BSTR* pp);
	STDMETHOD(InfoFind)(BSTR name, [out,retval] UINT * p);
	STDMETHOD(MetaSet)(BSTR name, BSTR value);
};

//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("0e1980d3-916a-482e-af87-578bcb1a4a25")
]
__interface IFbMetadbHandle: IDisposable
{
	[propget] STDMETHOD(_ptr)([out]void ** pp);
	[propget] STDMETHOD(Path)([out,retval] BSTR* pp);
	[propget] STDMETHOD(RawPath)([out,retval] BSTR * pp);
	[propget] STDMETHOD(SubSong)([out,retval] UINT* p);
	[propget] STDMETHOD(FileSize)([out,retval] LONGLONG* p);
	[propget] STDMETHOD(Length)([out,retval] double* p);
	STDMETHOD(GetFileInfo)([out,retval] IFbFileInfo ** pp);
	STDMETHOD(UpdateFileInfo)(IFbFileInfo * fileinfo);
	[vararg] STDMETHOD(UpdateFileInfoSimple)([satype(VARIANT)] SAFEARRAY * p);
	STDMETHOD(Compare)(IFbMetadbHandle * handle, [out,retval] VARIANT_BOOL * p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("64528708-ae09-49dd-8e8d-1417fe9a9f09")
]
__interface IFbMetadbHandleList: IDisposable
{
	[propget] STDMETHOD(_ptr)([out,retval] void ** pp);
	[propget] STDMETHOD(Item)(UINT index, [out,retval] IFbMetadbHandle ** pp);
	[propput] STDMETHOD(Item)(UINT index, IFbMetadbHandle * handle);
	[propget] STDMETHOD(Count)([out,retval] UINT * p);

	STDMETHOD(Clone)([out,retval] IFbMetadbHandleList ** pp);
	STDMETHOD(Insert)(UINT index, IFbMetadbHandle * handle, [out,retval] UINT * outIndex);
	STDMETHOD(InsertRange)(UINT index, IFbMetadbHandleList * handles, [out,retval] UINT * outIndex);
	STDMETHOD(Add)(IFbMetadbHandle * handle, [out,retval] UINT * p);
	STDMETHOD(AddRange)(IFbMetadbHandleList * handles);
	STDMETHOD(RemoveById)(UINT index);
	STDMETHOD(Remove)(IFbMetadbHandle * handle);
	STDMETHOD(RemoveRange)(UINT from, UINT count);
	STDMETHOD(RemoveAll)();
	STDMETHOD(Sort)();
	STDMETHOD(Find)(IFbMetadbHandle * handle, [out,retval] UINT * p);
	STDMETHOD(BSearch)(IFbMetadbHandle * handle, [out,retval] UINT * p);
	STDMETHOD(MakeIntersection)(IFbMetadbHandleList * handles);
	STDMETHOD(MakeUnion)(IFbMetadbHandleList * handles);
	STDMETHOD(MakeDifference)(IFbMetadbHandleList * handles);
	STDMETHOD(OrderByFormat)(__interface IFbTitleFormat * script, int direction);
	STDMETHOD(OrderByPath)();
	STDMETHOD(OrderByRelativePath)();
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("998d8666-b446-4e92-8e8f-797d3cce4b7e")
]
__interface IFbTitleFormat: IDisposable
{
	[propget] STDMETHOD(_ptr)([out,retval] void ** pp);
	STDMETHOD(Eval)([defaultvalue(0)] VARIANT_BOOL force, [out,retval] BSTR* pp);
	STDMETHOD(EvalWithMetadb)(IFbMetadbHandle * handle, [out,retval] BSTR * pp);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("1e9f95ae-63be-49dc-a395-ee386e8eb202")
]
__interface IMenuObj: IDisposable
{
	[propget] STDMETHOD(ID)([out,retval] UINT * p);
	STDMETHOD(AppendMenuItem)(UINT flags, UINT item_id, BSTR text);
	STDMETHOD(AppendMenuSeparator)();
	STDMETHOD(EnableMenuItem)(UINT id_or_pos, UINT enable, [defaultvalue(0)] VARIANT_BOOL bypos);
	STDMETHOD(CheckMenuItem)(UINT id_or_pos, VARIANT_BOOL check, [defaultvalue(0)] VARIANT_BOOL bypos);
	STDMETHOD(CheckMenuRadioItem)(UINT first, UINT last, UINT check, [defaultvalue(0)] VARIANT_BOOL bypos);
	STDMETHOD(TrackPopupMenu)(int x, int y, [defaultvalue(0)] UINT flags, [out,retval] UINT * item_id);
	STDMETHOD(AppendTo)(IMenuObj * parent, UINT flags, BSTR text);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("0e1bc833-b9f8-44b1-8240-57fff04602ad")
]
__interface IContextMenuManager: IDisposable
{
	STDMETHOD(InitContext)(VARIANT handles);
	STDMETHOD(InitNowPlaying)();
	STDMETHOD(BuildMenu)(IMenuObj * p, int base_id, int max_id);
	STDMETHOD(ExecuteByID)(UINT id, [out,retval] VARIANT_BOOL * p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("4a357221-1b75-4379-8de7-6a865bbfad10")
]
__interface IMainMenuManager: IDisposable
{
	STDMETHOD(Init)(BSTR root_name);
	STDMETHOD(BuildMenu)(IMenuObj * p, int base_id, int count);
	STDMETHOD(ExecuteByID)(UINT id, [out,retval] VARIANT_BOOL * p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("2d7436ad-6527-4154-a3c7-361ab8b88f5c")
]
__interface IFbProfiler: IDispatch
{
	STDMETHOD(Reset)();
	STDMETHOD(Print)();
	[propget] STDMETHOD(Time)([out,retval] INT * p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("1f40f9e1-c0fb-4021-80de-37c4d0a26f45")
]
__interface IFbUiSelectionHolder: IDisposable
{
	STDMETHOD(SetSelection)(IFbMetadbHandleList * handles);
	STDMETHOD(SetPlaylistSelectionTracking)();
	STDMETHOD(SetPlaylistTracking)();
};


[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("bae2e084-6545-4a17-9795-1496a4ee2741")
]
__interface IFbUtils: IDispatch
{
	[vararg] STDMETHOD(trace)([satype(VARIANT)] SAFEARRAY * p);
	STDMETHOD(ShowPopupMessage)(BSTR msg, [defaultvalue(WSPM_NAME)] BSTR title, [defaultvalue(0),range(0,2)] int iconid);
	STDMETHOD(CreateProfiler)([defaultvalue("")] BSTR name, [out,retval] IFbProfiler ** pp);
	STDMETHOD(TitleFormat)(BSTR expression, [out,retval] IFbTitleFormat** pp);
	STDMETHOD(GetNowPlaying)([out,retval] IFbMetadbHandle** pp);
	STDMETHOD(GetFocusItem)([defaultvalue(-1)] VARIANT_BOOL force, [out,retval] IFbMetadbHandle** pp);
	STDMETHOD(GetSelection)([out,retval] IFbMetadbHandle** pp);
	STDMETHOD(GetSelections)([defaultvalue(0)] UINT flags, [out,retval] IFbMetadbHandleList ** pp);
	STDMETHOD(GetSelectionType)([out,retval] UINT* p);
	STDMETHOD(AcquireUiSelectionHolder)([out,retval] IFbUiSelectionHolder ** outHolder);
	
	[propget] STDMETHOD(ComponentPath)([out,retval] BSTR* pp);
	[propget] STDMETHOD(FoobarPath)([out,retval] BSTR* pp);
	[propget] STDMETHOD(ProfilePath)([out,retval] BSTR* pp);
	[propget] STDMETHOD(IsPlaying)([out,retval] VARIANT_BOOL * p);
	[propget] STDMETHOD(IsPaused)([out,retval] VARIANT_BOOL * p);
	[propget] STDMETHOD(PlaybackTime)([out,retval] double* p);
	[propput] STDMETHOD(PlaybackTime)(double time);
	[propget] STDMETHOD(PlaybackLength)([out,retval] double* p);
	[propget] STDMETHOD(PlaybackOrder)([out,retval] UINT* p);
	[propput] STDMETHOD(PlaybackOrder)(UINT order);
	[propget] STDMETHOD(StopAfterCurrent)([out,retval] VARIANT_BOOL * p);
	[propput] STDMETHOD(StopAfterCurrent)(VARIANT_BOOL p);
	[propget] STDMETHOD(CursorFollowPlayback)([out,retval] VARIANT_BOOL * p);
	[propput] STDMETHOD(CursorFollowPlayback)(VARIANT_BOOL p);
	[propget] STDMETHOD(PlaybackFollowCursor)([out,retval] VARIANT_BOOL * p);
	[propput] STDMETHOD(PlaybackFollowCursor)(VARIANT_BOOL p);
	[propget] STDMETHOD(Volume)([out,retval] float* p);
	[propput] STDMETHOD(Volume)(float value);
	//
	STDMETHOD(Exit)();
	STDMETHOD(Play)();
	STDMETHOD(Stop)();
	STDMETHOD(Pause)();
	STDMETHOD(PlayOrPause)();
	STDMETHOD(Next)();
	STDMETHOD(Prev)();
	STDMETHOD(Random)();
	STDMETHOD(VolumeDown)();
	STDMETHOD(VolumeUp)();
	STDMETHOD(VolumeMute)();
	STDMETHOD(AddDirectory)();
	STDMETHOD(AddFiles)();
	STDMETHOD(ShowConsole)();
	STDMETHOD(ShowPreferences)();
	STDMETHOD(ClearPlaylist)();
	STDMETHOD(LoadPlaylist)();
	STDMETHOD(SavePlaylist)();
	STDMETHOD(RunMainMenuCommand)(BSTR command, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(RunContextCommand)(BSTR command, [defaultvalue(0)] UINT flags, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(RunContextCommandWithMetadb)(BSTR command, VARIANT handle, [defaultvalue(0)] UINT flags, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(CreateContextMenuManager)([out,retval] IContextMenuManager ** pp);
	STDMETHOD(CreateMainMenuManager)([out,retval] IMainMenuManager ** pp);
	STDMETHOD(IsMetadbInMediaLibrary)(IFbMetadbHandle * handle, [out,retval] VARIANT_BOOL * p);
	//
	[propget] STDMETHOD(ActivePlaylist)([out,retval] UINT * p);
	[propput] STDMETHOD(ActivePlaylist)(UINT idx);
	[propget] STDMETHOD(PlayingPlaylist)([out,retval] UINT * p);
	[propput] STDMETHOD(PlayingPlaylist)(UINT idx);
	[propget] STDMETHOD(PlaylistCount)([out,retval] UINT * p);
	[propget] STDMETHOD(PlaylistItemCount)(UINT idx, [out,retval] UINT * p);
	STDMETHOD(GetPlaylistName)(UINT idx, [out,retval] BSTR * p);
	STDMETHOD(CreatePlaylist)(UINT idx, BSTR name, [out,retval] UINT * p);
	STDMETHOD(RemovePlaylist)(UINT idx, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(MovePlaylist)(UINT from, UINT to, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(RenamePlaylist)(UINT idx, BSTR name, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(DuplicatePlaylist)(UINT from, [defaultvalue("")] BSTR name, [out,retval] UINT * p);
	STDMETHOD(IsAutoPlaylist)(UINT idx, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(CreateAutoPlaylist)(UINT idx, BSTR name, BSTR query, [defaultvalue("")] BSTR sort, [defaultvalue(0)]UINT flags, [out,retval] UINT * p);
	STDMETHOD(ShowAutoPlaylistUI)(UINT idx, [out,retval] VARIANT_BOOL * p);
};
_COM_SMARTPTR_TYPEDEF(IFbUtils, __uuidof(IFbUtils));

//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("c74bdea4-4587-45c2-b0a7-91fae0cdf1a4")
]
__interface ITimerObj: IDisposable
{
	[propget] STDMETHOD(ID)([out,retval] UINT * p);
};


[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("8a14d6a2-4582-4398-a6af-2206f2dabbbe")
]
__interface IThemeManager: IDisposable
{
	STDMETHOD(SetPartAndStateID)(int partid, int stateid);
	STDMETHOD(IsThemePartDefined)(int partid, [defaultvalue(0)] int stateid, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(DrawThemeBackground)(IGdiGraphics * gr, int x, int y, int w, int h, [defaultvalue(0)] int clip_x, [defaultvalue(0)] int clip_y, [defaultvalue(0)] int clip_w, [defaultvalue(0)] int clip_h);
	// Vista+
	//STDMETHOD(DrawThemeTextEx)(IGdiGraphics * gr, BSTR text, int x, int y, int w, int h, [defaultvalue(0)] int format, [out,retval] VARIANT * p);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("91830eda-b5f2-4061-9923-7880192a2734")
]
__interface IDropSourceAction: IDisposable
{
	[propget] STDMETHOD(Parsable)([out,retval] VARIANT_BOOL * parsable);
	[propput] STDMETHOD(Parsable)(VARIANT_BOOL parsable);
	//[propget] STDMETHOD(Mode)([out,retval] int * mode);
	[propget] STDMETHOD(Playlist)([out,retval] int * id);
	[propput] STDMETHOD(Playlist)(int id);
	[propget] STDMETHOD(ToSelect)([out,retval] VARIANT_BOOL * to_select);
	[propput] STDMETHOD(ToSelect)(VARIANT_BOOL to_select);
	
	STDMETHOD(ToPlaylist)();
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("81e1f0c0-1dfe-4996-abd9-ba98dff69e4c")
]
__interface IFbWindow: IDispatch
{
	[propget] STDMETHOD(ID)([out,retval] UINT* p);
	[propget] STDMETHOD(Width)([out,retval] INT* p);
	[propget] STDMETHOD(Height)([out,retval] INT* p);
	[propget] STDMETHOD(InstanceType)([out,retval] UINT* p);
	[propget] STDMETHOD(MaxWidth)([out,retval] UINT* p);
	[propput] STDMETHOD(MaxWidth)(UINT width);
	[propget] STDMETHOD(MaxHeight)([out,retval] UINT* p);
	[propput] STDMETHOD(MaxHeight)(UINT height);
	[propget] STDMETHOD(MinWidth)([out,retval] UINT* p);
	[propput] STDMETHOD(MinWidth)(UINT width);
	[propget] STDMETHOD(MinHeight)([out,retval] UINT* p);
	[propput] STDMETHOD(MinHeight)(UINT height);
	[propget] STDMETHOD(DlgCode)([out,retval] UINT* p);
	[propput] STDMETHOD(DlgCode)(UINT code);
	[propget] STDMETHOD(IsTransparent)([out,retval] VARIANT_BOOL* p);
	[propget] STDMETHOD(IsVisible)([out,retval] VARIANT_BOOL* p);
	STDMETHOD(Repaint)([defaultvalue(0)] VARIANT_BOOL force);
	STDMETHOD(RepaintRect)(UINT x, UINT y, UINT w, UINT h, [defaultvalue(0)] VARIANT_BOOL force);
	STDMETHOD(CreatePopupMenu)([out,retval] IMenuObj ** pp);
	STDMETHOD(CreateTimerTimeout)(UINT timeout, [out,retval] ITimerObj ** pp);
	STDMETHOD(CreateTimerInterval)(UINT delay, [out,retval] ITimerObj ** pp);
	STDMETHOD(KillTimer)(ITimerObj * p);
	STDMETHOD(SetInterval)(IDispatch * func, INT delay, [out,retval] UINT * outIntervalID);
	STDMETHOD(ClearInterval)(UINT intervalID);
	STDMETHOD(SetTimeout)(IDispatch * func, INT delay, [out,retval] UINT * outTimeoutID);
	STDMETHOD(ClearTimeout)(UINT timeoutID);
	STDMETHOD(NotifyOthers)(BSTR name, VARIANT info);
	STDMETHOD(WatchMetadb)(IFbMetadbHandle * handle);
	STDMETHOD(UnWatchMetadb)();
	STDMETHOD(CreateTooltip)([out,retval] __interface IFbTooltip ** pp);
	STDMETHOD(ShowConfigure)();
	STDMETHOD(ShowProperties)();
	STDMETHOD(GetProperty)(BSTR name, [optional] VARIANT defaultval, [out,retval] VARIANT * p);
	STDMETHOD(SetProperty)(BSTR name, VARIANT val);
	STDMETHOD(GetBackgroundImage)([out,retval] IGdiBitmap ** pp);
	STDMETHOD(SetCursor)(UINT id);
	STDMETHOD(GetColorCUI)(UINT type, [defaultvalue("")] BSTR guidstr, [out,retval] int * p);
	STDMETHOD(GetFontCUI)(UINT type, [defaultvalue("")] BSTR guidstr, [out,retval] IGdiFont ** pp);
	STDMETHOD(GetColorDUI)(UINT type, [out,retval] int * p);
	STDMETHOD(GetFontDUI)(UINT type, [out,retval] IGdiFont ** pp);
	STDMETHOD(CreateThemeManager)(BSTR classid, [out,retval] IThemeManager ** pp);
};
_COM_SMARTPTR_TYPEDEF(IFbWindow, __uuidof(IFbWindow));


//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("d53e81cd-0157-4cfe-a618-1F88d48dc0b7")
]
__interface IWSHUtils: IDispatch
{
	STDMETHOD(CheckComponent)(BSTR name, [defaultvalue(-1)] VARIANT_BOOL is_dll, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(CheckFont)(BSTR name, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(GetAlbumArt)(BSTR rawpath, [defaultvalue(0)] int art_id, [defaultvalue(-1)] VARIANT_BOOL need_stub, [out,retval] IGdiBitmap ** pp);
	STDMETHOD(GetAlbumArtV2)(IFbMetadbHandle * handle, [defaultvalue(0)] int art_id, [defaultvalue(-1)] VARIANT_BOOL need_stub, [out,retval] IGdiBitmap **pp);
	STDMETHOD(GetAlbumArtEmbedded)(BSTR rawpath, [defaultvalue(0)] int art_id, [out,retval] IGdiBitmap ** pp);
	STDMETHOD(GetAlbumArtAsync)(UINT window_id, IFbMetadbHandle * handle, [defaultvalue(0)] int art_id, [defaultvalue(-1)] VARIANT_BOOL need_stub, [defaultvalue(0)] VARIANT_BOOL only_embed, [defaultvalue(0)] VARIANT_BOOL no_load, [out,retval] UINT * p);
	STDMETHOD(ReadINI)(BSTR filename, BSTR section, BSTR key, [optional] VARIANT defaultval, [out,retval] BSTR * pp);
	STDMETHOD(WriteINI)(BSTR filename, BSTR section, BSTR key, VARIANT val, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(IsKeyPressed)(UINT vkey, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(PathWildcardMatch)(BSTR pattern, BSTR str, [out,retval] VARIANT_BOOL * p);
	STDMETHOD(ReadTextFile)(BSTR filename, [defaultvalue(0)] UINT codepage, [out,retval] BSTR * pp);
	STDMETHOD(GetSysColor)(UINT index, [out,retval] int * p);
	STDMETHOD(GetSystemMetrics)(UINT index, [out,retval] int * p);
	STDMETHOD(Glob)(BSTR pattern, [defaultvalue(FILE_ATTRIBUTE_DIRECTORY)] UINT exc_mask, [defaultvalue(0xffffffff)] UINT inc_mask, [out,retval] VARIANT * p);
	STDMETHOD(FileTest)(BSTR path, BSTR mode, [out,retval] VARIANT * p);
};
_COM_SMARTPTR_TYPEDEF(IWSHUtils, __uuidof(IWSHUtils));
