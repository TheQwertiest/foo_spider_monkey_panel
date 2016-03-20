/*
Text Designer Outline Text Library 0.2.9

Copyright (c) 2009 Wong Shao Voon

The Code Project Open License (CPOL)
http://www.codeproject.com/info/cpol10.aspx
*/


#ifndef _OUTLINETEXT_H_
#define _OUTLINETEXT_H_

#include <Gdiplus.h>
#include "IOutlineText.h"
#include "ITextStrategy.h"
#include "TextDblOutlineStrategy.h"
#include "TextOutlineStrategy.h"
#include "TextGlowStrategy.h"
#include "DiffusedShadowStrategy.h"

namespace TextDesign
{

class OutlineText : public IOutlineText
{
public:
	//!  default constructor
	OutlineText(void);
	//! destructor
	virtual ~OutlineText(void);

	/** Setting Text Glow effect
	
	@param[in]		clrText is the text color
	@param[in]		clrOutline is the outline color, alpha should be 12 to 32 for best glow effect
	@param[in]		nThickness is the outline thickness which should be 8 or greater for best glow effext
	*/
	void TextGlow(
		Gdiplus::Color clrText, 
		Gdiplus::Color clrOutline, 
		int nThickness);

	/** Setting  Outlined Text effect
	
	@param[in]		clrText is the text color
	@param[in]		clrOutline is the outline color
	@param[in]		nThickness is the outline thickness
	*/
	void TextOutline(
		Gdiplus::Color clrText, 
		Gdiplus::Color clrOutline, 
		int nThickness);

	/** Double Outline Text
	
	@param[in]		clrText is the text color
	@param[in]		clrOutline1 is the inner outline color
	@param[in]		clrOutline2 is the outer outline color
	@param[in]		nThickness1 is the inner outline thickness
	@param[in]		nThickness2 is the outer outline thickness
	*/
	void TextDblOutline(
		Gdiplus::Color clrText, 
		Gdiplus::Color clrOutline1, 
		Gdiplus::Color clrOutline2, 
		int nThickness1, 
		int nThickness2);

	//! Set the shadow bitmap with a bitmap
	void SetShadowBkgd(Gdiplus::Bitmap* pBitmap);

	//! Set the shadow bitmap with a color, width and height
	void SetShadowBkgd(Gdiplus::Color clrBkgd, int nWidth, int nHeight);

	//! Set shadow to nothing, reseting previous shadow settings
	void SetNullShadow();
	
	//! Enable shadow
	void EnableShadow(bool bEnable);

	/** Shadow Settings
	
	@param[in]		color is the text color
	@param[in]		nThickness is the shadow outline thickness
	@param[in]		ptOffset is the relative offset
	*/
	void Shadow(
		Gdiplus::Color color, 
		int nThickness,
		Gdiplus::Point ptOffset);

	/** Diffused Shadow Settings

	@param[in]		color is the text color
	@param[in]		nThickness is the shadow outline thickness
	@param[in]		ptOffset is the relative offset
	*/
	void DiffusedShadow(
		Gdiplus::Color color, 
		int nThickness,
		Gdiplus::Point ptOffset);

	/** Draw String, using a point as the starting point
	
	@param[in]		pGraphics is the graphics context
	@param[in]		pFontFamily is font family which is used(Collection of similar fonts)
	@param[in]		fontStyle like Bold, Italic or Regular should be specified.
	@param[in]		nfontSize is font size
	@param[in]		pszText is the text which is displayed.
	@param[in]		ptDraw is the staring point to draw
	@param[in]		pStrFormat is the string format to be specified(can be left at default)
	@return true for success
	*/
	bool DrawString(
		Gdiplus::Graphics* pGraphics, 
		Gdiplus::FontFamily* pFontFamily,
		Gdiplus::FontStyle fontStyle,
		int nfontSize,
		const wchar_t*pszText, 
		Gdiplus::Point ptDraw, 
		Gdiplus::StringFormat* pStrFormat);

	/** Draw String, using a rectangle
	
	@param[in]		pGraphics is the graphics context
	@param[in]		pFontFamily is font family which is used(Collection of similar fonts)
	@param[in]		fontStyle like Bold, Italic or Regular should be specified.
	@param[in]		nfontSize is font size
	@param[in]		pszText is the text which is displayed.
	@param[in]		rtDraw is the rectangle where the whole drawing will be centralized
	@param[in]		pStrFormat is the string format to be specified(can be left at default)
	@return true for success
	*/
	bool DrawString(
		Gdiplus::Graphics* pGraphics, 
		Gdiplus::FontFamily* pFontFamily,
		Gdiplus::FontStyle fontStyle,
		int nfontSize,
		const wchar_t*pszText, 
		Gdiplus::Rect rtDraw,
		Gdiplus::StringFormat* pStrFormat);

	/** Measure String, using a point as the starting point

	@param[in]		pGraphics is the graphics context
	@param[in]		pFontFamily is font family which is used(Collection of similar fonts)
	@param[in]		fontStyle like Bold, Italic or Regular should be specified.
	@param[in]		nfontSize is font size
	@param[in]		pszText is the text which is displayed.
	@param[in]		ptDraw is the staring point to draw
	@param[in]		pStrFormat is the string format to be specified(can be left at default)
	@param[out]		pfDestWidth is the destination pixels width
	@param[out]		pfDestHeight is the destination pixels height
	@return true for success
	*/
	bool MeasureString(
		Gdiplus::Graphics* pGraphics, 
		Gdiplus::FontFamily* pFontFamily,
		Gdiplus::FontStyle fontStyle,
		int nfontSize,
		const wchar_t*pszText, 
		Gdiplus::Point ptDraw, 
		Gdiplus::StringFormat* pStrFormat,
		float* pfDestWidth,
		float* pfDestHeight );

