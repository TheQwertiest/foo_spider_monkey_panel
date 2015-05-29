#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_playlist_impl.h"
#include "helpers.h"
#include "com_array.h"
#include "gdiplus_helpers.h"
#include "boxblurfilter.h"
#include "user_message.h"
#include "popup_msg.h"
#include "dbgtrace.h"
#include "obsolete.h"
#include "../TextDesinger/OutlineText.h"
#include "../TextDesinger/PngOutlineText.h"
#include <map>
#include <vector>
#include <algorithm>


// Helper functions
// 
// -1: not a valid metadb interface
//  0: metadb interface
//  1: metadb handles interface
static int TryGetMetadbHandleFromVariant(const VARIANT & obj, IDispatch ** ppuk)
{
	if (obj.vt != VT_DISPATCH || !obj.pdispVal)
		return -1;

	IDispatch * temp = NULL;

	if (SUCCEEDED(obj.pdispVal->QueryInterface(__uuidof(IFbMetadbHandle), (void **)&temp)))
	{
		*ppuk = temp;
		return 0;
	}
	else if (SUCCEEDED(obj.pdispVal->QueryInterface(__uuidof(IFbMetadbHandleList), (void **)&temp)))
	{
		*ppuk = temp;
		return 1;
	}

	return -1;
}

static inline unsigned ExtractColorFromVariant(VARIANT v)
{
	return (v.vt == VT_R8) ? static_cast<unsigned>(v.dblVal) : v.lVal;
}

GdiFont::GdiFont(Gdiplus::Font* p, HFONT hFont, bool managed /*= true*/) : GdiObj<IGdiFont, Gdiplus::Font>(p), 
	m_hFont(hFont), m_managed(managed)
{

}

void GdiFont::FinalRelease()
{
	if (m_hFont && m_managed)
	{
		DeleteFont(m_hFont);
		m_hFont = NULL;
	}

	// call parent
	GdiObj<IGdiFont, Gdiplus::Font>::FinalRelease();
}

STDMETHODIMP GdiFont::get_HFont(UINT* p)
{
	TRACK_FUNCTION();

	if (!p || !m_ptr) return E_POINTER;

	*p = (UINT)m_hFont;
	return S_OK;
}

STDMETHODIMP GdiFont::get_Height(UINT* p)
{
	TRACK_FUNCTION();

	if (!p || !m_ptr) return E_POINTER;

	Gdiplus::Bitmap img(1, 1, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(&img);

	*p = (UINT)m_ptr->GetHeight(&g);
	return S_OK;
}

STDMETHODIMP GdiFont::get_Name(LANGID langId, BSTR * outName)
{
	TRACK_FUNCTION();

	if (!outName || !m_ptr) return E_POINTER;

	Gdiplus::FontFamily fontFamily;
	WCHAR name[LF_FACESIZE] = {0};
	m_ptr->GetFamily(&fontFamily);
	fontFamily.GetFamilyName(name, langId);
	(*outName) = SysAllocString(name);
	return S_OK;
}

STDMETHODIMP GdiFont::get_Size(float * outSize)
{
	TRACK_FUNCTION();

	if (!outSize || !m_ptr) return E_POINTER;

	(*outSize) = m_ptr->GetSize();
	return S_OK;
}

STDMETHODIMP GdiFont::get_Style(INT * outStyle)
{
	TRACK_FUNCTION();

	if (!outStyle || !m_ptr) return E_POINTER;

	(*outStyle) = m_ptr->GetStyle();
	return S_OK;
}


STDMETHODIMP GdiBitmap::get_Width(UINT* p)
{
	TRACK_FUNCTION();

	if (!p || !m_ptr) return E_POINTER;

	*p = m_ptr->GetWidth();
	return S_OK;
}

STDMETHODIMP GdiBitmap::get_Height(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (!m_ptr) return E_POINTER;

	*p = m_ptr->GetHeight();
	return S_OK;
}

STDMETHODIMP GdiBitmap::Clone(float x, float y, float w, float h, IGdiBitmap** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!m_ptr) return E_POINTER;

	Gdiplus::Bitmap * img = m_ptr->Clone(x, y, w, h, PixelFormat32bppPARGB);

	if (!helpers::ensure_gdiplus_object(img))
	{
		if (img) delete img;
		(*pp) = NULL;
		return S_OK;
	}

	(*pp) = new com_object_impl_t<GdiBitmap>(img);
	return S_OK;
}

STDMETHODIMP GdiBitmap::RotateFlip(UINT mode)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	m_ptr->RotateFlip((Gdiplus::RotateFlipType)mode);
	return S_OK;
}

STDMETHODIMP GdiBitmap::ApplyAlpha(BYTE alpha, IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!m_ptr) return E_POINTER;

	UINT width = m_ptr->GetWidth();
	UINT height = m_ptr->GetHeight();
	Gdiplus::Bitmap * out = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(out);
	Gdiplus::ImageAttributes ia;
	Gdiplus::ColorMatrix cm = { 0.0 };
	Gdiplus::Rect rc;

	cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0;
	cm.m[3][3] = static_cast<float>(alpha) / 255;
	ia.SetColorMatrix(&cm);

	rc.X = rc.Y = 0;
	rc.Width = width;
	rc.Height = height;

	g.DrawImage(m_ptr, rc, 0, 0, width, height, Gdiplus::UnitPixel, &ia);

	(*pp) = new com_object_impl_t<GdiBitmap>(out);
	return S_OK;
}

STDMETHODIMP GdiBitmap::ApplyMask(IGdiBitmap * mask, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	*p = VARIANT_FALSE;

	if (!m_ptr) return E_POINTER;
	if (!mask) return E_INVALIDARG;

	Gdiplus::Bitmap * bitmap_mask = NULL;
	mask->get__ptr((void**)&bitmap_mask);

	if (!bitmap_mask || bitmap_mask->GetHeight() != m_ptr->GetHeight() || bitmap_mask->GetWidth() != m_ptr->GetWidth())
	{
		return E_INVALIDARG;
	}

	Gdiplus::Rect rect(0, 0, m_ptr->GetWidth(), m_ptr->GetHeight());
	Gdiplus::BitmapData bmpdata_mask = { 0 }, bmpdata_dst = { 0 };

	if (bitmap_mask->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata_mask) != Gdiplus::Ok)
	{
		return S_OK;
	}

	if (m_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bmpdata_dst) != Gdiplus::Ok)
	{
		bitmap_mask->UnlockBits(&bmpdata_mask);
		return S_OK;
	}

	const int width = rect.Width;
	const int height = rect.Height;
	const int size = width * height;
	//const int size_threshold = 512;
	t_uint32 * p_mask = reinterpret_cast<t_uint32 *>(bmpdata_mask.Scan0);
	t_uint32 * p_dst = reinterpret_cast<t_uint32 *>(bmpdata_dst.Scan0);
	const t_uint32 * p_mask_end = p_mask + rect.Width * rect.Height;
	t_uint32 alpha;

	while (p_mask < p_mask_end)
	{
		// Method 1:
		//alpha = (~*p_mask & 0xff) * (*p_dst >> 24) + 0x80;
		//*p_dst = ((((alpha >> 8) + alpha) & 0xff00) << 16) | (*p_dst & 0xffffff);
		// Method 2
		alpha = (((~*p_mask & 0xff) * (*p_dst >> 24)) << 16) & 0xff000000;
		*p_dst = alpha | (*p_dst & 0xffffff);

		++p_mask;
		++p_dst;
	}

	m_ptr->UnlockBits(&bmpdata_dst);
	bitmap_mask->UnlockBits(&bmpdata_mask);

	*p = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP GdiBitmap::CreateRawBitmap(IGdiRawBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!m_ptr) return E_POINTER;

	(*pp) = new com_object_impl_t<GdiRawBitmap>(m_ptr);
	return S_OK;
}

STDMETHODIMP GdiBitmap::GetGraphics(IGdiGraphics ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!m_ptr) return E_POINTER;

	Gdiplus::Graphics * g = new Gdiplus::Graphics(m_ptr);

	(*pp) = new com_object_impl_t<GdiGraphics>();
	(*pp)->put__ptr(g);
	return S_OK;
}

STDMETHODIMP GdiBitmap::ReleaseGraphics(IGdiGraphics * p)
{
	TRACK_FUNCTION();

	if (p)
	{
		Gdiplus::Graphics * g = NULL;
		p->get__ptr((void**)&g);
		p->put__ptr(NULL);
		if (g) delete g;
	}

	return S_OK;
}

STDMETHODIMP GdiBitmap::BoxBlur(int radius, int iteration)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	box_blur_filter bbf;

	bbf.set_op(radius, iteration);
	bbf.filter(*m_ptr);

	return S_OK;
}

STDMETHODIMP GdiBitmap::Resize(UINT w, UINT h, INT interpolationMode, IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!pp) return E_POINTER;

	Gdiplus::Bitmap * bitmap = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB); 
	Gdiplus::Graphics g(bitmap);

	g.SetInterpolationMode((Gdiplus::InterpolationMode)interpolationMode);
	g.DrawImage(m_ptr, 0, 0, w, h);

	(*pp) = new com_object_impl_t<GdiBitmap>(bitmap);
	return S_OK;
}

STDMETHODIMP GdiBitmap::GetColorScheme(UINT count, VARIANT * outArray)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!count) return E_INVALIDARG;

	Gdiplus::BitmapData bmpdata;
	Gdiplus::Rect rect(0, 0, m_ptr->GetWidth(), m_ptr->GetHeight());

	if (m_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata) != Gdiplus::Ok)
		return E_POINTER;

	std::map<unsigned, int> color_counters;
	const unsigned colors_length = bmpdata.Width * bmpdata.Height;
	const t_uint32 *colors = (const t_uint32 *)bmpdata.Scan0;

	for (unsigned i = 0; i < colors_length; i++)
	{
		// format: 0xaarrggbb
		unsigned color = colors[i];
		unsigned r = (color >> 16) & 0xff;
		unsigned g = (color >> 8) & 0xff;
		unsigned b = (color) & 0xff;

		// Round colors
		r = (r + 15) & 0xffffffe0;
		g = (g + 15) & 0xffffffe0;
		b = (b + 15) & 0xffffffe0;

		if (r > 0xff) r = 0xf0;
		if (g > 0xff) g = 0xf0;
		if (b > 0xff) b = 0xf0;

		++color_counters[Gdiplus::Color::MakeARGB(0xff, r, g, b)];
	}

	m_ptr->UnlockBits(&bmpdata);

	// Sorting
	typedef std::pair<unsigned, int> sort_vec_pair_t;
	std::vector<sort_vec_pair_t> sort_vec(color_counters.begin(), color_counters.end());
	color_counters.clear();
	count = min(count, sort_vec.size());
	std::partial_sort(sort_vec.begin(), sort_vec.begin() + count, sort_vec.end(), 
		[](const sort_vec_pair_t &a, const sort_vec_pair_t &b)
		{
			return a.second > b.second;
		});

	helpers::com_array_writer<> helper;
	if (!helper.create(count)) 
		return E_OUTOFMEMORY;
	for (long i = 0; i < helper.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_UI4;
		var.ulVal = sort_vec[i].first;

		if (FAILED(helper.put(i, var)))
		{
			helper.reset();
			return E_OUTOFMEMORY;
		}
	}

	outArray->vt = VT_ARRAY | VT_VARIANT;
	outArray->parray = helper.get_ptr();
	return S_OK;
}

