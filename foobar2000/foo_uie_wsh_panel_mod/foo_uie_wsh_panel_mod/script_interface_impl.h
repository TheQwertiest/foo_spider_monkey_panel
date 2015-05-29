#pragma once

#include "script_interface.h"
#include "com_tools.h"
#include "helpers.h"

template<class T, class T2>
class GdiObj : public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IGdiObj)
		COM_QI_ENTRY(IDisposable)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
	T2 * m_ptr;

	GdiObj<T, T2>(T2* p) : m_ptr(p)
	{
	}

	virtual ~GdiObj<T, T2>() { }

	virtual void FinalRelease()
	{
		if (m_ptr)
		{
			delete m_ptr;
			m_ptr = NULL;
		}
	}

public:
	// Default Dispose
	STDMETHODIMP Dispose()
	{
		FinalRelease();
		return S_OK;
	}

	STDMETHODIMP get__ptr(void ** pp)
	{
		*pp = m_ptr;
		return S_OK;
	}
};

class GdiFont : public GdiObj<IGdiFont, Gdiplus::Font>
{
protected:
	HFONT m_hFont;
	bool  m_managed;

	GdiFont(Gdiplus::Font* p, HFONT hFont, bool managed = true);

	virtual ~GdiFont() {}

	virtual void FinalRelease();

public:
	STDMETHODIMP get_HFont(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
	STDMETHODIMP get_Name(LANGID langId, BSTR * outName);
	STDMETHODIMP get_Size(float * outSize);
	STDMETHODIMP get_Style(INT * outStyle);
};

class GdiBitmap : public GdiObj<IGdiBitmap, Gdiplus::Bitmap>
{
protected:
	GdiBitmap(Gdiplus::Bitmap* p): GdiObj<IGdiBitmap, Gdiplus::Bitmap>(p) {}

public:
	STDMETHODIMP get_Width(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
	STDMETHODIMP Clone(float x, float y, float w, float h, IGdiBitmap ** pp);
	STDMETHODIMP RotateFlip(UINT mode);
	STDMETHODIMP ApplyAlpha(BYTE alpha, IGdiBitmap ** pp);
	STDMETHODIMP ApplyMask(IGdiBitmap * mask, VARIANT_BOOL * p);
	STDMETHODIMP CreateRawBitmap(IGdiRawBitmap ** pp);
	STDMETHODIMP GetGraphics(IGdiGraphics ** pp);
	STDMETHODIMP ReleaseGraphics(IGdiGraphics * p);
	STDMETHODIMP BoxBlur(int radius, int iteration);
	STDMETHODIMP Resize(UINT w, UINT h, INT interpolationMode, IGdiBitmap ** pp);
	STDMETHODIMP GetColorScheme(UINT count, VARIANT * outArray);
};

class GdiGraphics : public GdiObj<IGdiGraphics, Gdiplus::Graphics>
{
protected:
	GdiGraphics(): GdiObj<IGdiGraphics, Gdiplus::Graphics>(NULL) {}

	static void GetRoundRectPath(Gdiplus::GraphicsPath & gp, Gdiplus::RectF & rect, float arc_width, float arc_height);

public:
	STDMETHODIMP Dispose()
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP put__ptr(void * p);
	STDMETHODIMP FillSolidRect(float x, float y, float w, float h, VARIANT color);
	STDMETHODIMP FillGradRect(float x, float y, float w, float h, float angle, VARIANT color1, VARIANT color2, float focus);
	STDMETHODIMP FillRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, VARIANT color);
	STDMETHODIMP FillEllipse(float x, float y, float w, float h, VARIANT color);
	STDMETHODIMP FillPolygon(VARIANT color, INT fillmode, VARIANT points);

	STDMETHODIMP DrawLine(float x1, float y1, float x2, float y2, float line_width, VARIANT color);
	STDMETHODIMP DrawRect(float x, float y, float w, float h, float line_width, VARIANT color);
	STDMETHODIMP DrawRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT color);
	STDMETHODIMP DrawEllipse(float x, float y, float w, float h, float line_width, VARIANT color);
	STDMETHODIMP DrawPolygon(VARIANT color, float line_width, VARIANT points);