	/** Measure String, using a rectangle

	@param[in]		pGraphics is the graphics context
	@param[in]		pFontFamily is font family which is used(Collection of similar fonts)
	@param[in]		fontStyle like Bold, Italic or Regular should be specified.
	@param[in]		nfontSize is font size
	@param[in]		pszText is the text which is displayed.
	@param[in]		rtDraw is the rectangle where the whole drawing will be centralized
	@param[in]		pStrFormat is the string format to be specified(can be left at default)
	@param[out]		pfDestWidth is the destination pixels width
	@param[out]		pfDestHeight is the destination pixels height
	@return true for success
	*/
	bool MeasureString(
		Gdiplus::Graphics* pGraphics, 
		Gdiplus::FontFamily* pFontFamily,
		Gdiplus::FontStyle fontStyle,
		int nfontSize,
		const wchar_t*pszText, 
		Gdiplus::Rect rtDraw,
		Gdiplus::StringFormat* pStrFormat,
		float* pfDestWidth,
		float* pfDestHeight );

protected:
	/** Render Font Body Shadow(not the outline!)  using a specified starting point, using GDI+ paths
	
	@param[in]		pGraphics is the graphics context
	@param[in]		pFontFamily is font family which is used(Collection of similar fonts)
	@param[in]		fontStyle like Bold, Italic or Regular should be specified.
	@param[in]		nfontSize is font size
	@param[in]		pszText is the text which is displayed.
	@param[in]		ptDraw is the staring point to draw
	@param[in]		pStrFormat is the string format to be specified(can be left at default)
	*/
	void RenderFontShadow(
		Gdiplus::Graphics* pGraphics,
		Gdiplus::FontFamily* pFontFamily,
		Gdiplus::FontStyle fontStyle,
		int nfontSize,
		const wchar_t*pszText, 
		Gdiplus::Point ptDraw, 
		Gdiplus::StringFormat* pStrFormat);

	/** Render Font Body Shadow(not the outline!) using a specified rectangle, using GDI+ paths
	
	@param[in]		pGraphics is the graphics context
	@param[in]		pFontFamily is font family which is used(Collection of similar fonts)
	@param[in]		fontStyle like Bold, Italic or Regular should be specified.
	@param[in]		nfontSize is font size
	@param[in]		pszText is the text which is displayed.
	@param[in]		rtDraw is the rectangle where the whole drawing will be centralized
	@param[in]		pStrFormat is the string format to be specified(can be left at default)
	*/
	void RenderFontShadow(
		Gdiplus::Graphics* pGraphics,
		Gdiplus::FontFamily* pFontFamily,
		Gdiplus::FontStyle fontStyle,
		int nfontSize,
		const wchar_t*pszText, 
		Gdiplus::Rect rtDraw, 
		Gdiplus::StringFormat* pStrFormat);

	/** Render Font Body Shadow(not the outline!)  using a specified starting point, using GDI paths
	
	@param[in]		pGraphics is the graphics context
	@param[in]		pLogFont is the LOGFONT from which the font will be created.
	@param[in]		pszText is the text which is displayed.
	@param[in]		ptDraw is the staring point to draw
	*/
	void GdiRenderFontShadow(	
		Gdiplus::Graphics* pGraphics, 
		LOGFONTW* pLogFont,
		const wchar_t*pszText, 
		Gdiplus::Point ptDraw);

	/** Render Font Body Shadow(not the outline!)  using a specified rectangle, using GDI paths
	
	@param[in]		pGraphics is the graphics context
	@param[in]		pLogFont is the LOGFONT from which the font will be created.
	@param[in]		pszText is the text which is displayed.
	@param[in]		rtDraw is the rectangle where the whole drawing will be centralized
	*/
	void GdiRenderFontShadow(	
		Gdiplus::Graphics* pGraphics, 
		LOGFONTW* pLogFont,
		const wchar_t*pszText, 
		Gdiplus::Rect rtDraw);

	/** Alphablend function
	
	@param[in]		dest is the destination color
	@param[in]		source is the source color
	@param[in]		nAlpha is the alpha
	@param[in]		rtDraw is the rectangle where the whole drawing will be centralized
	@return blended color
	*/
	inline UINT Alphablend(UINT dest, UINT source, BYTE nAlpha);
	
	//! Text effect strategy polymorphic pointer
	ITextStrategy* m_pTextStrategy;
	//! Shadow effect strategy polymorphic pointer to draw the outline(Right now only 1 effect, TextOutlineStrategy)
	ITextStrategy* m_pShadowStrategy;
	//! Object to draw the text body, not the outline
	ITextStrategy* m_pFontBodyShadow;
	//! Specify how much to offset the shadow relatively.
	Gdiplus::Point m_ptShadowOffset;
	//! Shadow color
	Gdiplus::Color m_clrShadow;
	//! Background Bitmap for the shadow because shadow is transparent.
	Gdiplus::Bitmap* m_pBkgdBitmap;
	//! Enable Shadow
	bool m_bEnableShadow;
	//! DiffuseShadow
	bool m_bDiffuseShadow;
	//! Shadow Thickness
	int m_nShadowThickness;
};

} // namespace TextDesign

#endif // _OUTLINETEXT_H_