void GdiGraphics::GetRoundRectPath(Gdiplus::GraphicsPath & gp, Gdiplus::RectF & rect, float arc_width, float arc_height)
{
	TRACK_FUNCTION();

	float arc_dia_w = arc_width * 2;
	float arc_dia_h = arc_height * 2;
	Gdiplus::RectF corner(rect.X, rect.Y, arc_dia_w, arc_dia_h);

	gp.Reset();

	// top left
	gp.AddArc(corner, 180, 90);

	// top right
	corner.X += (rect.Width - arc_dia_w);
	gp.AddArc(corner, 270, 90);

	// bottom right
	corner.Y += (rect.Height - arc_dia_h);
	gp.AddArc(corner, 0, 90);

	// bottom left
	corner.X -= (rect.Width - arc_dia_w);
	gp.AddArc(corner, 90, 90);

	gp.CloseFigure();
}

STDMETHODIMP GdiGraphics::put__ptr(void * p)
{
	TRACK_FUNCTION();

	m_ptr = (Gdiplus::Graphics *)p;
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillSolidRect(float x, float y, float w, float h, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush brush(ExtractColorFromVariant(color));
	m_ptr->FillRectangle(&brush, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillGradRect(float x, float y, float w, float h, float angle, VARIANT color1, VARIANT color2, float focus)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::RectF rect(x, y, w, h);
	// HACK: Workaround for one pixel black line problem.
	//rect.Inflate(0.1f, 0.1f);
	Gdiplus::LinearGradientBrush brush(rect, ExtractColorFromVariant(color1), ExtractColorFromVariant(color2), angle, TRUE);

	brush.SetBlendTriangularShape(focus);
	m_ptr->FillRectangle(&brush, rect);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	// First, check parameters
	if (2 * arc_width > w || 2 * arc_height > h)
		return E_INVALIDARG;

	Gdiplus::SolidBrush br(ExtractColorFromVariant(color));
	Gdiplus::GraphicsPath gp;
	Gdiplus::RectF rect(x, y, w, h);
	GetRoundRectPath(gp, rect, arc_width, arc_height);
	m_ptr->FillPath(&br, &gp);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillEllipse(float x, float y, float w, float h, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush br(ExtractColorFromVariant(color));
	m_ptr->FillEllipse(&br, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillPolygon(VARIANT color, INT fillmode, VARIANT points)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush br(ExtractColorFromVariant(color));
	helpers::com_array_reader helper;
	pfc::array_t<Gdiplus::PointF> point_array;

	if (!helper.convert(&points)) return E_INVALIDARG;
	if ((helper.get_count() % 2) != 0) return E_INVALIDARG;

	point_array.set_count(helper.get_count() >> 1);

	for (long i = 0; i < static_cast<long>(point_array.get_count()); ++i)
	{
		_variant_t varX, varY;

		helper.get_item(i * 2, varX);
		helper.get_item(i * 2 + 1, varY);

		if (FAILED(VariantChangeType(&varX, &varX, 0, VT_R4))) return E_INVALIDARG;
		if (FAILED(VariantChangeType(&varY, &varY, 0, VT_R4))) return E_INVALIDARG;

		point_array[i].X = varX.fltVal;
		point_array[i].Y = varY.fltVal;
	}

	m_ptr->FillPolygon(&br, point_array.get_ptr(), point_array.get_count(), (Gdiplus::FillMode)fillmode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawLine(float x1, float y1, float x2, float y2, float line_width, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::Pen pen(ExtractColorFromVariant(color), line_width);
	m_ptr->DrawLine(&pen, x1, y1, x2, y2);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawRect(float x, float y, float w, float h, float line_width, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::Pen pen(ExtractColorFromVariant(color), line_width);
	m_ptr->DrawRectangle(&pen, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	// First, check parameters
	if (2 * arc_width > w || 2 * arc_height > h)
		return E_INVALIDARG;

	Gdiplus::Pen pen(ExtractColorFromVariant(color), line_width);
	Gdiplus::GraphicsPath gp;
	Gdiplus::RectF rect(x, y, w, h);
	GetRoundRectPath(gp, rect, arc_width, arc_height);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap(Gdiplus::LineCapRound);
	m_ptr->DrawPath(&pen, &gp);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawEllipse(float x, float y, float w, float h, float line_width, VARIANT color)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::Pen pen(ExtractColorFromVariant(color), line_width);
	m_ptr->DrawEllipse(&pen, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawPolygon(VARIANT color, float line_width, VARIANT points)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush br(ExtractColorFromVariant(color));
	helpers::com_array_reader helper;
	pfc::array_t<Gdiplus::PointF> point_array;

	if (!helper.convert(&points)) return E_INVALIDARG;
	if ((helper.get_count() % 2) != 0) return E_INVALIDARG;

	point_array.set_count(helper.get_count() >> 1);

	for (long i = 0; i < static_cast<long>(point_array.get_count()); ++i)
	{
		_variant_t varX, varY;

		helper.get_item(i * 2, varX);
		helper.get_item(i * 2 + 1, varY);

		if (FAILED(VariantChangeType(&varX, &varX, 0, VT_R4))) return E_INVALIDARG;
		if (FAILED(VariantChangeType(&varY, &varY, 0, VT_R4))) return E_INVALIDARG;

		point_array[i].X = varX.fltVal;
		point_array[i].Y = varY.fltVal;
	}

	Gdiplus::Pen pen(ExtractColorFromVariant(color), line_width);
	m_ptr->DrawPolygon(&pen, point_array.get_ptr(), point_array.get_count());
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawString(BSTR str, IGdiFont* font, VARIANT color, float x, float y, float w, float h, int flags)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!str || !font) return E_INVALIDARG;

	Gdiplus::Font* fn = NULL;
	font->get__ptr((void**)&fn);
	if (!fn) return E_INVALIDARG;

	Gdiplus::SolidBrush br(ExtractColorFromVariant(color));
	Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());

	if (flags != 0)
	{
		fmt.SetAlignment((Gdiplus::StringAlignment)((flags >> 28) & 0x3));      //0xf0000000
		fmt.SetLineAlignment((Gdiplus::StringAlignment)((flags >> 24) & 0x3));  //0x0f000000
		fmt.SetTrimming((Gdiplus::StringTrimming)((flags >> 20) & 0x7));        //0x00f00000
		fmt.SetFormatFlags((Gdiplus::StringFormatFlags)(flags & 0x7FFF));       //0x0000ffff
	}

	m_ptr->DrawString(str, -1, fn, Gdiplus::RectF(x, y, w, h), &fmt, &br);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawImage(IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!image) return E_INVALIDARG;

	Gdiplus::Bitmap* img = NULL;
	image->get__ptr((void**)&img);
	if (!img) return E_INVALIDARG;

	Gdiplus::Matrix old_m;

	if (angle != 0.0)
	{
		Gdiplus::Matrix m;
		Gdiplus::RectF rect;
		Gdiplus::PointF pt;

		pt.X = dstX + dstW / 2;
		pt.Y = dstY + dstH / 2;
		m.RotateAt(angle, pt);
		m_ptr->GetTransform(&old_m);
		m_ptr->SetTransform(&m);
	}

	if (alpha != (BYTE)~0)
	{
		Gdiplus::ImageAttributes ia;
		Gdiplus::ColorMatrix cm = { 0.0f };

		cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
		cm.m[3][3] = static_cast<float>(alpha) / 255;

		ia.SetColorMatrix(&cm);

		m_ptr->DrawImage(img, Gdiplus::RectF(dstX, dstY, dstW, dstH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, &ia);
	}
	else
	{
		m_ptr->DrawImage(img, Gdiplus::RectF(dstX, dstY, dstW, dstH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel);
	}

	if (angle != 0.0)
	{
		m_ptr->SetTransform(&old_m);
	}

	return S_OK;
}

STDMETHODIMP GdiGraphics::GdiDrawBitmap(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!bitmap) return E_INVALIDARG;

	HDC src_dc = NULL;
	bitmap->get__Handle(&src_dc);
	if (!src_dc) return E_INVALIDARG;

	HDC dc = m_ptr->GetHDC();

	if (dstW == srcW && dstH == srcH)
	{
		BitBlt(dc, dstX, dstY, dstW, dstH, src_dc, srcX, srcY, SRCCOPY);
	}
	else
	{
		SetStretchBltMode(dc, HALFTONE);
		SetBrushOrgEx(dc, 0, 0, NULL);
		StretchBlt(dc, dstX, dstY, dstW, dstH, src_dc, srcX, srcY, srcW, srcH, SRCCOPY);
	}

	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::GdiAlphaBlend(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!bitmap) return E_INVALIDARG;

	HDC src_dc = NULL;
	bitmap->get__Handle(&src_dc);
	if (!src_dc) return E_INVALIDARG;

	HDC dc = m_ptr->GetHDC();
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };

	::GdiAlphaBlend(dc, dstX, dstY, dstW, dstH, src_dc, srcX, srcY, srcW, srcH, bf);
	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::GdiDrawText(BSTR str, IGdiFont * font, VARIANT color, int x, int y, int w, int h, int format, VARIANT * p)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!str || !font) return E_INVALIDARG;
	if (!p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	if (!hFont) return E_INVALIDARG;

	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();
	RECT rc = { x, y, x + w, y + h };
	DRAWTEXTPARAMS dpt = { sizeof(DRAWTEXTPARAMS), 4, 0, 0, -1 };

	oldfont = SelectFont(dc, hFont);
	SetTextColor(dc, helpers::convert_argb_to_colorref(ExtractColorFromVariant(color)));
	SetBkMode(dc, TRANSPARENT);
	SetTextAlign(dc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

	// Remove DT_MODIFYSTRING flag
	if (format & DT_MODIFYSTRING)
		format &= ~DT_MODIFYSTRING;

	// Well, magic :P
	if (format & DT_CALCRECT)
	{
		RECT rc_calc = {0}, rc_old = {0};

		memcpy(&rc_calc, &rc, sizeof(RECT));
		memcpy(&rc_old, &rc, sizeof(RECT));

		DrawText(dc, str, -1, &rc_calc, format);

		format &= ~DT_CALCRECT;

		// adjust vertical align
		if (format & DT_VCENTER)
		{
			rc.top = rc_old.top + (((rc_old.bottom - rc_old.top) - (rc_calc.bottom - rc_calc.top)) >> 1);
			rc.bottom = rc.top + (rc_calc.bottom - rc_calc.top);
		}
		else if (format & DT_BOTTOM)
		{
			rc.top = rc_old.bottom - (rc_calc.bottom - rc_calc.top);
		}
	}

	DrawTextEx(dc, str, -1, &rc, format, &dpt);

	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);

	// Returns an VBArray:
	//   [0] left   (DT_CALCRECT) 
	//   [1] top    (DT_CALCRECT)
	//   [2] right  (DT_CALCRECT)
	//   [3] bottom (DT_CALCRECT)
	//   [4] characters drawn
	const int elements[] = 
	{
		rc.left,
		rc.top,
		rc.right,
		rc.bottom,
		dpt.uiLengthDrawn
	};

	helpers::com_array_writer<> helper;

	if (!helper.create(_countof(elements)))
		return E_OUTOFMEMORY;

	for (long i = 0; i < helper.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_I4;
		var.lVal = elements[i];

		if (FAILED(helper.put(i, var)))
		{
			helper.reset();
			return E_OUTOFMEMORY;
		}
	}

	p->vt = VT_ARRAY | VT_VARIANT;
	p->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP GdiGraphics::MeasureString(BSTR str, IGdiFont * font, float x, float y, float w, float h, int flags, IMeasureStringInfo ** pp)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!str || !font) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	Gdiplus::Font* fn = NULL;
	font->get__ptr((void**)&fn);
	if (!fn) return E_INVALIDARG;

	Gdiplus::StringFormat fmt = Gdiplus::StringFormat::GenericTypographic();

	if (flags != 0)
	{
		fmt.SetAlignment((Gdiplus::StringAlignment)((flags >> 28) & 0x3));      //0xf0000000
		fmt.SetLineAlignment((Gdiplus::StringAlignment)((flags >> 24) & 0x3));  //0x0f000000
		fmt.SetTrimming((Gdiplus::StringTrimming)((flags >> 20) & 0x7));        //0x00f00000
		fmt.SetFormatFlags((Gdiplus::StringFormatFlags)(flags & 0x7FFF));       //0x0000ffff
	}

	Gdiplus::RectF bound;
	int chars, lines;

	m_ptr->MeasureString(str, -1, fn, Gdiplus::RectF(x, y, w, h), &fmt, &bound, &chars, &lines);

	(*pp) = new com_object_impl_t<MeasureStringInfo>(bound.X, bound.Y, bound.Width, bound.Height, lines, chars);
	return S_OK;
}

STDMETHODIMP GdiGraphics::CalcTextWidth(BSTR str, IGdiFont * font, UINT * p)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!str || !font) return E_INVALIDARG;
	if (!p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	if (!hFont) return E_INVALIDARG;

	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();

	oldfont = SelectFont(dc, hFont);
	*p = helpers::get_text_width(dc, str, SysStringLen(str));
	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::CalcTextHeight(BSTR str, IGdiFont * font, UINT * p)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;
	if (!str || !font) return E_INVALIDARG;
	if (!p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	if (!hFont) return E_INVALIDARG;

	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();

	oldfont = SelectFont(dc, hFont);
	*p = helpers::get_text_height(dc, str, SysStringLen(str));
	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::EstimateLineWrap(BSTR str, IGdiFont * font, int max_width, VARIANT * p)
{
	if (!m_ptr) return E_POINTER;
	if (!str || !font) return E_INVALIDARG;
	if (!p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	if (!hFont) return E_INVALIDARG;

	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();
	pfc::list_t<helpers::wrapped_item> result;

	oldfont = SelectFont(dc, hFont);
	estimate_line_wrap(dc, str, SysStringLen(str), max_width, result);
	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);

	helpers::com_array_writer<> helper;

	if (!helper.create(result.get_count() * 2))
	{
		return E_OUTOFMEMORY;
	}

	for (long i = 0; i < helper.get_count() / 2; ++i)
	{
		_variant_t var1, var2;

		var1.vt = VT_BSTR;
		var1.bstrVal = result[i].text;
		var2.vt = VT_I4;
		var2.lVal = result[i].width;

		helper.put(i * 2, var1);
		helper.put(i * 2 + 1, var2);
	}

	p->vt = VT_ARRAY | VT_VARIANT;
	p->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP GdiGraphics::SetTextRenderingHint(UINT mode)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	m_ptr->SetTextRenderingHint((Gdiplus::TextRenderingHint)mode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::SetSmoothingMode(INT mode)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	m_ptr->SetSmoothingMode((Gdiplus::SmoothingMode)mode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::SetInterpolationMode(INT mode)
{
	TRACK_FUNCTION();

	if (!m_ptr) return E_POINTER;

	m_ptr->SetInterpolationMode((Gdiplus::InterpolationMode)mode);
	return S_OK;
}

STDMETHODIMP GdiUtils::Font(BSTR name, float pxSize, int style, IGdiFont** pp)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	Gdiplus::Font * font = new Gdiplus::Font(name, pxSize, style, Gdiplus::UnitPixel);

	if (!helpers::ensure_gdiplus_object(font))
	{
		if (font) delete font;
		(*pp) = NULL;
		return S_OK;
	}

	// Generate HFONT
	// The benefit of replacing Gdiplus::Font::GetLogFontW is that you can get it work with CCF/OpenType fonts.
	HFONT hFont = CreateFont(
			-(int)pxSize,
			0,
			0,
			0,
			(style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
			(style & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
			(style & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
			(style & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			name);
	(*pp) = new com_object_impl_t<GdiFont>(font, hFont);
	return S_OK;
}

STDMETHODIMP GdiUtils::Image(BSTR path, IGdiBitmap** pp)
{
	TRACK_FUNCTION();

	if (!path) return E_INVALIDARG;
	if (!pp) return E_POINTER;
	(*pp) = NULL;

	// Since using Gdiplus::Bitmap(path) will result locking file, so use IStream instead to prevent it.
	IStreamPtr pStream;
	HRESULT hr = SHCreateStreamOnFileEx(path, STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, NULL, &pStream);
	if (FAILED(hr)) return S_OK;
	Gdiplus::Bitmap * img = new Gdiplus::Bitmap(pStream, PixelFormat32bppPARGB);

	if (!helpers::ensure_gdiplus_object(img))
	{
		if (img) delete img;
		return S_OK;
	}

	(*pp) = new com_object_impl_t<GdiBitmap>(img);
	return S_OK;
}

STDMETHODIMP GdiUtils::CreateImage(int w, int h, IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	Gdiplus::Bitmap * img = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB);

	if (!helpers::ensure_gdiplus_object(img))
	{
		if (img) delete img;
		(*pp) = NULL;
		return S_OK;
	}

	(*pp) = new com_object_impl_t<GdiBitmap>(img);
	return S_OK;
}

STDMETHODIMP GdiUtils::CreateStyleTextRender(VARIANT_BOOL pngmode, IStyleTextRender ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = new com_object_impl_t<StyleTextRender>(pngmode != VARIANT_FALSE);
	return S_OK;
}

STDMETHODIMP GdiUtils::LoadImageAsync(UINT window_id, BSTR path, UINT * p)
{
	TRACK_FUNCTION();

	if (!path) return E_INVALIDARG;
	if (!p) return E_POINTER;

	unsigned cookie = 0;

	try
	{
		helpers::load_image_async * task = new helpers::load_image_async((HWND)window_id, path);

		if (simple_thread_pool::instance().enqueue(task))
			cookie = reinterpret_cast<unsigned>(task);
		else
			delete task;
	}
	catch (std::exception &) {}

	(*p) = cookie;
	return S_OK;
}

STDMETHODIMP FbFileInfo::get__ptr(void ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	*pp = m_info_ptr;
	return S_OK;
}

STDMETHODIMP FbFileInfo::get_MetaCount(UINT* p)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!p) return E_POINTER;

	*p = (UINT)m_info_ptr->meta_get_count();
	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaValueCount(UINT idx, UINT* p)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!p) return E_POINTER;

	*p = (UINT)m_info_ptr->meta_enum_value_count(idx);
	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaName(UINT idx, BSTR* pp)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!pp) return E_POINTER;

	(*pp) = NULL;

	if (idx < m_info_ptr->meta_get_count())
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->meta_enum_name(idx);
		(*pp) = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaValue(UINT idx, UINT vidx, BSTR* pp)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!pp) return E_POINTER;

	(*pp) = NULL;

	if (idx < m_info_ptr->meta_get_count() && vidx < m_info_ptr->meta_enum_value_count(idx))
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->meta_enum_value(idx, vidx);
		(*pp) = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaFind(BSTR name, UINT * p)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!name) return E_POINTER;
	if (!p) return E_POINTER;

	*p = m_info_ptr->meta_find(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaRemoveField(BSTR name)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!name) return E_INVALIDARG;

	m_info_ptr->meta_remove_field(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaAdd(BSTR name, BSTR value, UINT * p)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!name || !value) return E_INVALIDARG;

	*p = m_info_ptr->meta_add(pfc::stringcvt::string_utf8_from_wide(name), 
		pfc::stringcvt::string_utf8_from_wide(value));

	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaInsertValue(UINT idx, UINT vidx, BSTR value)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!value) return E_INVALIDARG;

	if (idx < m_info_ptr->meta_get_count() && vidx < m_info_ptr->meta_enum_value_count(idx))
	{
		m_info_ptr->meta_insert_value(idx, vidx, pfc::stringcvt::string_utf8_from_wide(value));
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::get_InfoCount(UINT* p)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!p) return E_POINTER;

	*p = (UINT)m_info_ptr->info_get_count();
	return S_OK;
}

STDMETHODIMP FbFileInfo::InfoName(UINT idx, BSTR* pp)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!pp) return E_POINTER;

	(*pp) = NULL;

	if (idx < m_info_ptr->info_get_count())
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->info_enum_name(idx);
		(*pp) = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::InfoValue(UINT idx, BSTR* pp)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!pp) return E_POINTER;

	(*pp) = NULL;

	if (idx < m_info_ptr->info_get_count())
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->info_enum_value(idx);
		(*pp) = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::InfoFind(BSTR name, UINT * p)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!name) return E_INVALIDARG;
	if (!p) return E_POINTER;

	*p = m_info_ptr->info_find(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaSet(BSTR name, BSTR value)
{
	TRACK_FUNCTION();

	if (!m_info_ptr) return E_POINTER;
	if (!name || !value) return E_INVALIDARG;

	pfc::string8_fast uname = pfc::stringcvt::string_utf8_from_wide(name);
	pfc::string8_fast uvalue = pfc::stringcvt::string_utf8_from_wide(value);

	m_info_ptr->meta_set(uname, uvalue);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get__ptr(void ** pp)
{
	TRACK_FUNCTION();

	(*pp) = m_handle.get_ptr();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_Path(BSTR* pp)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!pp) return E_POINTER;

	pfc::stringcvt::string_wide_from_utf8_fast ucs = file_path_display(m_handle->get_path());

	(*pp) = SysAllocString(ucs);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_RawPath(BSTR * pp)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!pp) return E_POINTER;

	pfc::stringcvt::string_wide_from_utf8_fast ucs = m_handle->get_path();

	(*pp) = SysAllocString(ucs);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_SubSong(UINT* p)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!p) return E_POINTER;

	*p = m_handle->get_subsong_index();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_FileSize(LONGLONG* p)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!p) return E_POINTER;

	*p = m_handle->get_filesize();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_Length(double* p)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!p) return E_POINTER;

	*p = m_handle->get_length();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::GetFileInfo(IFbFileInfo ** pp)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!pp) return E_POINTER;

	file_info_impl * info_ptr = new file_info_impl;

	m_handle->get_info(*info_ptr);
	(*pp) = new com_object_impl_t<FbFileInfo>(info_ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::UpdateFileInfo(IFbFileInfo * fileinfo)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!fileinfo) return E_INVALIDARG;

	static_api_ptr_t<metadb_io_v2> io;
	file_info_impl * info_ptr = NULL;
	fileinfo->get__ptr((void**)&info_ptr);
	if (!info_ptr) return E_INVALIDARG;

	PRINT_OBSOLETE_MESSAGE_ONCE("UpdateFileInfo() is now obsolete, please use UpdateFileInfoSimple() instead.");

	io->update_info_async_simple(pfc::list_single_ref_t<metadb_handle_ptr>(m_handle),
		pfc::list_single_ref_t<const file_info *>(info_ptr),
		core_api::get_main_window(), metadb_io_v2::op_flag_delay_ui, NULL);

	return S_OK;
}