	STDMETHODIMP DrawString(BSTR str, IGdiFont* font, VARIANT color, float x, float y, float w, float h, int flags);
	STDMETHODIMP GdiDrawText(BSTR str, IGdiFont * font, VARIANT color, int x, int y, int w, int h, int format, VARIANT * p);
	STDMETHODIMP DrawImage(IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha);
	STDMETHODIMP GdiDrawBitmap(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH);
	STDMETHODIMP GdiAlphaBlend(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha);
	STDMETHODIMP MeasureString(BSTR str, IGdiFont * font, float x, float y, float w, float h, int flags, IMeasureStringInfo ** pp);
	STDMETHODIMP CalcTextWidth(BSTR str, IGdiFont * font, UINT * p);
	STDMETHODIMP CalcTextHeight(BSTR str, IGdiFont * font, UINT * p);
	STDMETHODIMP EstimateLineWrap(BSTR str, IGdiFont * font, int max_width, VARIANT * p);
	STDMETHODIMP SetTextRenderingHint(UINT mode);
	STDMETHODIMP SetSmoothingMode(INT mode);
	STDMETHODIMP SetInterpolationMode(INT mode);
};

class GdiRawBitmap : public IDisposableImpl4<IGdiRawBitmap>
{
protected:
	UINT m_width, m_height;
	HDC m_hdc;
	HBITMAP m_hbmp, m_hbmpold;

	GdiRawBitmap(Gdiplus::Bitmap * p_bmp);

	virtual ~GdiRawBitmap() { }

	virtual void FinalRelease()
	{
		if (m_hdc)
		{
			SelectBitmap(m_hdc, m_hbmpold);
			DeleteDC(m_hdc);
			m_hdc = NULL;
		}

		if (m_hbmp)
		{
			DeleteBitmap(m_hbmp);
			m_hbmp = NULL;
		}
	}

public:
	STDMETHODIMP get__Handle(HDC * p);
	STDMETHODIMP get_Width(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
	//STDMETHODIMP GetBitmap(IGdiBitmap ** pp);
};

class MeasureStringInfo : public IDispatchImpl3<IMeasureStringInfo>
{
protected:
	float m_x, m_y, m_w, m_h;
	int m_l, m_c;

	MeasureStringInfo(float x, float y, float w, float h, int l, int c) : m_x(x), m_y(y), m_w(w), m_h(h), m_l(l), m_c(c) {}

	virtual ~MeasureStringInfo() {}

public:
	STDMETHODIMP get_x(float * p);
	STDMETHODIMP get_y(float * p);
	STDMETHODIMP get_width(float * p);
	STDMETHODIMP get_height(float * p);
	STDMETHODIMP get_lines(int * p);
	STDMETHODIMP get_chars(int * p);
};

// NOTE: Do not use com_object_impl_t<> to initialize, use com_object_singleton_t<> instead.
class GdiUtils : public IDispatchImpl3<IGdiUtils>
{
protected:
	GdiUtils() {}
	virtual ~GdiUtils() {}

public:
	STDMETHODIMP Font(BSTR name, float pxSize, int style, IGdiFont** pp);
	STDMETHODIMP Image(BSTR path, IGdiBitmap** pp);
	STDMETHODIMP CreateImage(int w, int h, IGdiBitmap ** pp);
	STDMETHODIMP CreateStyleTextRender(VARIANT_BOOL pngmode, IStyleTextRender ** pp);
	STDMETHODIMP LoadImageAsync(UINT window_id, BSTR path, UINT * p);
};

class FbFileInfo : public IDisposableImpl4<IFbFileInfo>
{
protected:
	file_info_impl * m_info_ptr;

	FbFileInfo(file_info_impl * p_info_ptr) : m_info_ptr(p_info_ptr) {}

	virtual ~FbFileInfo() { }

