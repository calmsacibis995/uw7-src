/*
 *	@(#) qvisProcs.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 21:49:14 PDT 1992	mikep@sco.com
 *	- Add qvisDrawBankedPoints()
 *	S001	Sun Oct 11 16:17:19 PDT 1992	mikep@sco.com
 *	- Add qvisSolidZeroSegPtToPt()
 *	S002	Thu Feb 11 10:34:17 PST 1993	mikep@sco.com
 *	- Incorperate Compaq changes
 *      S003    Tue Jul 13 12:45:37 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 *      S004    Tue Sep 20 14:32:18 PDT 1994    davidw@sco.com
 *      - Correct compiler warnings.
 *	- 2nd arg to qvisGlyphCacheClearFont changed to unique 
 *	  structureID in everest.  Key off of agaII.
 */

/**
 *
 * qvisProcs.h
 *
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mjk       04/07/92  Originated (see RCS log)
 * waltc     02/04/93  Remove qvisSolidZeroSegPtToPt().
 * waltc     06/07/93  Remove qvisHelpValidateGC
 */

#ifndef _QVIS_PROCS_H
#define _QVIS_PROCS_H

/*
 * qvisProcs.h
 *
 * routines for the "qvis" port
 */

#include "qvisDefs.h"
#include "nfb/nfbGlyph.h"

/*
 * qvisCmap.c
 */

extern void qvisSetColor(
			    unsigned int cmap,
			    unsigned int index,
			    unsigned short r,
			    unsigned short g,
			    unsigned short b,
			    ScreenPtr pScreen);

extern void qvisRestoreColormap(
				   ScreenPtr pScreen);

/*
 * qvisCursor.c
 */

extern void qvisInstallCursor(
				 unsigned long int *image,
				 unsigned int hotx,
				 unsigned int hoty,
				 ScreenPtr pScreen);

extern void qvisSetCursorPos(
			        unsigned int x,
			        unsigned int y,
			        ScreenPtr pScreen);

extern void qvisCursorOn(
			    int on,
			    ScreenPtr pScreen);

extern void qvisSetCursorColor(
				  unsigned short fr,
				  unsigned short fg,
				  unsigned short fb,
				  unsigned short br,
				  unsigned short bg,
				  unsigned short bb,
				  ScreenPtr pScreen);

/*
 * qvisGC.c
 */

extern void qvisValidateWindowGC(
				    GCPtr pGC,
				    Mask changes,
				    DrawablePtr pDraw);

/*
 * qvisImage.c
 */
extern void qvisReadImage(
			     BoxPtr pbox,
			     void *imagearg,
			     unsigned int stride,
			     DrawablePtr pDraw);

extern void qvisReadBankedImage(
				   BoxPtr pbox,
				   void *imagearg,
				   unsigned int stride,
				   DrawablePtr pDraw);

extern void qvisDrawImage(
			     BoxPtr pbox,
			     void *imagearg,
			     unsigned int stride,
			     unsigned char alu,
			     unsigned long planemask,
			     DrawablePtr pDraw);

extern void qvisDrawBankedImage(
				   BoxPtr pbox,
				   void *imagearg,
				   unsigned int stride,
				   unsigned char alu,
				   unsigned long planemask,
				   DrawablePtr pDraw);

/*
 * qvisInit.c
 */

extern Bool qvisProbe();

extern Bool qvisInit(
		        int index,
		        struct _Screen * pScreen,
		        int argc,
		        char **argv);

extern void							/* S004 */
 qvisCloseScreen(
		    int index,
		    ScreenPtr pScreen);

/*
 * qvisScreen.c
 */

extern void qvisSaveDACState(
   ScreenPtr pScreen);

extern void qvisRestoreDACState(
   ScreenPtr pScreen);

extern Bool
 qvisBlankScreen(
		    int on,
		    ScreenPtr pScreen);

extern void qvisSetGraphics(
			       ScreenPtr pScreen);

extern void qvisSetText(
			   ScreenPtr pScreen);

extern void qvisSaveGState(
			      ScreenPtr pScreen);

extern void qvisInvalidateShadows(
				     qvisPrivateData * qvisPriv);

extern void qvisRestoreGState(
				 ScreenPtr pScreen);

/*
 * qvisWin.c
 */

extern void qvisValidateWindowPriv(
				      struct _Window * pWin);

/*
 * qvisRectOps.c
 */