STDMETHODIMP FbMetadbHandle::UpdateFileInfoSimple(SAFEARRAY * p)
{
	TRACK_FUNCTION();

	if (m_handle.is_empty()) return E_POINTER;
	if (!p) return E_INVALIDARG;

	helpers::file_info_pairs_filter::t_field_value_map field_value_map;
	pfc::stringcvt::string_utf8_from_wide ufield, uvalue, umultival;
	HRESULT hr;
	LONG nLBound = 0, nUBound = -1;
	LONG nCount;

	if (FAILED(hr = SafeArrayGetLBound(p, 1, &nLBound)))
		return hr;

	if (FAILED(hr = SafeArrayGetUBound(p, 1, &nUBound)))
		return hr;

	nCount = nUBound - nLBound + 1;

	if (nCount < 2)
		return DISP_E_BADPARAMCOUNT;

	// Enum every two elems
	for (LONG i = nLBound; i < nUBound; i += 2)
	{
		_variant_t var_field, var_value;
		LONG n1 = i;
		LONG n2 = i + 1;

		if (FAILED(hr = SafeArrayGetElement(p, &n1, &var_field)))
			return hr;

		if (FAILED(hr = SafeArrayGetElement(p, &n2, &var_value)))
			return hr;

		if (FAILED(hr = VariantChangeType(&var_field, &var_field, 0, VT_BSTR)))
			return hr;

		if (FAILED(hr = VariantChangeType(&var_value, &var_value, 0, VT_BSTR)))
			return hr;

		ufield.convert(var_field.bstrVal);
		uvalue.convert(var_value.bstrVal);

		field_value_map[ufield] = uvalue;
	}

	// Get multivalue fields
	if (nCount % 2 != 0)
	{
		_variant_t var_multival;
		LONG n = nUBound;

		if (FAILED(hr = SafeArrayGetElement(p, &n, &var_multival)))
			return hr;

		if (FAILED(hr = VariantChangeType(&var_multival, &var_multival, 0, VT_BSTR)))
			return hr;

		umultival.convert(var_multival.bstrVal);
	}

	static_api_ptr_t<metadb_io_v2> io;

	io->update_info_async(pfc::list_single_ref_t<metadb_handle_ptr>(m_handle), 
		new service_impl_t<helpers::file_info_pairs_filter>(m_handle, field_value_map, umultival), 
		core_api::get_main_window(), metadb_io_v2::op_flag_delay_ui, NULL);

	return S_OK;
}