	virtual void FinalRelease()
	{
		if (m_info_ptr)
		{
			delete m_info_ptr;
			m_info_ptr = NULL;
		}
	}

public:
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP get_MetaCount(UINT* p);
	STDMETHODIMP MetaValueCount(UINT idx, UINT* p);
	STDMETHODIMP MetaName(UINT idx, BSTR* pp);
	STDMETHODIMP MetaValue(UINT idx, UINT vidx, BSTR* pp);
	STDMETHODIMP MetaFind(BSTR name, UINT * p);
	STDMETHODIMP MetaRemoveField(BSTR name);
	STDMETHODIMP MetaAdd(BSTR name, BSTR value, UINT * p);
	STDMETHODIMP MetaInsertValue(UINT idx, UINT vidx, BSTR value);
	STDMETHODIMP get_InfoCount(UINT* p);
	STDMETHODIMP InfoName(UINT idx, BSTR* pp);
	STDMETHODIMP InfoValue(UINT idx, BSTR* pp);
	STDMETHODIMP InfoFind(BSTR name, UINT * p);
	STDMETHODIMP MetaSet(BSTR name, BSTR value);
};

class FbMetadbHandle : public IDisposableImpl4<IFbMetadbHandle>
{
protected:
	metadb_handle_ptr m_handle;

	FbMetadbHandle(const metadb_handle_ptr & src) : m_handle(src) {}

	FbMetadbHandle(metadb_handle * src) : m_handle(src) {}

	virtual ~FbMetadbHandle() { }

	virtual void FinalRelease()
	{
		m_handle.release();
	}

public:
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP get_Path(BSTR* pp);
	STDMETHODIMP get_RawPath(BSTR * pp);
	STDMETHODIMP get_SubSong(UINT* p);
	STDMETHODIMP get_FileSize(LONGLONG* p);
	STDMETHODIMP get_Length(double* p);
	STDMETHODIMP GetFileInfo(IFbFileInfo ** pp);
	STDMETHODIMP UpdateFileInfoSimple(SAFEARRAY * p);
	STDMETHODIMP Compare(IFbMetadbHandle * handle, VARIANT_BOOL * p);
};

class FbMetadbHandleList : public IDisposableImpl4<IFbMetadbHandleList>
{
protected:
	metadb_handle_list m_handles;

	FbMetadbHandleList(metadb_handle_list_cref handles) : m_handles(handles) {}

	virtual ~FbMetadbHandleList() {}

	virtual void FinalRelease()
	{
		m_handles.remove_all();
	}

public:
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP get_Item(UINT index, IFbMetadbHandle ** pp);
	STDMETHODIMP put_Item(UINT index, IFbMetadbHandle * handle);
	STDMETHODIMP get_Count(UINT * p);

	STDMETHODIMP Clone(IFbMetadbHandleList ** pp);
	STDMETHODIMP Insert(UINT index, IFbMetadbHandle * handle, UINT * outIndex);
	STDMETHODIMP InsertRange(UINT index, IFbMetadbHandleList * handles, UINT * outIndex);
	STDMETHODIMP Add(IFbMetadbHandle * handle, UINT * p);
	STDMETHODIMP AddRange(IFbMetadbHandleList * handles);
	STDMETHODIMP RemoveById(UINT index);
	STDMETHODIMP Remove(IFbMetadbHandle * handle);
	STDMETHODIMP RemoveRange(UINT from, UINT count);
	STDMETHODIMP RemoveAll();
	STDMETHODIMP Sort();
	STDMETHODIMP Find(IFbMetadbHandle * handle, UINT * p);
	STDMETHODIMP BSearch(IFbMetadbHandle * handle, UINT * p);
	STDMETHODIMP MakeIntersection(IFbMetadbHandleList * handles);
	STDMETHODIMP MakeUnion(IFbMetadbHandleList * handles);
	STDMETHODIMP MakeDifference(IFbMetadbHandleList * handles);
	STDMETHODIMP OrderByFormat(__interface IFbTitleFormat * script, int direction);
	STDMETHODIMP OrderByPath();
	STDMETHODIMP OrderByRelativePath();
};

class FbTitleFormat : public IDisposableImpl4<IFbTitleFormat>
{
protected:
	titleformat_object::ptr m_obj;

