/*
Text Designer Outline Text Library 0.2.9

Copyright (c) 2009 Wong Shao Voon

The Code Project Open License (CPOL)
http://www.codeproject.com/info/cpol10.aspx
*/

#ifndef _GDIPATH_H_
#define _GDIPATH_H_

#include <Gdiplus.h>

namespace TextDesign
{

class GDIPath
{
public:
	//!  default constructor
	GDIPath(void);
	//! destructor
	~GDIPath(void);

public:
	/** Get the text path using GDI DrawText, using a rectangle
	
	@param[in]		pGraphics is the graphics context
	@param[out]	ppPath will be filled with the GDI paths
	@param[in]		pszText is the text for the path
	@param[in]		plf is the LOGFONT which must be filled for the font to be created
	@param[in]		rtDraw is the rect where the drawing will be centralized.
	@return true for success
	*/
	static bool GetStringPath(
		Gdiplus::Graphics* pGraphics, 
		Gdiplus::GraphicsPath** ppPath, 
		const wchar_t* pszText, 
		LOGFONTW* plf,	
		Gdiplus::Rect rtDraw);

	/** Measure the maximum width and height needed to draw this path

	@param[in]		pGraphics is the graphics context
	@param[in]	    pGraphicsPath which will be used for calculation
	@param[out]		pfPixelsWidth is the destination pixels width
	@param[out]		pfPixelsHeight is the destination pixels height
	@return true for success
	*/
	static bool MeasureGraphicsPath(
		Gdiplus::Graphics* pGraphics,
		Gdiplus::GraphicsPath* pGraphicsPath,
		float* pfPixelsWidth,
		float* pfPixelsHeight);

	/** Measure the maximum width and height needed to draw this path

	@param[in]		pGraphics is the graphics context
	@param[in]	    pGraphicsPath which will be used for calculation
	@param[out]		pfPixelsWidth is the destination pixels width
	@param[out]		pfPixelsHeight is the destination pixels height
	@return true for success
	*/
	static bool MeasureGraphicsPathRealHeight(
		Gdiplus::Graphics* pGraphics,
		Gdiplus::GraphicsPath* pGraphicsPath,
		float* pfPixelsWidth,
		float* pfPixelsHeight);

	/** Convert the units to pixel unit

	@param[in]		pGraphics is the graphics context to get the unit type used currently
	@param[in]	    fSrcWidth is the source unit width
	@param[in]	    fSrcHeight is the source unit height
	@param[out]		pfDestWidth is the destination pixels width
	@param[out]		pfDestHeight is the destination pixels height
	@return true for success
	*/
	static bool ConvertToPixels(
		Gdiplus::Graphics* pGraphics,
		float fSrcWidth,
		float fSrcHeight,
		float* pfDestWidth,
		float* pfDestHeight );


protected:
	/** PolyDraw the path
	
	@param[out]	pPath will be filled with the GDI paths
	@param[in]		lppt is points to be plotted
	@param[in]		lpbTypes is the types, eg PT_LINETO, PT_BEZIERTO, etc for the each point in lppt
	@param[in]		cCount is the total counts of lppt
	*/
	static void PolyDraw(
		Gdiplus::GraphicsPath* pPath, 
		CONST LPPOINT lppt, 
		CONST LPBYTE lpbTypes,
		int cCount );

};

} // namespace TextDesign

#endif // _GDIPATH_H_