STDMETHODIMP FbMetadbHandle::Compare(IFbMetadbHandle * handle, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	*p = VARIANT_FALSE;

	if (handle)
	{
		metadb_handle * ptr = NULL;
		handle->get__ptr((void **)&ptr);

		*p = TO_VARIANT_BOOL(ptr == m_handle.get_ptr());
	}

	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::get__ptr(void ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	*pp = &m_handles;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::get_Item(UINT index, IFbMetadbHandle ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (index >= m_handles.get_size()) return E_INVALIDARG;
	if (index >= m_handles.get_count()) return E_INVALIDARG;

	*pp = new com_object_impl_t<FbMetadbHandle>(m_handles.get_item_ref(index));
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::put_Item(UINT index, IFbMetadbHandle * handle)
{
	TRACK_FUNCTION();

	if (index >= m_handles.get_size()) return E_INVALIDARG;
	if (!handle) return E_INVALIDARG;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void **)&ptr);
	m_handles.replace_item(index, ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::get_Count(UINT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_handles.get_count();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Clone(IFbMetadbHandleList ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	*pp = new com_object_impl_t<FbMetadbHandleList>(m_handles);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Sort()
{
	TRACK_FUNCTION();

	metadb_handle_list_helper::sort_by_pointer_remove_duplicates(m_handles);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Find(IFbMetadbHandle * handle, UINT * p)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	if (!p) return E_POINTER;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void **)&ptr);
	*p = m_handles.find_item(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::BSearch(IFbMetadbHandle * handle, UINT * p)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	if (!p) return E_POINTER;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void **)&ptr);
	*p = m_handles.bsearch_by_pointer(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Insert(UINT index, IFbMetadbHandle * handle, UINT * outIndex)
{
	TRACK_FUNCTION();

	if (!outIndex) return E_POINTER;
	if (!handle) return E_INVALIDARG;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void **)&ptr);
	(*outIndex) = m_handles.insert_item(ptr, index);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::InsertRange(UINT index, IFbMetadbHandleList * handles, UINT * outIndex)
{
	TRACK_FUNCTION();

	if (!outIndex) return E_POINTER;
	if (!handles) return E_INVALIDARG;

	metadb_handle_list * handles_ptr = NULL;
	handles->get__ptr((void **)&handles_ptr);
	if (!handles_ptr) return E_INVALIDARG;
	(*outIndex) = m_handles.insert_items(*handles_ptr, index);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Add(IFbMetadbHandle * handle, UINT * p)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	if (!p) return E_POINTER;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void **)&ptr);
	*p = m_handles.add_item(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::AddRange(IFbMetadbHandleList * handles)
{
	TRACK_FUNCTION();

	if (!handles) return E_INVALIDARG;

	metadb_handle_list * handles_ptr = NULL;
	handles->get__ptr((void **)&handles_ptr);
	if (!handles_ptr) return E_INVALIDARG;
	m_handles.add_items(*handles_ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::RemoveById(UINT index)
{
	TRACK_FUNCTION();

	if (index >= m_handles.get_count()) return E_INVALIDARG;
	m_handles.remove_by_idx(index);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Remove(IFbMetadbHandle * handle)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void **)&ptr);
	m_handles.remove_item(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::RemoveAll()
{
	TRACK_FUNCTION();

	m_handles.remove_all();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::RemoveRange(UINT from, UINT count)
{
	TRACK_FUNCTION();

	m_handles.remove_from_idx(from, count);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::MakeIntersection(IFbMetadbHandleList * handles)
{
	TRACK_FUNCTION();

	if (!handles) return E_INVALIDARG;

	metadb_handle_list * handles_ptr = NULL;
	handles->get__ptr((void **)&handles_ptr);
	if (!handles_ptr) return E_INVALIDARG;

	metadb_handle_list_ref handles_ref = *handles_ptr;
	metadb_handle_list result;
	t_size walk1 = 0;
	t_size walk2 = 0;
	t_size last1 = m_handles.get_count();
	t_size last2 = handles_ptr->get_count();

	while (walk1 != last1 && walk2 != last2)
	{
		if (m_handles[walk1] < handles_ref[walk2])
			++walk1;
		else if (handles_ref[walk2] < m_handles[walk1])
			++walk2;
		else 
		{
			result.add_item(m_handles[walk1]);
			++walk1;
			++walk2;
		}
	}

	m_handles = result;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::MakeUnion(IFbMetadbHandleList * handles)
{
	TRACK_FUNCTION();

	if (!handles) return E_INVALIDARG;

	metadb_handle_list * handles_ptr = NULL;
	handles->get__ptr((void **)&handles_ptr);
	if (!handles_ptr) return E_INVALIDARG;

	metadb_handle_list_ref handles_ref = *handles_ptr;
	metadb_handle_list result;
	t_size walk1 = 0;
	t_size walk2 = 0;
	t_size last1 = m_handles.get_count();
	t_size last2 = handles_ptr->get_count();

	while (walk1 != last1 && walk2 != last2) 
	{
		if (m_handles[walk1] < handles_ref[walk2]) 
		{
			result.add_item(m_handles[walk1]);
			++walk1;
		}
		else if (handles_ref[walk2] < m_handles[walk1]) 
		{
			result.add_item(handles_ref[walk2]);
			++walk2;
		}
		else 
		{
			result.add_item(m_handles[walk1]);
			++walk1;
			++walk2;
		}
	}

	m_handles = result;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::MakeDifference(IFbMetadbHandleList * handles)
{
	TRACK_FUNCTION();

	if (!handles) return E_INVALIDARG;

	metadb_handle_list * handles_ptr = NULL;
	handles->get__ptr((void **)&handles_ptr);
	if (!handles_ptr) return E_INVALIDARG;

	metadb_handle_list_ref handles_ref = *handles_ptr;
	metadb_handle_list result;
	t_size walk1 = 0;
	t_size walk2 = 0;
	t_size last1 = m_handles.get_count();
	t_size last2 = handles_ptr->get_count();

	while (walk1 != last1 && walk2 != last2)
	{
		if (m_handles[walk1] < handles_ref[walk2]) 
		{
			result.add_item(m_handles[walk1]);
			++walk1;
		}
		else if (handles_ref[walk2] < m_handles[walk1])
		{
			++walk2;
		}
		else 
		{
			++walk1;
			++walk2;
		}
	}

	m_handles = result;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::OrderByFormat(__interface IFbTitleFormat * script, int direction)
{
	TRACK_FUNCTION();

	if (!script) return E_INVALIDARG;
	titleformat_object * obj = NULL;
	script->get__ptr((void **)&obj);
	if (!obj) return E_INVALIDARG;
	m_handles.sort_by_format(obj, NULL, direction);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::OrderByPath()
{
	TRACK_FUNCTION();

	m_handles.sort_by_path();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::OrderByRelativePath()
{
	TRACK_FUNCTION();

	m_handles.sort_by_relative_path();
	return S_OK;
}


STDMETHODIMP FbTitleFormat::get__ptr(void ** pp)
{
	TRACK_FUNCTION();

	*pp = m_obj.get_ptr();
	return S_OK;
}

STDMETHODIMP FbTitleFormat::Eval(VARIANT_BOOL force, BSTR* pp)
{
	TRACK_FUNCTION();

	if (m_obj.is_empty()) return E_POINTER;
	if (!pp) return E_POINTER;

	pfc::string8_fast text;

	if (!static_api_ptr_t<playback_control>()->playback_format_title(NULL, text, m_obj, NULL, playback_control::display_level_all) && force)
	{
		metadb_handle_ptr handle;

		if (!metadb::g_get_random_handle(handle))
		{
			static_api_ptr_t<metadb> m;

			// HACK: A fake file handle should be okay
			m->handle_create(handle, make_playable_location("file://C:\\________.ogg", 0));
		}

		handle->format_title(NULL, text, m_obj, NULL);
	}

	(*pp) = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(text));
	return S_OK;
}

STDMETHODIMP FbTitleFormat::EvalWithMetadb(IFbMetadbHandle * handle, BSTR * pp)
{
	TRACK_FUNCTION();

	if (m_obj.is_empty()) return E_POINTER;
	if (!handle) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void**)&ptr);
	if (!ptr) return E_INVALIDARG;

	pfc::string8_fast text;
	ptr->format_title(NULL, text, m_obj, NULL);
	(*pp) = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(text));
	return S_OK;
}


STDMETHODIMP FbUtils::trace(SAFEARRAY * p)
{
	TRACK_FUNCTION();

	if (!p) return E_INVALIDARG;

	pfc::string8_fast str;
	LONG nLBound = 0, nUBound = -1;
	HRESULT hr;

	if (FAILED(hr = SafeArrayGetLBound(p, 1, &nLBound)))
		return hr;

	if (FAILED(hr = SafeArrayGetUBound(p, 1, &nUBound)))
		return hr;

	for (LONG i = nLBound; i <= nUBound; ++i)
	{
		_variant_t var;
		LONG n = i;

		if (FAILED(SafeArrayGetElement(p, &n, &var)))
			continue;

		if (FAILED(hr = VariantChangeType(&var, &var, VARIANT_ALPHABOOL, VT_BSTR)))
			continue;

		str.add_string(pfc::stringcvt::string_utf8_from_wide(var.bstrVal));
		str.add_byte(' ');
	}

	console::info(str);
	return S_OK;
}

STDMETHODIMP FbUtils::ShowPopupMessage(BSTR msg, BSTR title, int iconid)
{
	TRACK_FUNCTION();

	if (!msg || !title) return E_INVALIDARG;

	popup_msg::g_show(pfc::stringcvt::string_utf8_from_wide(msg), 
		pfc::stringcvt::string_utf8_from_wide(title), (popup_message::t_icon)iconid);
	return S_OK;
}

STDMETHODIMP FbUtils::CreateProfiler(BSTR name, IFbProfiler ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!name) return E_INVALIDARG;

	(*pp) = new com_object_impl_t<FbProfiler>(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbUtils::TitleFormat(BSTR expression, IFbTitleFormat** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!expression) return E_INVALIDARG;

	(*pp) = new com_object_impl_t<FbTitleFormat>(expression);
	return S_OK;
}

STDMETHODIMP FbUtils::GetNowPlaying(IFbMetadbHandle** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	metadb_handle_ptr metadb;

	if (!static_api_ptr_t<playback_control>()->get_now_playing(metadb))
	{
		(*pp) = NULL;
		return S_OK;
	}

	(*pp) = new com_object_impl_t<FbMetadbHandle>(metadb);
	return S_OK;
}

STDMETHODIMP FbUtils::GetFocusItem(VARIANT_BOOL force, IFbMetadbHandle** pp)
{
	return FbPlaylistMangerTemplate::GetPlaylistFocusItemHandle(force, pp);
}

STDMETHODIMP FbUtils::GetSelection(IFbMetadbHandle** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	metadb_handle_list items;

	static_api_ptr_t<ui_selection_manager>()->get_selection(items);

	if (items.get_count() > 0)
	{
		(*pp) = new com_object_impl_t<FbMetadbHandle>(items[0]);
	}
	else
	{
		(*pp) = NULL;
	}

	return S_OK;
}

STDMETHODIMP FbUtils::GetSelections(UINT flags, IFbMetadbHandleList ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<ui_selection_manager_v2>()->get_selection(items, flags);
	(*pp) = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbUtils::GetSelectionType(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	const GUID * guids[] = {
		&contextmenu_item::caller_undefined,
		&contextmenu_item::caller_active_playlist_selection,
		&contextmenu_item::caller_active_playlist,
		&contextmenu_item::caller_playlist_manager, 
		&contextmenu_item::caller_now_playing, 
		&contextmenu_item::caller_keyboard_shortcut_list, 
		&contextmenu_item::caller_media_library_viewer,
	};

	*p = 0;
	GUID type = static_api_ptr_t<ui_selection_manager_v2>()->get_selection_type(0);

	for (t_size i = 0; i < _countof(guids); ++i)
	{
		if (*guids[i] == type)
		{
			*p = i;
			break;
		}
	}

	return S_OK;
}

STDMETHODIMP FbUtils::AcquireUiSelectionHolder(IFbUiSelectionHolder ** outHolder)
{
	TRACK_FUNCTION();

	if (!outHolder) return E_INVALIDARG;

	ui_selection_holder::ptr holder = static_api_ptr_t<ui_selection_manager>()->acquire();
	(*outHolder) = new com_object_impl_t<FbUiSelectionHolder>(holder);
	return S_OK;
}

STDMETHODIMP FbUtils::get_ComponentPath(BSTR* pp)
{
	TRACK_FUNCTION();

	static pfc::stringcvt::string_wide_from_utf8 path(helpers::get_fb2k_component_path());

	(*pp) = SysAllocString(path.get_ptr());
	return S_OK;
}

STDMETHODIMP FbUtils::get_FoobarPath(BSTR* pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	static pfc::stringcvt::string_wide_from_utf8 path(helpers::get_fb2k_path());

	(*pp) = SysAllocString(path.get_ptr());
	return S_OK;
}

STDMETHODIMP FbUtils::get_ProfilePath(BSTR* pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	static pfc::stringcvt::string_wide_from_utf8 path(helpers::get_profile_path());

	(*pp) = SysAllocString(path.get_ptr());
	return S_OK;
}

STDMETHODIMP FbUtils::get_IsPlaying(VARIANT_BOOL* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(static_api_ptr_t<playback_control>()->is_playing());
	return S_OK;
}

STDMETHODIMP FbUtils::get_IsPaused(VARIANT_BOOL* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(static_api_ptr_t<playback_control>()->is_paused());
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackTime(double* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->playback_get_position();
	return S_OK;
}

STDMETHODIMP FbUtils::put_PlaybackTime(double time)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playback_control>()->playback_seek(time);
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackLength(double* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->playback_get_length();
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackOrder(UINT* p)
{
	return FbPlaylistMangerTemplate::get_PlaybackOrder(p);
}

STDMETHODIMP FbUtils::put_PlaybackOrder(UINT order)
{
	return FbPlaylistMangerTemplate::put_PlaybackOrder(order);
}

STDMETHODIMP FbUtils::get_StopAfterCurrent(VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->get_stop_after_current();
	return S_OK;
}

STDMETHODIMP FbUtils::put_StopAfterCurrent(VARIANT_BOOL p)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playback_control>()->set_stop_after_current(p != VARIANT_FALSE);
	return S_OK;
}

STDMETHODIMP FbUtils::get_CursorFollowPlayback(VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(config_object::g_get_data_bool_simple(standard_config_objects::bool_cursor_follows_playback, false));
	return S_OK;
}

STDMETHODIMP FbUtils::put_CursorFollowPlayback(VARIANT_BOOL p)
{
	TRACK_FUNCTION();

	config_object::g_set_data_bool(standard_config_objects::bool_cursor_follows_playback, (p != VARIANT_FALSE));
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackFollowCursor(VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(config_object::g_get_data_bool_simple(standard_config_objects::bool_playback_follows_cursor, false));
	return S_OK;
}

STDMETHODIMP FbUtils::put_PlaybackFollowCursor(VARIANT_BOOL p)
{
	TRACK_FUNCTION();

	config_object::g_set_data_bool(standard_config_objects::bool_playback_follows_cursor, (p != VARIANT_FALSE));
	return S_OK;
}

STDMETHODIMP FbUtils::get_Volume(float* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->get_volume();
	return S_OK;
}

STDMETHODIMP FbUtils::put_Volume(float value)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playback_control>()->set_volume(value);
	return S_OK;
}

STDMETHODIMP FbUtils::Exit()
{
	TRACK_FUNCTION();

	standard_commands::main_exit();
	return S_OK;
}

STDMETHODIMP FbUtils::Play()
{
	TRACK_FUNCTION();

	standard_commands::main_play();
	return S_OK;
}

STDMETHODIMP FbUtils::Stop()
{
	TRACK_FUNCTION();

	standard_commands::main_stop();
	return S_OK;
}

STDMETHODIMP FbUtils::Pause()
{
	TRACK_FUNCTION();

	standard_commands::main_pause();
	return S_OK;
}

STDMETHODIMP FbUtils::PlayOrPause()
{
	TRACK_FUNCTION();

	standard_commands::main_play_or_pause();
	return S_OK;
}

STDMETHODIMP FbUtils::Next()
{
	TRACK_FUNCTION();

	standard_commands::main_next();
	return S_OK;
}

STDMETHODIMP FbUtils::Prev()
{
	TRACK_FUNCTION();

	standard_commands::main_previous();
	return S_OK;
}

STDMETHODIMP FbUtils::Random()
{
	TRACK_FUNCTION();

	standard_commands::main_random();
	return S_OK;
}

STDMETHODIMP FbUtils::VolumeDown()
{
	TRACK_FUNCTION();

	standard_commands::main_volume_down();
	return S_OK;
}

STDMETHODIMP FbUtils::VolumeUp()
{
	TRACK_FUNCTION();

	standard_commands::main_volume_up();
	return S_OK;
}

STDMETHODIMP FbUtils::VolumeMute()
{
	TRACK_FUNCTION();

	standard_commands::main_volume_mute();
	return S_OK;
}

STDMETHODIMP FbUtils::AddDirectory()
{
	TRACK_FUNCTION();

	standard_commands::main_add_directory();
	return S_OK;
}

STDMETHODIMP FbUtils::AddFiles()
{
	TRACK_FUNCTION();

	standard_commands::main_add_files();
	return S_OK;
}

STDMETHODIMP FbUtils::ShowConsole()
{
	TRACK_FUNCTION();

	// HACK: This command won't work
	//standard_commands::main_show_console();

	// {5B652D25-CE44-4737-99BB-A3CF2AEB35CC}
	const GUID guid_main_show_console = 
	{ 0x5b652d25, 0xce44, 0x4737, { 0x99, 0xbb, 0xa3, 0xcf, 0x2a, 0xeb, 0x35, 0xcc } };

	standard_commands::run_main(guid_main_show_console);
	return S_OK;
}

STDMETHODIMP FbUtils::ShowPreferences()
{
	TRACK_FUNCTION();

	standard_commands::main_preferences();
	return S_OK;
}

STDMETHODIMP FbUtils::ClearPlaylist()
{
	TRACK_FUNCTION();

	standard_commands::main_clear_playlist();
	return S_OK;
}

STDMETHODIMP FbUtils::LoadPlaylist()
{
	TRACK_FUNCTION();

	standard_commands::main_load_playlist();
	return S_OK;
}

STDMETHODIMP FbUtils::SavePlaylist()
{
	TRACK_FUNCTION();

	standard_commands::main_save_playlist();
	return S_OK;
}

STDMETHODIMP FbUtils::RunMainMenuCommand(BSTR command, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!command) return E_INVALIDARG;
	if (!p) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide name(command);

	*p = TO_VARIANT_BOOL(helpers::execute_mainmenu_command_by_name_SEH(name));
	return S_OK;
}

STDMETHODIMP FbUtils::RunContextCommand(BSTR command, UINT flags, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!command) return E_INVALIDARG;
	if (!p) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide name(command);
	*p = TO_VARIANT_BOOL(helpers::execute_context_command_by_name_SEH(name, metadb_handle_list(), flags));
	return S_OK;
}

STDMETHODIMP FbUtils::RunContextCommandWithMetadb(BSTR command, VARIANT handle, UINT flags, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	IDispatchPtr handle_s = NULL;
	int try_result = TryGetMetadbHandleFromVariant(handle, &handle_s);

	if (!command || try_result < 0 || !handle_s) return E_INVALIDARG;
	if (!p) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide name(command);
	metadb_handle_list handle_list;
	void * ptr = NULL;

	switch (try_result)
	{
	case 0:
		reinterpret_cast<IFbMetadbHandle *>(handle_s.GetInterfacePtr())->get__ptr(&ptr);
		if (!ptr) return E_INVALIDARG;
		handle_list = pfc::list_single_ref_t<metadb_handle_ptr>(reinterpret_cast<metadb_handle *>(ptr));
		break;

	case 1:
		reinterpret_cast<IFbMetadbHandleList *>(handle_s.GetInterfacePtr())->get__ptr(&ptr);
		if (!ptr) return E_INVALIDARG;
		handle_list = *reinterpret_cast<metadb_handle_list *>(ptr);
		break;

	default:
		return E_INVALIDARG;
	}

	*p = TO_VARIANT_BOOL(helpers::execute_context_command_by_name_SEH(name, handle_list, flags));
	return S_OK;
}

STDMETHODIMP FbUtils::CreateContextMenuManager(IContextMenuManager ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = new com_object_impl_t<ContextMenuManager>();
	return S_OK;
}

STDMETHODIMP FbUtils::CreateMainMenuManager(IMainMenuManager ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = new com_object_impl_t<MainMenuManager>();
	return S_OK;
}

STDMETHODIMP FbUtils::IsMetadbInMediaLibrary(IFbMetadbHandle * handle, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	if (!p) return E_POINTER;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void**)&ptr);
	*p = TO_VARIANT_BOOL(static_api_ptr_t<library_manager>()->is_item_in_library(ptr));
	return S_OK;
}

STDMETHODIMP FbUtils::get_ActivePlaylist(UINT * p)
{
	return FbPlaylistMangerTemplate::get_ActivePlaylist(p);
}

STDMETHODIMP FbUtils::put_ActivePlaylist(UINT idx)
{
	return FbPlaylistMangerTemplate::put_ActivePlaylist(idx);
}

STDMETHODIMP FbUtils::get_PlayingPlaylist(UINT * p)
{
	return FbPlaylistMangerTemplate::get_PlayingPlaylist(p);
}

STDMETHODIMP FbUtils::put_PlayingPlaylist(UINT idx)
{
	return FbPlaylistMangerTemplate::put_PlayingPlaylist(idx);
}

STDMETHODIMP FbUtils::get_PlaylistCount(UINT * p)
{
	return FbPlaylistMangerTemplate::get_PlaylistCount(p);
}

STDMETHODIMP FbUtils::get_PlaylistItemCount(UINT idx, UINT * p)
{
	return FbPlaylistMangerTemplate::get_PlaylistItemCount(idx, p);
}

STDMETHODIMP FbUtils::GetPlaylistName(UINT idx, BSTR * p)
{
	return FbPlaylistMangerTemplate::GetPlaylistName(idx, p);
}

STDMETHODIMP FbUtils::CreatePlaylist(UINT idx, BSTR name, UINT * p)
{
	return FbPlaylistMangerTemplate::CreatePlaylist(idx, name, p);
}

STDMETHODIMP FbUtils::RemovePlaylist(UINT idx, VARIANT_BOOL * p)
{
	return FbPlaylistMangerTemplate::RemovePlaylist(idx, p);
}

STDMETHODIMP FbUtils::MovePlaylist(UINT from, UINT to, VARIANT_BOOL * p)
{
	return FbPlaylistMangerTemplate::MovePlaylist(from, to, p);
}

STDMETHODIMP FbUtils::RenamePlaylist(UINT idx, BSTR name, VARIANT_BOOL * p)
{
	return FbPlaylistMangerTemplate::RenamePlaylist(idx, name, p);
}

STDMETHODIMP FbUtils::DuplicatePlaylist(UINT from, BSTR name, UINT * p)
{
	return FbPlaylistMangerTemplate::DuplicatePlaylist(from, name, p);
}

STDMETHODIMP FbUtils::IsAutoPlaylist(UINT idx, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	try
	{
		*p = TO_VARIANT_BOOL(static_api_ptr_t<autoplaylist_manager>()->is_client_present(idx));
	}
	catch (...)
	{
		*p = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP FbUtils::CreateAutoPlaylist(UINT idx, BSTR name, BSTR query, BSTR sort, UINT flags, UINT * p)
{
	TRACK_FUNCTION();

	if (!name || !query) return E_INVALIDARG;
	if (!p) return E_POINTER;

	UINT pos = 0;
	HRESULT hr = CreatePlaylist(idx, name, &pos);

	if (FAILED(hr)) return hr;

	pfc::stringcvt::string_utf8_from_wide wquery(query);
	pfc::stringcvt::string_utf8_from_wide wsort(sort);

	try
	{
		*p = pos;
		static_api_ptr_t<autoplaylist_manager>()->add_client_simple(wquery, wsort, pos, flags);
	}
	catch (...)
	{
		*p = pfc_infinite;
	}

	return S_OK;
}

STDMETHODIMP FbUtils::ShowAutoPlaylistUI(UINT idx, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	*p = VARIANT_TRUE;
	static_api_ptr_t<autoplaylist_manager> manager;

	try
	{
		if (manager->is_client_present(idx))
		{
			autoplaylist_client_ptr client = manager->query_client(idx);
			client->show_ui(idx);
		}
	}
	catch (...)
	{			
		*p = VARIANT_FALSE;
	}

	return S_OK;
}


STDMETHODIMP MenuObj::get_ID(UINT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_INVALIDARG;
	if (!m_hMenu) return E_POINTER;

	*p = (UINT)m_hMenu;
	return S_OK;
}

STDMETHODIMP MenuObj::AppendMenuItem(UINT flags, UINT item_id, BSTR text)
{
	TRACK_FUNCTION();

	if (!m_hMenu) return E_POINTER;
	if ((flags & MF_STRING) && !text) return E_INVALIDARG;
	if ((flags & MF_POPUP) && !(item_id & 0xffff0000)) return E_INVALIDARG;

	if (flags & MF_POPUP)
	{
		PRINT_OBSOLETE_MESSAGE_ONCE("Please use AppendTo() method to create sub menu instead of AppendMenuItem()");
	}

	::AppendMenu(m_hMenu, flags, item_id, text);
	return S_OK;
}

STDMETHODIMP MenuObj::AppendMenuSeparator()
{
	TRACK_FUNCTION();

	if (!m_hMenu) return E_POINTER;
	::AppendMenu(m_hMenu, MF_SEPARATOR, 0, 0);
	return S_OK;
}

STDMETHODIMP MenuObj::EnableMenuItem(UINT id_or_pos, UINT enable, VARIANT_BOOL bypos)
{
	TRACK_FUNCTION();

	if (!m_hMenu) return E_POINTER;

	enable &= ~(MF_BYPOSITION | MF_BYCOMMAND);
	enable |= bypos ? MF_BYPOSITION : MF_BYCOMMAND;

	::EnableMenuItem(m_hMenu, id_or_pos, enable);
	return S_OK;
}

STDMETHODIMP MenuObj::CheckMenuItem(UINT id_or_pos, VARIANT_BOOL check, VARIANT_BOOL bypos)
{
	TRACK_FUNCTION();

	if (!m_hMenu) return E_POINTER;

	UINT ucheck = bypos ? MF_BYPOSITION : MF_BYCOMMAND;
	if (check) ucheck = MF_CHECKED;
	::CheckMenuItem(m_hMenu, id_or_pos, ucheck);
	return S_OK;
}

STDMETHODIMP MenuObj::CheckMenuRadioItem(UINT first, UINT last, UINT check, VARIANT_BOOL bypos)
{
	TRACK_FUNCTION();

	if (!m_hMenu) return E_POINTER;
	::CheckMenuRadioItem(m_hMenu, first, last, check, bypos ? MF_BYPOSITION : MF_BYCOMMAND);
	return S_OK;
}

STDMETHODIMP MenuObj::TrackPopupMenu(int x, int y, UINT flags, UINT * item_id)
{
	TRACK_FUNCTION();

	if (!m_hMenu) return E_POINTER;
	if (!item_id) return E_POINTER;

	POINT pt = {x, y};

	// Only include specified flags
	flags |= TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;
	flags &= ~TPM_RECURSE;

	ClientToScreen(m_wnd_parent, &pt);
	(*item_id) = ::TrackPopupMenu(m_hMenu, flags, pt.x, pt.y, 0, m_wnd_parent, 0);
	return S_OK;
}

STDMETHODIMP MenuObj::AppendTo(IMenuObj * parent, UINT flags, BSTR text)
{
	TRACK_FUNCTION();

	if (!parent) return E_POINTER;
	if (!m_hMenu) return E_POINTER;
	if (!text) return E_INVALIDARG;

	MenuObj * pMenuParent = static_cast<MenuObj *>(parent);
	if (::AppendMenu(pMenuParent->m_hMenu, flags | MF_STRING | MF_POPUP, UINT_PTR(m_hMenu), text))
		m_has_detached = true;
	return S_OK;
}


STDMETHODIMP ContextMenuManager::InitContext(VARIANT handle)
{
	TRACK_FUNCTION();

	IDispatchPtr handle_s = NULL;
	int try_result = TryGetMetadbHandleFromVariant(handle, &handle_s);

	if (try_result < 0 || !handle_s) return E_INVALIDARG;

	metadb_handle_list handle_list;
	void * ptr = NULL;

	switch (try_result)
	{
	case 0:
		reinterpret_cast<IFbMetadbHandle *>(handle_s.GetInterfacePtr())->get__ptr(&ptr);
		if (!ptr) return E_INVALIDARG;
		handle_list = pfc::list_single_ref_t<metadb_handle_ptr>(reinterpret_cast<metadb_handle *>(ptr));
		break;

	case 1:
		reinterpret_cast<IFbMetadbHandleList *>(handle_s.GetInterfacePtr())->get__ptr(&ptr);
		if (!ptr) return E_INVALIDARG;
		handle_list = *reinterpret_cast<metadb_handle_list *>(ptr);
		break;

	default:
		return E_INVALIDARG;
	}

	contextmenu_manager::g_create(m_cm);
	m_cm->init_context(handle_list, 0);
	return S_OK;
}

STDMETHODIMP ContextMenuManager::InitNowPlaying()
{
	TRACK_FUNCTION();

	contextmenu_manager::g_create(m_cm);
	m_cm->init_context_now_playing(0);
	return S_OK;
}

STDMETHODIMP ContextMenuManager::BuildMenu(IMenuObj * p, int base_id, int max_id)
{
	TRACK_FUNCTION();

	if (m_cm.is_empty()) return E_POINTER;
	if (!p) return E_INVALIDARG;

	UINT menuid;
	contextmenu_node * parent = parent = m_cm->get_root();

	p->get_ID(&menuid);
	m_cm->win32_build_menu((HMENU)menuid, parent, base_id, max_id);
	return S_OK;
}

STDMETHODIMP ContextMenuManager::ExecuteByID(UINT id, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (m_cm.is_empty()) return E_POINTER;

	*p = TO_VARIANT_BOOL(m_cm->execute_by_id(id));
	return S_OK;
}

STDMETHODIMP MainMenuManager::Init(BSTR root_name)
{
	TRACK_FUNCTION();

	if (!root_name) return E_INVALIDARG;

	struct t_valid_root_name
	{
		const wchar_t * name;
		const GUID * guid;
	};

	// In mainmenu_groups: 
	//   static const GUID file,view,edit,playback,library,help;
	const t_valid_root_name valid_root_names[] = 
	{
		{L"file",     &mainmenu_groups::file},
		{L"view",     &mainmenu_groups::view},
		{L"edit",     &mainmenu_groups::edit},
		{L"playback", &mainmenu_groups::playback},
		{L"library",  &mainmenu_groups::library},
		{L"help",     &mainmenu_groups::help},
	};

	// Find
	for (int i = 0; i < _countof(valid_root_names); ++i)
	{
		if (_wcsicmp(root_name, valid_root_names[i].name) == 0)
		{
			// found
			m_mm = standard_api_create_t<mainmenu_manager>();
			m_mm->instantiate(*valid_root_names[i].guid);
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

STDMETHODIMP MainMenuManager::BuildMenu(IMenuObj * p, int base_id, int count)
{
	TRACK_FUNCTION();

	if (m_mm.is_empty()) return E_POINTER;
	if (!p) return E_INVALIDARG;

	UINT menuid;

	p->get_ID(&menuid);

	// HACK: workaround for foo_menu_addons
	try
	{
		m_mm->generate_menu_win32((HMENU)menuid, base_id, count, 0);
	}
	catch (...) {}

	return S_OK;
}

STDMETHODIMP MainMenuManager::ExecuteByID(UINT id, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (m_mm.is_empty()) return E_POINTER;

	*p = TO_VARIANT_BOOL(m_mm->execute_command(id));
	return S_OK;
}

STDMETHODIMP TimerObj::get_ID(UINT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_id;
	return S_OK;
}

STDMETHODIMP FbProfiler::Reset()
{
	TRACK_FUNCTION();

	m_timer.start();
	return S_OK;
}

STDMETHODIMP FbProfiler::Print()
{
	TRACK_FUNCTION();

	console::formatter() << WSPM_NAME ": FbProfiler (" << m_name << "): " << (int)(m_timer.query() * 1000) << " ms";
	return S_OK;
}

STDMETHODIMP FbProfiler::get_Time(INT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = (int)(m_timer.query() * 1000);
	return S_OK;
}


STDMETHODIMP FbUiSelectionHolder::SetSelection(IFbMetadbHandleList * handles)
{
	TRACK_FUNCTION();

	if (!handles) return E_INVALIDARG;

	metadb_handle_list * ptrHandles = NULL;
	handles->get__ptr((void**)&handles);
	if (ptrHandles) m_holder->set_selection(*ptrHandles);
	return S_OK;
}

STDMETHODIMP FbUiSelectionHolder::SetPlaylistSelectionTracking()
{
	TRACK_FUNCTION();

	m_holder->set_playlist_selection_tracking();
	return S_OK;
}

STDMETHODIMP FbUiSelectionHolder::SetPlaylistTracking()
{
	TRACK_FUNCTION();

	m_holder->set_playlist_tracking();
	return S_OK;
}


STDMETHODIMP MeasureStringInfo::get_x(float * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_x;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_y(float * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_y;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_width(float * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_w;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_height(float * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_h;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_lines(int * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_l;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_chars(int * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_c;
	return S_OK;
}

STDMETHODIMP GdiRawBitmap::get_Width(UINT* p)
{
	TRACK_FUNCTION();

	if (!p || !m_hdc) return E_POINTER;

	*p = m_width;
	return S_OK;
}

STDMETHODIMP GdiRawBitmap::get_Height(UINT* p)
{
	TRACK_FUNCTION();

	if (!p || !m_hdc) return E_POINTER;

	*p = m_height;
	return S_OK;
}

STDMETHODIMP GdiRawBitmap::get__Handle(HDC * p)
{
	TRACK_FUNCTION();

	if (!p || !m_hdc) return E_POINTER;

	*p = m_hdc;
	return S_OK;
}

GdiRawBitmap::GdiRawBitmap(Gdiplus::Bitmap * p_bmp)
{
	PFC_ASSERT(p_bmp != NULL);
	m_width = p_bmp->GetWidth();
	m_height = p_bmp->GetHeight();

	m_hdc = CreateCompatibleDC(NULL);
	m_hbmp = gdiplus_helpers::create_hbitmap_from_gdiplus_bitmap(p_bmp);
	m_hbmpold = SelectBitmap(m_hdc, m_hbmp);
}

//STDMETHODIMP GdiRawBitmap::GetBitmap(IGdiBitmap ** pp)
//{
//	SelectBitmap(m_hdc, m_hbmpold);
//
//	Gdiplus::Bitmap * bitmap_mask = Gdiplus::Bitmap::FromHBITMAP(m_hbmp, NULL);
//
//	(*pp) = new com_object_impl_t<GdiBitmap>(bitmap_mask);
//	SelectBitmap(m_hdc, m_hbmp);
//
//	return S_OK;
//}

STDMETHODIMP WSHUtils::CheckComponent(BSTR name, VARIANT_BOOL is_dll, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!p) return E_POINTER;

	service_enum_t<componentversion> e;
	service_ptr_t<componentversion> ptr;
	pfc::string8_fast uname = pfc::stringcvt::string_utf8_from_wide(name);
	pfc::string8_fast temp;

	*p = VARIANT_FALSE;

	while (e.next(ptr))
	{
		if (is_dll)
			ptr->get_file_name(temp);
		else
			ptr->get_component_name(temp);

		if (!_stricmp(temp, uname))
		{
			*p = VARIANT_TRUE;
			break;
		}
	}

	return S_OK;
}

STDMETHODIMP WSHUtils::CheckFont(BSTR name, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!p) return E_POINTER;

	WCHAR family_name_eng[LF_FACESIZE] = { 0 };
	WCHAR family_name_loc[LF_FACESIZE] = { 0 };
	Gdiplus::InstalledFontCollection font_collection;
	Gdiplus::FontFamily * font_families;
	int count = font_collection.GetFamilyCount();
	int recv;

	*p = VARIANT_FALSE;
	font_families = new Gdiplus::FontFamily[count];
	font_collection.GetFamilies(count, font_families, &recv);

	if (recv == count)
	{
		// Find
		for (int i = 0; i < count; ++i)
		{
			font_families[i].GetFamilyName(family_name_eng, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			font_families[i].GetFamilyName(family_name_loc);

			if (_wcsicmp(name, family_name_loc) == 0 || _wcsicmp(name, family_name_eng) == 0)
			{
				*p = VARIANT_TRUE;
				break;
			}
		}
	}

	delete [] font_families;
	return S_OK;
}

STDMETHODIMP WSHUtils::GetAlbumArt(BSTR rawpath, int art_id, VARIANT_BOOL need_stub, IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	return helpers::get_album_art(rawpath, pp, art_id, need_stub);
}

STDMETHODIMP WSHUtils::GetAlbumArtV2(IFbMetadbHandle * handle, int art_id, VARIANT_BOOL need_stub, IGdiBitmap **pp)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	metadb_handle * ptr = NULL;
	handle->get__ptr((void**)&ptr);
	return helpers::get_album_art_v2(ptr, pp, art_id, need_stub);
}

STDMETHODIMP WSHUtils::GetAlbumArtEmbedded(BSTR rawpath, int art_id, IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!rawpath) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	return helpers::get_album_art_embedded(rawpath, pp, art_id);
}

STDMETHODIMP WSHUtils::GetAlbumArtAsync(UINT window_id, IFbMetadbHandle * handle, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, VARIANT_BOOL no_load, UINT * p)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	if (!p) return E_POINTER;

	unsigned cookie = 0;
	metadb_handle * ptr = NULL;
	handle->get__ptr((void**)&ptr);

	if (ptr)
	{
		try
		{
			helpers::album_art_async * task = new helpers::album_art_async((HWND)window_id,
				ptr, art_id, need_stub, only_embed, no_load);

			if (simple_thread_pool::instance().enqueue(task))
				cookie = reinterpret_cast<unsigned>(task);
			else
				delete task;
		}
		catch (std::exception &)
		{
			cookie = 0;
		}
	}
	else
	{
		cookie = 0;
	}

	(*p) = cookie;
	return S_OK;
}

STDMETHODIMP WSHUtils::ReadINI(BSTR filename, BSTR section, BSTR key, VARIANT defaultval, BSTR * pp)
{
	TRACK_FUNCTION();

	if (!filename || !section || !key) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	enum { BUFFER_LEN = 255 };
	TCHAR buff[BUFFER_LEN] = { 0 };

	GetPrivateProfileString(section, key, NULL, buff, BUFFER_LEN, filename);

	if (!buff[0])
	{
		_variant_t var;

		if (SUCCEEDED(VariantChangeType(&var, &defaultval, 0, VT_BSTR)))
		{
			(*pp) = SysAllocString(var.bstrVal);
			return S_OK;
		}
	}

	(*pp) = SysAllocString(buff);
	return S_OK;
}

STDMETHODIMP WSHUtils::WriteINI(BSTR filename, BSTR section, BSTR key, VARIANT val, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!filename || !section || !key) return E_INVALIDARG;
	if (!p) return E_POINTER;

	_variant_t var;
	HRESULT hr;

	if (FAILED(hr = VariantChangeType(&var, &val, 0, VT_BSTR)))
		return hr;

	*p = TO_VARIANT_BOOL(WritePrivateProfileString(section, key, var.bstrVal, filename));
	return S_OK;
}

STDMETHODIMP WSHUtils::IsKeyPressed(UINT vkey, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(::IsKeyPressed(vkey));
	return S_OK;
}

STDMETHODIMP WSHUtils::PathWildcardMatch(BSTR pattern, BSTR str, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!pattern || !str) return E_INVALIDARG;
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(PathMatchSpec(str, pattern));
	return S_OK;
}

STDMETHODIMP WSHUtils::ReadTextFile(BSTR filename, UINT codepage, BSTR * pp)
{
	TRACK_FUNCTION();

	if (!filename) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	pfc::array_t<wchar_t> content;

	*pp = NULL;

	if (helpers::read_file_wide(codepage, filename, content))
	{
		*pp = SysAllocString(content.get_ptr());
	}

	return S_OK;
}

STDMETHODIMP WSHUtils::GetSysColor(UINT index, int * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	int ret = ::GetSysColor(index);

	if (!ret)
	{
		if (!::GetSysColorBrush(index))
		{
			// invalid
			*p = 0;
			return S_OK;
		}
	}

	*p = helpers::convert_colorref_to_argb(ret);
	return S_OK;
}

STDMETHODIMP WSHUtils::GetSystemMetrics(UINT index, INT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = ::GetSystemMetrics(index);
	return S_OK;
}

STDMETHODIMP WSHUtils::Glob(BSTR pattern, UINT exc_mask, UINT inc_mask, VARIANT * p)
{
	TRACK_FUNCTION();

	if (!pattern) return E_INVALIDARG;
	if (!p) return E_POINTER;

	pfc::string8_fast path = pfc::stringcvt::string_utf8_from_wide(pattern);
	const char * fn = path.get_ptr() + path.scan_filename();
	pfc::string8_fast dir(path.get_ptr(), fn - path.get_ptr());
	puFindFile ff = uFindFirstFile(path.get_ptr());

	pfc::string_list_impl files;

	if (ff)
	{
		do
		{
			DWORD attr = ff->GetAttributes();

			if ((attr & inc_mask) && !(attr & exc_mask))
			{
				pfc::string8_fast fullpath = dir;
				fullpath.add_string(ff->GetFileName());
				files.add_item(fullpath.get_ptr());
			}
		} while (ff->FindNext());
	}

	delete ff;
	ff = NULL;

	helpers::com_array_writer<> helper;

	if (!helper.create(files.get_count())) 
		return E_OUTOFMEMORY;

	for (long i = 0; i < helper.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_BSTR;
		var.bstrVal = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(files[i]).get_ptr());

		if (FAILED(helper.put(i, var)))
		{
			// deep destroy
			helper.reset();
			return E_OUTOFMEMORY;
		}
	}

	p->vt = VT_ARRAY | VT_VARIANT;
	p->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP WSHUtils::FileTest(BSTR path, BSTR mode, VARIANT * p)
{
	TRACK_FUNCTION();

	if (!path || !mode) return E_INVALIDARG;
	if (!p) return E_POINTER;

	if (wcscmp(mode, L"e") == 0)  // exists
	{
		p->vt = VT_BOOL;
		p->boolVal = TO_VARIANT_BOOL(PathFileExists(path));
	}
	else if (wcscmp(mode, L"s") == 0)
	{
		HANDLE fh = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);  
		LARGE_INTEGER size = {0};

		if (fh != INVALID_HANDLE_VALUE)
		{
			GetFileSizeEx(fh, &size);
			CloseHandle(fh);
		}

		// Only 32bit integers...
		p->vt = VT_UI4;
		p->ulVal = (size.HighPart) ? pfc::infinite32 : size.LowPart;
	}
	else if (wcscmp(mode, L"d") == 0)
	{
		p->vt = VT_BOOL;
		p->boolVal = TO_VARIANT_BOOL(PathIsDirectory(path));
	}
	else if (wcscmp(mode, L"split") == 0)
	{
		const wchar_t * fn = PathFindFileName(path);
		const wchar_t * ext = PathFindExtension(fn);
		wchar_t dir[MAX_PATH] = {0};
		helpers::com_array_writer<> helper;
		_variant_t vars[3];

		if (!helper.create(_countof(vars))) return E_OUTOFMEMORY;

		vars[0].vt = VT_BSTR;
		vars[0].bstrVal = NULL;
		vars[1].vt = VT_BSTR;
		vars[1].bstrVal = NULL;
		vars[2].vt = VT_BSTR;
		vars[2].bstrVal = NULL;

		if (PathIsFileSpec(fn))
		{
			StringCchCopyN(dir, _countof(dir), path, fn - path);
			PathAddBackslash(dir);

			vars[0].bstrVal = SysAllocString(dir);
			vars[1].bstrVal = SysAllocStringLen(fn, ext - fn);
			vars[2].bstrVal = SysAllocString(ext);
		}
		else
		{
			StringCchCopy(dir, _countof(dir), path);
			PathAddBackslash(dir);

			vars[0].bstrVal = SysAllocString(dir);
		}

		for (long i = 0; i < helper.get_count(); ++i)
		{
			if (FAILED(helper.put(i, vars[i])))
			{
				helper.reset();
				return E_OUTOFMEMORY;
			}
		}

		p->vt = VT_VARIANT | VT_ARRAY;
		p->parray = helper.get_ptr();
	}
	else if (wcscmp(mode, L"chardet") == 0)
	{
		p->vt = VT_UI4;
		p->ulVal = helpers::detect_charset(pfc::stringcvt::string_utf8_from_wide(path));
	}
	//{
	//    p->vt = VT_BSTR;
	//    const char * codepage = helpers::detect_charset_moz_chardet(pfc::stringcvt::string_utf8_from_wide(path));
	//    if (!codepage)
	//    {
	//        p->bstrVal = NULL;
	//    }
	//    else
	//    {
	//        p->bstrVal = SysAllocString(pfc::stringcvt::string_wide_from_ansi(codepage));
	//    }
	//}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

StyleTextRender::StyleTextRender(bool pngmode) : m_pOutLineText(NULL), m_pngmode(pngmode)
{
	if (!pngmode)
		m_pOutLineText = new TextDesign::OutlineText;
	else
		m_pOutLineText = new TextDesign::PngOutlineText;
}

void StyleTextRender::FinalRelease()
{
	if (m_pOutLineText)
	{
		delete m_pOutLineText;
		m_pOutLineText = NULL;
	}
}

STDMETHODIMP StyleTextRender::OutLineText(int text_color, int outline_color, int outline_width)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;

	m_pOutLineText->TextOutline(text_color, outline_color, outline_width);
	return S_OK;
}

STDMETHODIMP StyleTextRender::DoubleOutLineText(int text_color, int outline_color1, int outline_color2, int outline_width1, int outline_width2)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;

	m_pOutLineText->TextDblOutline(text_color, outline_color1, outline_color2, outline_width1, outline_width2);
	return S_OK;
}

STDMETHODIMP StyleTextRender::GlowText(int text_color, int glow_color, int glow_width)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;

	m_pOutLineText->TextGlow(text_color, glow_color, glow_width);
	return S_OK;
}

STDMETHODIMP StyleTextRender::EnableShadow(VARIANT_BOOL enable)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;

	m_pOutLineText->EnableShadow(enable == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP StyleTextRender::ResetShadow()
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;

	m_pOutLineText->SetNullShadow();
	return S_OK;
}

STDMETHODIMP StyleTextRender::Shadow(VARIANT color, int thickness, int offset_x, int offset_y)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;
	m_pOutLineText->Shadow(ExtractColorFromVariant(color), thickness, Gdiplus::Point(offset_x, offset_y));
	return S_OK;
}

STDMETHODIMP StyleTextRender::DiffusedShadow(VARIANT color, int thickness, int offset_x, int offset_y)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;
	m_pOutLineText->DiffusedShadow(ExtractColorFromVariant(color), thickness, Gdiplus::Point(offset_x, offset_y));
	return S_OK;
}

STDMETHODIMP StyleTextRender::SetShadowBackgroundColor(VARIANT color, int width, int height)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;
	m_pOutLineText->SetShadowBkgd(ExtractColorFromVariant(color), width, height);
	return S_OK;
}

STDMETHODIMP StyleTextRender::SetShadowBackgroundImage(IGdiBitmap * img)
{
	TRACK_FUNCTION();

	if (!m_pOutLineText) return E_POINTER;
	if (!img) return E_INVALIDARG;

	Gdiplus::Bitmap * pBitmap = NULL;
	img->get__ptr((void**)&pBitmap);
	if (!pBitmap) return E_INVALIDARG;
	m_pOutLineText->SetShadowBkgd(pBitmap);
	return S_OK;
}

STDMETHODIMP StyleTextRender::RenderStringPoint(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, int flags, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p || !m_pOutLineText) return E_POINTER;
	if (!g || !font || !str) return E_INVALIDARG;

	Gdiplus::Font * fn = NULL;
	Gdiplus::Graphics * graphics = NULL;
	font->get__ptr((void**)&fn);
	g->get__ptr((void**)&graphics);
	if (!fn || !graphics) return E_INVALIDARG;

	Gdiplus::FontFamily family;

	fn->GetFamily(&family);

	int fontstyle = fn->GetStyle();
	int fontsize = static_cast<int>(fn->GetSize());
	Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());

	if (flags != 0)
	{
		fmt.SetAlignment((Gdiplus::StringAlignment)((flags >> 28) & 0x3));      //0xf0000000
		fmt.SetLineAlignment((Gdiplus::StringAlignment)((flags >> 24) & 0x3));  //0x0f000000
		fmt.SetTrimming((Gdiplus::StringTrimming)((flags >> 20) & 0x7));        //0x00f00000
		fmt.SetFormatFlags((Gdiplus::StringFormatFlags)(flags & 0x7FFF));       //0x0000ffff
	}

	m_pOutLineText->DrawString(graphics, &family, (Gdiplus::FontStyle)fontstyle, 
		fontsize, str, Gdiplus::Point(x, y), &fmt);
	return S_OK;
}

STDMETHODIMP StyleTextRender::RenderStringRect(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, int w, int h, int flags, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p || !m_pOutLineText) return E_POINTER;
	if (!g || !font || !str) return E_INVALIDARG;

	Gdiplus::Font * fn = NULL;
	Gdiplus::Graphics * graphics = NULL;
	font->get__ptr((void**)&fn);
	g->get__ptr((void**)&graphics);
	if (!fn || !graphics) return E_INVALIDARG;

	Gdiplus::FontFamily family;

	fn->GetFamily(&family);

	int fontstyle = fn->GetStyle();
	int fontsize = static_cast<int>(fn->GetSize());
	Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());

	if (flags != 0)
	{
		fmt.SetAlignment((Gdiplus::StringAlignment)((flags >> 28) & 0x3));      //0xf0000000
		fmt.SetLineAlignment((Gdiplus::StringAlignment)((flags >> 24) & 0x3));  //0x0f000000
		fmt.SetTrimming((Gdiplus::StringTrimming)((flags >> 20) & 0x7));        //0x00f00000
		fmt.SetFormatFlags((Gdiplus::StringFormatFlags)(flags & 0x7FFF));       //0x0000ffff
	}

	m_pOutLineText->DrawString(graphics, &family, (Gdiplus::FontStyle)fontstyle, 
		fontsize, str, Gdiplus::Rect(x, y, w, h), &fmt);
	return S_OK;
}