	FbTitleFormat(BSTR expr)
	{
		pfc::stringcvt::string_utf8_from_wide utf8 = expr;
		static_api_ptr_t<titleformat_compiler>()->compile_safe(m_obj, utf8);
	}

	virtual ~FbTitleFormat() {}

	virtual void FinalRelease()
	{
		m_obj.release();
	}

public:
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP Eval(VARIANT_BOOL force, BSTR * pp);
	STDMETHODIMP EvalWithMetadb(IFbMetadbHandle * handle, BSTR * pp);
};

class ContextMenuManager : public IDisposableImpl4<IContextMenuManager>
{
protected:
	contextmenu_manager::ptr m_cm;

	ContextMenuManager() {}
	virtual ~ContextMenuManager() {}

	virtual void FinalRelease()
	{
		m_cm.release();
	}

public:
	STDMETHODIMP InitContext(VARIANT handle);
	STDMETHODIMP InitNowPlaying();
	STDMETHODIMP BuildMenu(IMenuObj * p, int base_id, int max_id);
	STDMETHODIMP ExecuteByID(UINT id, VARIANT_BOOL * p);
};

class MainMenuManager : public IDisposableImpl4<IMainMenuManager>
{
protected:
	mainmenu_manager::ptr m_mm;

	MainMenuManager() {}
	virtual ~MainMenuManager() {}

	virtual void FinalRelease()
	{
		m_mm.release();
	}

public:
	STDMETHODIMP Init(BSTR root_name);
	STDMETHODIMP BuildMenu(IMenuObj * p, int base_id, int count);
	STDMETHODIMP ExecuteByID(UINT id, VARIANT_BOOL * p);
};

class FbProfiler: public IDispatchImpl3<IFbProfiler>
{
protected:
	pfc::string_simple m_name;
	helpers::mm_timer m_timer;

	FbProfiler(const char * p_name) : m_name(p_name) { m_timer.start(); }
	virtual ~FbProfiler() {}

public:
	STDMETHODIMP Reset();
	STDMETHODIMP Print();
	STDMETHODIMP get_Time(INT * p);
};

class FbUiSelectionHolder : public IDisposableImpl4<IFbUiSelectionHolder>
{
protected:
	ui_selection_holder::ptr m_holder;

	FbUiSelectionHolder(const ui_selection_holder::ptr & holder) : m_holder(holder) {}
	virtual ~FbUiSelectionHolder() {}

	virtual void FinalRelease() 
	{
		m_holder.release();
	}

public:
	STDMETHODIMP SetSelection(IFbMetadbHandleList * handles);
	STDMETHODIMP SetPlaylistSelectionTracking();
	STDMETHODIMP SetPlaylistTracking();
};

// NOTE: Do not use com_object_impl_t<> to initialize, use com_object_singleton_t<> instead.
class FbUtils : public IDispatchImpl3<IFbUtils>
{
protected:
	FbUtils() {}
	virtual ~FbUtils() {}

public:
	STDMETHODIMP trace(SAFEARRAY * p);
	STDMETHODIMP ShowPopupMessage(BSTR msg, BSTR title, int iconid);
	STDMETHODIMP CreateProfiler(BSTR name, IFbProfiler ** pp);
	STDMETHODIMP TitleFormat(BSTR expression, IFbTitleFormat** pp);
	STDMETHODIMP GetNowPlaying(IFbMetadbHandle** pp);
	STDMETHODIMP GetFocusItem(VARIANT_BOOL force, IFbMetadbHandle** pp);
	STDMETHODIMP GetSelection(IFbMetadbHandle** pp);
	STDMETHODIMP GetSelections(UINT flags, IFbMetadbHandleList ** pp);
	STDMETHODIMP GetSelectionType(UINT* p);
	STDMETHODIMP AcquireUiSelectionHolder(IFbUiSelectionHolder ** outHolder);

