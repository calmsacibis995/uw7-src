/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/cfb.w32/w32teblt8.c,v 3.3 1995/01/28 15:50:40 dawes Exp $ */
/*
 * TEGblt - ImageText expanded glyph fonts only.  For
 * 8 bit displays, in Copy mode with no clipping.
 */

/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
*/
/* $XConsortium: w32teblt8.c /main/2 1995/11/12 16:18:22 kaleb $ */

#if PSZ == 8

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#include	"w32itext.h"

/*
 * this code supports up to 5 characters at a time.  The performance
 * differences between 4 and 5 is usually small (~7% on PMAX) and
 * frequently negative (SPARC and Sun3), so this file is compiled
 * only once for now.  If you want to use the other options, you'll
 * need to hack cfbgc.c as well.
 */

#define CFBTEGBLT8 W32TEGlyphBlt8

/*
 * On little-endian machines (or where fonts are padded to 32-bit
 * boundaries) we can use some magic to avoid the expense of getleftbits
 */

typedef unsigned int	*glyphPointer;

void
CFBTEGBLT8 (pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	xInit, yInit;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer	pglyphBase;	/* start of array of glyphs */
{
    register unsigned long  c;
    register unsigned long  leftMask, rightMask;
    register int	    hTmp;
    register int	    xoff1;
    register glyphPointer   char1;

    FontPtr		pfont = pGC->font;
    unsigned long	*pdstBase, stipple;
    int			widthDst;
    int			widthGlyph;
    int			h, i, j, k, dst, dst2, dst_pitch, string_width, bytes;
    int			x, y;
    BoxRec		bbox;		/* for clipping */
    int			widthGlyphs;
    long		text_buffer, text1, text2, line_hop;
    LongP		p;
    void		(*w32_text)();

    cfbGetLongWidthAndPointer(pDrawable, widthDst, pdstBase)

    if ((CARD32)pdstBase != VGABASE)
    {
	cfbTEGlyphBlt8 (pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase);
	return;
    }

    widthGlyph = FONTMAXBOUNDS(pfont,characterWidth);
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);
    if (!h)
	return;

    line_hop = widthDst << 2;
    string_width = widthGlyph * nglyph;
    dst_pitch = line_hop - string_width;

    x = xInit + FONTMAXBOUNDS(pfont,leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    bbox.x1 = x;
    bbox.x2 = x + string_width;
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch (RECT_IN_REGION(pGC->pScreen,  cfbGetCompositeClip(pGC), &bbox))
    {
      case rgnPART:
	cfbImageGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase);
      case rgnOUT:
	return;
    }
  
    bytes = (widthGlyph + 7) >> 3;
    dst = y * (widthDst << 2) + x;
    if (W32OrW32i)
    {
	W32_INIT_IMAGE_TEXT(PFILL(pGC->fgPixel), PFILL(pGC->bgPixel),
			    line_hop - 1, widthGlyph, h)
	switch (bytes)
	{
	    case 1:
		w32_text = W32ImageText1;
		break;
	    case 2:
		w32_text = W32ImageText2;
		break;
	    case 3:
		w32_text = W32ImageText3;
		break;
	    case 4:
		w32_text = W32ImageText4;
		break;
	}
	for (j = 0; j < nglyph; j++)
	{
	    char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    *MBP2 = dst;
	    (*w32_text)(h, char1);
	    dst += widthGlyph;
	}
    }
    else /* w32p */
    {
	W32P_INIT_IMAGE_TEXT(PFILL(pGC->fgPixel), PFILL(pGC->bgPixel),
			     line_hop - 1, widthGlyph, h)
	switch (bytes)
	{
	    case 1:
		w32_text = W32pImageText1;
		break;
	    case 2:
		w32_text = W32pImageText2;
		break;
	    case 3:
		w32_text = W32pImageText3;
		break;
	    case 4:
		w32_text = W32pImageText4;
		break;
	}
	for (i = 0; i < nglyph; i++)
	{
	    char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    *ACL_DESTINATION_ADDRESS = dst;
	    (*w32_text)(h, char1);
	    dst += widthGlyph;
	}
    }

}
#endif /* PSZ == 8 */