STDMETHODIMP StyleTextRender::SetPngImage(IGdiBitmap * img)
{
	TRACK_FUNCTION();

	if (!m_pngmode) return E_NOTIMPL;
	if (!img) return E_INVALIDARG;
	if (!m_pOutLineText) return E_POINTER;

	Gdiplus::Bitmap * pBitmap = NULL;
	img->get__ptr((void**)&pBitmap);
	if (!pBitmap) return E_INVALIDARG;
	TextDesign::PngOutlineText * pPngOutlineText = reinterpret_cast<TextDesign::PngOutlineText *>(m_pOutLineText);
	pPngOutlineText->SetPngImage(pBitmap);
	return S_OK;
}

STDMETHODIMP ThemeManager::SetPartAndStateID(int partid, int stateid)
{
	TRACK_FUNCTION();

	if (!m_theme) return E_POINTER;

	m_partid = partid;
	m_stateid = stateid;
	return S_OK;
}

STDMETHODIMP ThemeManager::IsThemePartDefined(int partid, int stateid, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!m_theme) return E_POINTER;
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(::IsThemePartDefined(m_theme, partid, stateid));
	return S_OK;
}

STDMETHODIMP ThemeManager::DrawThemeBackground(IGdiGraphics * gr, int x, int y, int w, int h, int clip_x, int clip_y, int clip_w, int clip_h)
{
	TRACK_FUNCTION();

	if (!m_theme) return E_POINTER;
	if (!gr) return E_INVALIDARG;

	Gdiplus::Graphics * graphics = NULL;
	gr->get__ptr((void **)&graphics);
	if (!graphics) return E_INVALIDARG;

	RECT rc = { x, y, x + w, y + h};
	RECT clip_rc = { clip_x, clip_y, clip_x + clip_w, clip_y + clip_h};
	LPCRECT pclip_rc = &clip_rc;
	HDC dc = graphics->GetHDC();

	if (clip_x == 0 && clip_y == 0 && clip_w == 0 && clip_h == 0)
	{
		pclip_rc = NULL;
	}

	HRESULT hr = ::DrawThemeBackground(m_theme, dc, m_partid, m_stateid, &rc, pclip_rc);

	graphics->ReleaseHDC(dc);
	return hr;
}