	STDMETHODIMP get_ComponentPath(BSTR* pp);
	STDMETHODIMP get_FoobarPath(BSTR* pp);
	STDMETHODIMP get_ProfilePath(BSTR* pp);
	STDMETHODIMP get_IsPlaying(VARIANT_BOOL* p);
	STDMETHODIMP get_IsPaused(VARIANT_BOOL* p);
	STDMETHODIMP get_PlaybackTime(double* p);
	STDMETHODIMP put_PlaybackTime(double time);
	STDMETHODIMP get_PlaybackLength(double* p);
	STDMETHODIMP get_PlaybackOrder(UINT* p);
	STDMETHODIMP put_PlaybackOrder(UINT order);
	STDMETHODIMP get_StopAfterCurrent(VARIANT_BOOL * p);
	STDMETHODIMP put_StopAfterCurrent(VARIANT_BOOL p);
	STDMETHODIMP get_CursorFollowPlayback(VARIANT_BOOL * p);
	STDMETHODIMP put_CursorFollowPlayback(VARIANT_BOOL p);
	STDMETHODIMP get_PlaybackFollowCursor(VARIANT_BOOL * p);
	STDMETHODIMP put_PlaybackFollowCursor(VARIANT_BOOL p);
	STDMETHODIMP get_Volume(float* p);
	STDMETHODIMP put_Volume(float value);

	STDMETHODIMP Exit();
	STDMETHODIMP Play();
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP PlayOrPause();
	STDMETHODIMP Random();
	STDMETHODIMP Next();
	STDMETHODIMP Prev();
	STDMETHODIMP VolumeDown();
	STDMETHODIMP VolumeUp();
	STDMETHODIMP VolumeMute();
	STDMETHODIMP AddDirectory();
	STDMETHODIMP AddFiles();
	STDMETHODIMP ShowConsole();
	STDMETHODIMP ShowPreferences();
	STDMETHODIMP ClearPlaylist();
	STDMETHODIMP LoadPlaylist();
	STDMETHODIMP SavePlaylist();
	STDMETHODIMP RunMainMenuCommand(BSTR command, VARIANT_BOOL * p);
	STDMETHODIMP RunContextCommand(BSTR command, UINT flags, VARIANT_BOOL * p);
	STDMETHODIMP RunContextCommandWithMetadb(BSTR command, VARIANT handle, UINT flags, VARIANT_BOOL * p);
	STDMETHODIMP CreateContextMenuManager(IContextMenuManager ** pp);
	STDMETHODIMP CreateMainMenuManager(IMainMenuManager ** pp);
	STDMETHODIMP IsMetadbInMediaLibrary(IFbMetadbHandle * handle, VARIANT_BOOL * p);
	STDMETHODIMP IsLibraryEnabled(VARIANT_BOOL * p);

	STDMETHODIMP get_ActivePlaylist(UINT * p);
	STDMETHODIMP put_ActivePlaylist(UINT idx);
	STDMETHODIMP get_PlayingPlaylist(UINT * p);
	STDMETHODIMP put_PlayingPlaylist(UINT idx);
	STDMETHODIMP get_PlaylistCount(UINT * p);
	STDMETHODIMP get_PlaylistItemCount(UINT idx, UINT * p);
	STDMETHODIMP GetPlaylistName(UINT idx, BSTR * p);
	STDMETHODIMP CreatePlaylist(UINT idx, BSTR name, UINT * p);
	STDMETHODIMP RemovePlaylist(UINT idx, VARIANT_BOOL * p);
	STDMETHODIMP MovePlaylist(UINT from, UINT to, VARIANT_BOOL * p);
	STDMETHODIMP RenamePlaylist(UINT idx, BSTR name, VARIANT_BOOL * p);
	STDMETHODIMP DuplicatePlaylist(UINT from, BSTR name, UINT * p);
	STDMETHODIMP IsAutoPlaylist(UINT idx, VARIANT_BOOL * p);
	STDMETHODIMP CreateAutoPlaylist(UINT idx, BSTR name, BSTR query, BSTR sort, UINT flags, UINT * p);
	STDMETHODIMP ShowAutoPlaylistUI(UINT idx, VARIANT_BOOL * p);
	STDMETHODIMP ShowLibrarySearchUI(BSTR query);
	STDMETHODIMP GetLibraryItems(IFbMetadbHandleList ** outItems);
};

class MenuObj : public IDisposableImpl4<IMenuObj>
{
protected:
	HMENU m_hMenu;
	HWND  m_wnd_parent;
	bool  m_has_detached;