extern void qvisCopyRect(
			    BoxPtr pdstBox,
			    DDXPointPtr psrc,
			    unsigned char alu,
			    unsigned long planemask,
			    DrawablePtr pDrawable);


extern void qvisDrawMonoImage(
				 BoxPtr pbox,
				 void *image,			/* S004 */
				 unsigned int startx,
				 unsigned int stride,
				 unsigned long fg,
				 unsigned char alu,
				 unsigned long planemask,
				 DrawablePtr pDrawable);

extern void qvisDrawOpaqueMonoImage(
				       BoxPtr pbox,
				       void *image,		/* S004 */
				       unsigned int startx,
				       unsigned int stride,
				       unsigned long fg,
				       unsigned long bg,
				       unsigned char alu,
				       unsigned long planemask,
				       DrawablePtr pDrawable);

extern void qvisDrawPoints(
			      DDXPointPtr ppt,
			      unsigned int npts,
			      unsigned long fg,
			      unsigned char alu,
			      unsigned long planemask,
			      DrawablePtr pDrawable);

extern void qvisDrawBankedPoints(				/* S000 */
			      DDXPointPtr ppt,
			      unsigned int npts,
			      unsigned long fg,
			      unsigned char alu,
			      unsigned long planemask,
			      DrawablePtr pDrawable);


extern void qvisDrawSolidRects(
				  BoxPtr pBox,
				  unsigned int nBox,
				  unsigned long color,
				  unsigned char rop,
				  unsigned long planemask,
				  DrawablePtr pDrawable);


/*
 * qvisBres.c
 */

extern void qvisSolidZeroSeg(
			        GCPtr gc,
			        DrawablePtr pDraw,
			        int signdx,
			        int signdy,
			        int axis,
			        int x,
			        int y,
			        int e,
			        int e1,
			        int e2,
			        int len);

/*
 * qvisGlyph.c
 */

extern void qvisUncachedDrawMonoGlyphs(
					  nfbGlyphInfo * glyph_info,
					  unsigned int nglyphs,
					  unsigned long fg,
					  unsigned char alu,
					  unsigned long planemask,
					  unsigned long font_private,
					  DrawablePtr pDrawable);

#ifdef TEST_CASE

extern void qvisSSBlitDrawMonoGlyphs(
				        nfbGlyphInfo * glyph_info,
				        unsigned int nglyphs,
				        unsigned long fg,
				        unsigned char alu,
				        unsigned long planemask,
				        unsigned long font_private,
				        DrawablePtr pDrawable);

#endif				/* TEST_CASE */

/*
 * qvisPolyPnt.c
 */

extern void qvisPolyPoint(
			     DrawablePtr pDraw,
			     GCPtr pGC,
			     int mode,
			     int npt,
			     xPoint * ppt);

/*
 * qvisSpans.c
 */

extern void qvisSolidFillSpans(
				  GCPtr pGC,
				  DrawablePtr pDraw,
				  DDXPointPtr ppt,
				  unsigned int *pwidth,
				  unsigned int n);

/*
 * qvisGlCache.c
 */

extern Bool
 qvisInitGlyphCache(
		       ScreenPtr pScreen);

extern void qvisFreeGlyphCache(
				  ScreenPtr pScreen);

extern void qvisInvalidateGlyphCache(
				        qvisPrivateData * qvisPriv);

extern void qvisCachedDrawMonoGlyphs(
				        nfbGlyphInfo * glyph_info,
				        unsigned int nglyphs,
				        unsigned long fg,
				        unsigned char alu,
				        unsigned long planemask,
				        unsigned long font_private,
				        DrawablePtr pDrawable);

extern void qvisGlyphCacheClearFont(
#ifdef agaII
				       unsigned long font_id,
#else
				       struct _nfbFontPS * font_id, /* S004 */
#endif
				       ScreenPtr pScreen);

/*
 * qvisHelpGC.c
 */

extern void
 qvisGCOpsCacheInit();

extern void
 qvisGCOpsCacheReset();

/*
 * qvisAssem.s
 */

extern void qvisSquirt(
			  unsigned int fb_ptr,
			  short w,
			  short h,
			  short x,
			  short y);

extern void qvisZip(
		       short w,
		       short h,
		       short x,
		       short y);

extern void qvisSpit(
		       short x,
		       short y,
		       short x1,
		       short y1);

#endif				/* _QVIS_PROCS_H */
