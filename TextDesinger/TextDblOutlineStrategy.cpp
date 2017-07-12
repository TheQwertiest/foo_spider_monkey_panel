/*
Text Designer Outline Text Library 0.2.9

Copyright (c) 2009 Wong Shao Voon

The Code Project Open License (CPOL)
http://www.codeproject.com/info/cpol10.aspx
*/

#include "StdAfx.h"
#include "TextDblOutlineStrategy.h"
#include "GDIPath.h"

using namespace TextDesign;

TextDblOutlineStrategy::TextDblOutlineStrategy(void)
:
m_nThickness1(2),
m_nThickness2(2)
{
}

TextDblOutlineStrategy::~TextDblOutlineStrategy(void)
{
}

void TextDblOutlineStrategy::Init(
	Gdiplus::Color clrText, 
	Gdiplus::Color clrOutline1, 
	Gdiplus::Color clrOutline2, 
	int nThickness1, 
	int nThickness2)
{
	m_clrText = clrText; 
	m_clrOutline1 = clrOutline1; 
	m_clrOutline2 = clrOutline2;
	m_nThickness1 = nThickness1; 
	m_nThickness2 = nThickness2;

}

bool TextDblOutlineStrategy::DrawString(
	Gdiplus::Graphics* pGraphics,
	Gdiplus::FontFamily* pFontFamily,
	Gdiplus::FontStyle fontStyle,
	int nfontSize,
	const wchar_t*pszText, 
	Gdiplus::Point ptDraw, 
	Gdiplus::StringFormat* pStrFormat)
{
	using namespace Gdiplus;
	GraphicsPath path;
	Status status = path.AddString(pszText,wcslen(pszText),pFontFamily,fontStyle,nfontSize,ptDraw,pStrFormat);
	if(status!=Ok)
		return false;

	Pen pen2(m_clrOutline2,m_nThickness1+m_nThickness2);
	pen2.SetLineJoin(LineJoinRound);
	pGraphics->DrawPath(&pen2, &path);
	Pen pen1(m_clrOutline1,m_nThickness1);
	pen1.SetLineJoin(LineJoinRound);
	pGraphics->DrawPath(&pen1, &path);

	SolidBrush brush(m_clrText);
	Status status2 = pGraphics->FillPath(&brush, &path);

	return status2 == Ok;
}

bool TextDblOutlineStrategy::DrawString(
	Gdiplus::Graphics* pGraphics,
	Gdiplus::FontFamily* pFontFamily,
	Gdiplus::FontStyle fontStyle,
	int nfontSize,
	const wchar_t*pszText, 
	Gdiplus::Rect rtDraw, 
	Gdiplus::StringFormat* pStrFormat)
{
	using namespace Gdiplus;
	GraphicsPath path;
	Status status = path.AddString(pszText,wcslen(pszText),pFontFamily,fontStyle,nfontSize,rtDraw,pStrFormat);
	if(status!=Ok)
		return false;

	Pen pen2(m_clrOutline2,m_nThickness1+m_nThickness2);
	pen2.SetLineJoin(LineJoinRound);
	pGraphics->DrawPath(&pen2, &path);
	Pen pen1(m_clrOutline1,m_nThickness1);
	pen1.SetLineJoin(LineJoinRound);
	pGraphics->DrawPath(&pen1, &path);

	SolidBrush brush(m_clrText);
	Status status2 = pGraphics->FillPath(&brush, &path);

	return status2 == Ok;
}

bool TextDblOutlineStrategy::MeasureString(
	Gdiplus::Graphics* pGraphics, 
	Gdiplus::FontFamily* pFontFamily,
	Gdiplus::FontStyle fontStyle,
	int nfontSize,
	const wchar_t*pszText, 
	Gdiplus::Point ptDraw, 
	Gdiplus::StringFormat* pStrFormat,
	float* pfDestWidth,
	float* pfDestHeight )
{
	using namespace Gdiplus;
	GraphicsPath path;
	Status status = path.AddString(pszText,wcslen(pszText),pFontFamily,fontStyle,nfontSize,ptDraw,pStrFormat);
	if(status!=Ok)
		return false;

	*pfDestWidth= ptDraw.X;
	*pfDestHeight= ptDraw.Y;
	bool b = GDIPath::MeasureGraphicsPath(pGraphics, &path, pfDestWidth, pfDestHeight);

	if(false==b)
		return false;

	float pixelThick = 0.0f;
	b = GDIPath::ConvertToPixels(pGraphics,m_nThickness1+m_nThickness2,0.0f,&pixelThick,NULL);

	if(false==b)
		return false;

	*pfDestWidth += pixelThick;
	*pfDestHeight += pixelThick;

	return true;
}

bool TextDblOutlineStrategy::MeasureString(
	Gdiplus::Graphics* pGraphics, 
	Gdiplus::FontFamily* pFontFamily,
	Gdiplus::FontStyle fontStyle,
	int nfontSize,
	const wchar_t*pszText, 
	Gdiplus::Rect rtDraw,
	Gdiplus::StringFormat* pStrFormat,
	float* pfDestWidth,
	float* pfDestHeight )
{
	using namespace Gdiplus;
	GraphicsPath path;
	Status status = path.AddString(pszText,wcslen(pszText),pFontFamily,fontStyle,nfontSize,rtDraw,pStrFormat);
	if(status!=Ok)
		return false;

	*pfDestWidth= rtDraw.GetLeft();
	*pfDestHeight= rtDraw.GetTop();
	bool b = GDIPath::MeasureGraphicsPath(pGraphics, &path, pfDestWidth, pfDestHeight);

	if(false==b)
		return false;

	float pixelThick = 0.0f;
	b = GDIPath::ConvertToPixels(pGraphics,m_nThickness1+m_nThickness2,0.0f,&pixelThick,NULL);

	if(false==b)
		return false;

	*pfDestWidth += pixelThick;
	*pfDestHeight += pixelThick;

	return true;
}