	MenuObj(HWND wnd_parent) : m_wnd_parent(wnd_parent), m_has_detached(false)
	{
		m_hMenu = ::CreatePopupMenu();
	}

	virtual ~MenuObj() { }

	virtual void FinalRelease()
	{
		if (!m_has_detached && m_hMenu && IsMenu(m_hMenu))
		{
			DestroyMenu(m_hMenu);
			m_hMenu = NULL;
		}
	}

public:
	STDMETHODIMP get_ID(UINT * p);
	STDMETHODIMP AppendMenuItem(UINT flags, UINT item_id, BSTR text);
	STDMETHODIMP AppendMenuSeparator();
	STDMETHODIMP EnableMenuItem(UINT id_or_pos, UINT enable, VARIANT_BOOL bypos);
	STDMETHODIMP CheckMenuItem(UINT id_or_pos, VARIANT_BOOL check, VARIANT_BOOL bypos);
	STDMETHODIMP CheckMenuRadioItem(UINT first, UINT last, UINT check, VARIANT_BOOL bypos);
	STDMETHODIMP TrackPopupMenu(int x, int y, UINT flags, UINT * item_id);
	STDMETHODIMP AppendTo(IMenuObj * parent, UINT flags, BSTR text);
};

class TimerObj : public IDisposableImpl4<ITimerObj>
{
protected:
	UINT m_id;

	TimerObj(UINT id) : m_id(id) {}

	virtual ~TimerObj() { }

	virtual void FinalRelease()
	{		
		if (m_id != 0)
		{
			timeKillEvent(m_id);
			m_id = 0;
		}
	}

public:
	STDMETHODIMP get_ID(UINT * p);
};

// NOTE: Do not use com_object_impl_t<> to initialize, use com_object_singleton_t<> instead.
class WSHUtils : public IDispatchImpl3<IWSHUtils>
{
protected:
	WSHUtils() {}
	virtual ~WSHUtils() {}

public:
	STDMETHODIMP CheckComponent(BSTR name, VARIANT_BOOL is_dll, VARIANT_BOOL * p);
	STDMETHODIMP CheckFont(BSTR name, VARIANT_BOOL * p);
	STDMETHODIMP GetAlbumArtV2(IFbMetadbHandle * handle, int art_id, VARIANT_BOOL need_stub, IGdiBitmap **pp);
	STDMETHODIMP GetAlbumArtEmbedded(BSTR rawpath, int art_id, IGdiBitmap ** pp);
	STDMETHODIMP GetAlbumArtAsync(UINT window_id, IFbMetadbHandle * handle, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, VARIANT_BOOL no_load, UINT * p);
	STDMETHODIMP ReadINI(BSTR filename, BSTR section, BSTR key, VARIANT defaultval, BSTR * pp);
	STDMETHODIMP WriteINI(BSTR filename, BSTR section, BSTR key, VARIANT val, VARIANT_BOOL * p);
	STDMETHODIMP IsKeyPressed(UINT vkey, VARIANT_BOOL * p);
	STDMETHODIMP PathWildcardMatch(BSTR pattern, BSTR str, VARIANT_BOOL * p);
	STDMETHODIMP ReadTextFile(BSTR filename, UINT codepage, BSTR * pp);
	STDMETHODIMP GetSysColor(UINT index, int * p);
	STDMETHODIMP GetSystemMetrics(UINT index, INT * p);
	STDMETHODIMP Glob(BSTR pattern, UINT exc_mask, UINT inc_mask, VARIANT * p);
	STDMETHODIMP FileTest(BSTR path, BSTR mode, VARIANT * p);
};

// forward declaration
namespace TextDesign
{
	class IOutlineText;
}

class StyleTextRender : public IDisposableImpl4<IStyleTextRender>
{
protected:
	TextDesign::IOutlineText * m_pOutLineText;
	bool m_pngmode;