STDMETHODIMP DropSourceAction::get_Parsable(VARIANT_BOOL * parsable)
{
	TRACK_FUNCTION();
	if (!parsable) return E_POINTER;
	*parsable = TO_VARIANT_BOOL(m_parsable);
	return S_OK;
}

STDMETHODIMP DropSourceAction::put_Parsable(VARIANT_BOOL parsable)
{
	TRACK_FUNCTION();
	m_parsable = parsable == VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_Mode(int * mode)
{
	TRACK_FUNCTION();
	if (!mode) return E_POINTER;
	*mode = m_action_mode;
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_Playlist(int * id)
{
	TRACK_FUNCTION();
	if (!id) return E_POINTER;
	*id = m_playlist_idx;
	return S_OK;
}

STDMETHODIMP DropSourceAction::put_Playlist(int id)
{
	TRACK_FUNCTION();
	m_playlist_idx = id;
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_ToSelect(VARIANT_BOOL * select)
{
	TRACK_FUNCTION();
	if (!select) return E_POINTER;
	*select = TO_VARIANT_BOOL(m_to_select);
	return S_OK;
}

STDMETHODIMP DropSourceAction::put_ToSelect(VARIANT_BOOL select)
{
	TRACK_FUNCTION();
	m_to_select = (select == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP DropSourceAction::ToPlaylist()
{
	TRACK_FUNCTION();
	m_action_mode = kActionModePlaylist;
	return S_OK;
}
