#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiFont.c	1.1"

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <ati.h>

/*	HARDWARE FONT CONTROL		*/
/*		OPTIONAL		*/

SIBool atiCheckDLFont(fontnum, fontInfo)
SIint32 fontnum;
SIFontInfoP fontInfo;

{
    int w, h, nx, ny;

    if (fontnum >= ATI_NUM_FONTS)
	return(SI_FAIL);

    w = fontInfo->SFmax.SFrbearing - fontInfo->SFmin.SFlbearing;
    h = fontInfo->SFmax.SFascent + fontInfo->SFmax.SFascent;

    nx = lfb.stride / w;
    ny = (fontInfo->SFnumglyph + nx - 1) / nx;

    if (ny * h > ati.font_h)
	return(SI_FAIL);

    return(SI_SUCCEED);
}

SIBool atiDownLoadFont(fontnum, fontInfo, glyphlist)
SIint32 fontnum;
SIFontInfoP fontInfo;
SIGlyphP glyphlist;

{
    int w, h, nx, ny;
    int nchars;
    unsigned int m = 1 << fontnum, nm = ~m;
    SIbitmapP bmap;
    u_long d, *p;
    int n, x, y, i, j;
    SIGlyphP glP;

    if (fontnum >= ATI_NUM_FONTS)
	return(SI_FAIL);

    nchars = fontInfo->SFnumglyph;
    w = fontInfo->SFmax.SFrbearing - fontInfo->SFmin.SFlbearing;
    h = fontInfo->SFmax.SFascent + fontInfo->SFmax.SFascent;

    nx = lfb.stride / w;
    ny = (nchars + nx - 1) / nx;

    if (ny * h > ati.font_h)
	return(SI_FAIL);

    if (atiGlyphs[fontnum] == NULL) {
	glP = (SIGlyphP)malloc(nchars * sizeof(SIGlyph));
    }
    else
	glP = (SIGlyphP)realloc(atiGlyphs[fontnum],
				nchars * sizeof(SIGlyph));

    if (glP == NULL) {
	fprintf(stderr, "Download font: malloc/realloc failure\n");
	return(SI_FAIL);
    }
    
    atiGlyphs[fontnum] = glP;
    atiFontInfo[fontnum] = *fontInfo;

    memmove(glP, glyphlist, nchars * sizeof(SIGlyph));

    nx *= w;
    ny *= h;
    
    n = 0;
    for (y = 0; (y < ny) && (n < nchars); y += h) {
	for (x = 0; (x < nx) && (n < nchars); x += w, n++) {
	    bmap = &glP[n].SFglyph;
	    for (i = 0; i < bmap->Bheight; i++) {
		p = (u_long *)BitmapScanL(bmap, i);
		for (j = 0; j < bmap->Bwidth; j++) {
		    if ((j & 31) == 0)
			d = *p++;

		    if (d & 1)
			*ScreenAddr(x+j, y+i+ati.font_off) |= m;
		    else
			*ScreenAddr(x+j, y+i+ati.font_off) &= nm;

		    d >>= 1;
		}
	    }
	}
    }

    return(SI_SUCCEED);
}

SIBool atiFreeFont(fontnum)
SIint32 fontnum;

{
    if (fontnum >= ATI_NUM_FONTS)
	return(SI_FAIL);

    free(atiGlyphs[fontnum]);
    atiGlyphs[fontnum] = NULL;
    atiFontInfo[fontnum].SFnumglyph = 0;

    return(SI_SUCCEED);
}

SIBool atiStplbltFont(fontnum, x, y, count, glyphNums, forcetype)
SIint32 fontnum, x, y, count, forcetype;
SIint16 *glyphNums;

{
    int smode, dmode, i;
    SIGStateP gstateP = lfb_cur_GStateP;
    SIGlyphP glyphs = atiGlyphs[fontnum], gl;
    SIFontInfoP fontInfo = &atiFontInfo[fontnum];
    int nx, ny, w, h, n, dx, dy;
    int gx, gy, gw, gh;

    if (fontnum >= ATI_NUM_FONTS)
	return(SI_FAIL);

    if (! atiClipping)
	atiClipOn();

    dmode = ati_mode_trans[gstateP->SGmode];

    if (forcetype)
	smode = forcetype;
    else
	smode = gstateP->SGstplmode;

    ATI_NEED_FIFO(3);
    outw(WRT_MASK, gstateP->SGpmask);
    outw(ALU_FG_FN, dmode);
    outw(ALU_BG_FN, 0x03);

    if (smode == SGOPQStipple) {
	int w = 0;

 	/* 
 	 * The spec for ImageText is to ignore the function and use
 	 * GXcopy.  Reset ALU_FG_FN here to accomodate that.
 	 *
	 */

	ATI_NEED_FIFO(6);
	outw(ALU_FG_FN, 0x7);
	outw(FRGD_COLOR, gstateP->SGbg);
	outw(DP_CONFIG, 0x2011);
	outw(CUR_X, x);
	outw(CUR_Y, y - atiFontInfo[fontnum].SFlascent);
	outw(DEST_X_START, x);

	for (i = 0; i < count; i++)
	    w += (glyphs + glyphNums[i])->SFwidth;

	ATI_NEED_FIFO(2);
	outw(DEST_X_END, x + w);
	outw(DEST_Y_END, y + atiFontInfo[fontnum].SFldescent);
    }

    ATI_NEED_FIFO(3);
    outw(FRGD_COLOR, gstateP->SGfg);
    outw(RD_MASK, 1 << fontnum);
    outw(DP_CONFIG, 0x2071);

    w = fontInfo->SFmax.SFrbearing - fontInfo->SFmin.SFlbearing;
    h = fontInfo->SFmax.SFascent + fontInfo->SFmax.SFascent;

    nx = lfb.stride / w;
    ny = (fontInfo->SFnumglyph + nx - 1) / nx;

    for (i = 0; i < count; i++) {
	n = glyphNums[i];
	gl = glyphs + n;

	gx = (n % nx) * w;
	gy = ati.font_off + (n / nx) * h;
	gw = gl->SFrbearing - gl->SFlbearing;
	gh = gl->SFascent + gl->SFdescent;

	ATI_NEED_FIFO(5);
	outw(SRC_X, gx);
	outw(SRC_X_START, gx);
	outw(SRC_X_END, gx + gw);
	outw(SRC_Y, gy);
	outw(SRC_Y_DIR, 1);

	dx = x + gl->SFlbearing;
	dy = y - gl->SFascent;

	ATI_NEED_FIFO(5);
	outw(CUR_X, dx);
	outw(DEST_X_START, dx);
	outw(DEST_X_END, dx + gw);
	outw(CUR_Y, dy);
	outw(DEST_Y_END, dy + gh);

	x += gl->SFwidth;
    }

    return(SI_SUCCEED);
}