	StyleTextRender(bool pngmode);
	virtual ~StyleTextRender() {}

	virtual void FinalRelease();

public:
	// Outline
	STDMETHODIMP OutLineText(int text_color, int outline_color, int outline_width);
	STDMETHODIMP DoubleOutLineText(int text_color, int outline_color1, int outline_color2, int outline_width1, int outline_width2);
	STDMETHODIMP GlowText(int text_color, int glow_color, int glow_width);
	// Shadow
	STDMETHODIMP EnableShadow(VARIANT_BOOL enable);
	STDMETHODIMP ResetShadow();
	STDMETHODIMP Shadow(VARIANT color, int thickness, int offset_x, int offset_y);
	STDMETHODIMP DiffusedShadow(VARIANT color, int thickness, int offset_x, int offset_y);
	STDMETHODIMP SetShadowBackgroundColor(VARIANT color, int width, int height);
	STDMETHODIMP SetShadowBackgroundImage(IGdiBitmap * img);
	// Render 
	STDMETHODIMP RenderStringPoint(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, int flags, VARIANT_BOOL * p);
	STDMETHODIMP RenderStringRect(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, int w, int h, int flags, VARIANT_BOOL * p);
	// PNG Mode only
	STDMETHODIMP SetPngImage(IGdiBitmap * img);
};

class ThemeManager : public IDisposableImpl4<IThemeManager>
{
protected:
	HTHEME m_theme;
	int m_partid;
	int m_stateid;

	ThemeManager(HWND hwnd, BSTR classlist) : m_theme(NULL), m_partid(0), m_stateid(0)
	{
		m_theme = OpenThemeData(hwnd, classlist);

		if (!m_theme) throw pfc::exception_invalid_params();
	}

	virtual ~ThemeManager() {}

	virtual void FinalRelease()
	{
		if (m_theme)
		{
			CloseThemeData(m_theme);
			m_theme = NULL;
		}
	}

public:
	STDMETHODIMP SetPartAndStateID(int partid, int stateid);
	STDMETHODIMP IsThemePartDefined(int partid, int stateid, VARIANT_BOOL * p);
	STDMETHODIMP DrawThemeBackground(IGdiGraphics * gr, int x, int y, int w, int h, int clip_x, int clip_y, int clip_w, int clip_h);
};

class DropSourceAction : public IDisposableImpl4<IDropSourceAction>
{
public:
	enum t_action_mode
	{
		kActionModeNone = 0,
		kActionModePlaylist,
		kActionModeFilenames,
	};

protected:
	// -1 means active playlist
	int m_playlist_idx;
	t_action_mode m_action_mode;
	bool m_to_select;
	bool m_parsable;

	DropSourceAction() { Reset(); }
	virtual ~DropSourceAction() {}

	virtual void FinalRelease() {}

public:
	inline void Reset()
	{
		m_playlist_idx = -1;
		m_to_select = true;
		m_action_mode = kActionModeNone;
		m_parsable = false;
	}

	inline t_action_mode & Mode() { return m_action_mode; }
	inline bool & Parsable() { return m_parsable; }
	inline int & Playlist() { return m_playlist_idx; }
	inline bool & ToSelect() { return m_to_select; }

public:
	STDMETHODIMP get_Parsable(VARIANT_BOOL * parsable);
	STDMETHODIMP put_Parsable(VARIANT_BOOL parsable);
	STDMETHODIMP get_Mode(int * mode);
	STDMETHODIMP get_Playlist(int * id);
	STDMETHODIMP put_Playlist(int id);
	STDMETHODIMP get_ToSelect(VARIANT_BOOL * to_select);
	STDMETHODIMP put_ToSelect(VARIANT_BOOL to_select);

	STDMETHODIMP ToPlaylist();
};
