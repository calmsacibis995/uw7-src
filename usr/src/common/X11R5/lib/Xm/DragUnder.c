#pragma ident	"@(#)m1.2libs:Xm/DragUnder.c	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/DrawP.h>
#include <Xm/DropSMgr.h>
#include "DragCI.h"
#include "DragICCI.h"
#include "DragUnderI.h"
#include "MessagesI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MESSAGE1	catgets(Xm_catd,MS_DragUnder,MSG_DU_1,_XmMsgDragUnder_0000)
#define MESSAGE2	catgets(Xm_catd,MS_DragUnder,MSG_DU_1,_XmMsgDragUnder_0001)
#else
#define MESSAGE1	_XmMsgDragUnder_0000
#define MESSAGE2	_XmMsgDragUnder_0001
#endif


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XmAnimationSaveData CreateAnimationSaveData() ;
static void FreeAnimationData() ;
static Boolean SaveAll() ;
static Boolean SaveSegments() ;
static void DrawHighlight() ;
static void DrawShadow() ;
static void DrawPixmap() ;
static void AnimateEnter() ;
static void AnimateLeave() ;

#else

static XmAnimationSaveData CreateAnimationSaveData( 
                        XmDragContext dc,
                        XmAnimationData aData,
                        XmDragProcCallbackStruct *dpcb) ;
static void FreeAnimationData( 
                        XmAnimationSaveData aSaveData) ;
static Boolean SaveAll( 
                        XmAnimationSaveData aSaveData,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height) ;
#endif /* NeedWidePrototypes */
static Boolean SaveSegments( 
                        XmAnimationSaveData aSaveData,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
#endif /* NeedWidePrototypes */
                        Dimension *thickness) ;
static void DrawHighlight( 
                        XmAnimationSaveData aSaveData) ;
static void DrawShadow( 
                        XmAnimationSaveData aSaveData) ;
static void DrawPixmap( 
                        XmAnimationSaveData aSaveData) ;
static void AnimateEnter( 
                        XmDropSiteManagerObject dsm,
                        XmAnimationData aData,
                        XmDragProcCallbackStruct *dpcb) ;
static void AnimateLeave( 
                        XmDropSiteManagerObject dsm,
                        XmAnimationData aData,
                        XmDragProcCallbackStruct *dpcb) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*****************************************************************************
 *
 *  CreateAnimationSaveData ()
 *
 *  Create and fill an XmAnimationSaveData structure containing the data
 *  needed to animate the dropsite.
 ***************************************************************************/

static XmAnimationSaveData 
#ifdef _NO_PROTO
CreateAnimationSaveData( dc, aData, dpcb )
        XmDragContext dc ;
        XmAnimationData aData ;
        XmDragProcCallbackStruct *dpcb ;
#else
CreateAnimationSaveData(
        XmDragContext dc,
        XmAnimationData aData,
        XmDragProcCallbackStruct *dpcb )
#endif /* _NO_PROTO */
{
    XmAnimationSaveData		aSaveData;
    XGCValues			v;
    unsigned long		vmask;
    XmDropSiteVisuals		dsv;
    int				ac;
    Arg				al[5];
    Window			junkWin;
    int				junkInt;
    unsigned int		junkUInt;

    aSaveData = (XmAnimationSaveData)
	XtMalloc (sizeof (XmAnimationSaveDataRec));

    aSaveData->dragOver = aData->dragOver;
    aSaveData->display = XtDisplay (dc);
    aSaveData->xmScreen = (XmScreen) XmGetXmScreen (aData->screen);

    aSaveData->window = aData->window;
    aSaveData->windowX = aData->windowX;
    aSaveData->windowY = aData->windowY;

    if (aSaveData->dragOver) {
        aSaveData->xmScreen = (XmScreen) XmGetXmScreen (XtScreen (aSaveData->dragOver));
    }
    else {
        aSaveData->xmScreen = (XmScreen) XmGetXmScreen(XtScreen (dc));
    }

    /*
     *  Get the window depth.
     */

    if (!XGetGeometry (aSaveData->display, aSaveData->window, 
		       &junkWin, &junkInt, &junkInt,
		       &junkUInt, &junkUInt, &junkUInt,
                       &(aSaveData->windowDepth))) {
	_XmWarning ((Widget) dc, MESSAGE1);
        aSaveData->windowDepth = 0;
    }

    aSaveData->clipRegion = aData->clipRegion;
    aSaveData->dropSiteRegion = aData->dropSiteRegion;

    dsv = XmDropSiteGetActiveVisuals ((Widget) dc);
    aSaveData->background = dsv->background;
    aSaveData->foreground = dsv->foreground;
    aSaveData->topShadowColor = dsv->topShadowColor;
    aSaveData->topShadowPixmap = dsv->topShadowPixmap;
    aSaveData->bottomShadowColor = dsv->bottomShadowColor;
    aSaveData->bottomShadowPixmap = dsv->bottomShadowPixmap;
    aSaveData->shadowThickness = dsv->shadowThickness;
    aSaveData->highlightThickness = dsv->highlightThickness;
    aSaveData->highlightColor = dsv->highlightColor;
    aSaveData->highlightPixmap = dsv->highlightPixmap;
    aSaveData->borderWidth = dsv->borderWidth;
    XtFree ((char *)dsv);

    ac = 0;
    XtSetArg (al[ac], XmNanimationStyle, &(aSaveData->animationStyle)); ac++;
    XtSetArg (al[ac], XmNanimationMask, &(aSaveData->animationMask)); ac++;
    XtSetArg (al[ac], XmNanimationPixmap, &(aSaveData->animationPixmap)); ac++;
    XtSetArg (al[ac], XmNanimationPixmapDepth,
	      &(aSaveData->animationPixmapDepth)); ac++;
    XmDropSiteRetrieve ((Widget) dc, al, ac);

    if (aSaveData->animationStyle == XmDRAG_UNDER_PIXMAP &&
	aSaveData->animationPixmap != None &&
        aSaveData->animationPixmap != XmUNSPECIFIED_PIXMAP &&
        aSaveData->animationPixmapDepth != 1 &&
        aSaveData->animationPixmapDepth != aSaveData->windowDepth) {

	_XmWarning ((Widget) dc, MESSAGE2);
        aSaveData->animationPixmap = XmUNSPECIFIED_PIXMAP;
    }

    /*
     *  Create the draw GC.
     */

    v.foreground = aSaveData->foreground;
    v.background = aSaveData->background;
    v.graphics_exposures = False;
    v.subwindow_mode = IncludeInferiors;
    vmask = GCGraphicsExposures|GCSubwindowMode|GCForeground|GCBackground;
    aSaveData->drawGC =
	XCreateGC (aSaveData->display, aSaveData->window, vmask, &v);

    /* initialize savedPixmaps list */

    aSaveData->savedPixmaps = NULL;
    aSaveData->numSavedPixmaps = 0;

    return (aSaveData);
}

/*****************************************************************************
 *
 *  FreeAnimationData ()
 *
 *  Free an XmAnimationSaveData structure.
 ***************************************************************************/

static void 
#ifdef _NO_PROTO
FreeAnimationData( aSaveData )
        XmAnimationSaveData aSaveData ;
#else
FreeAnimationData(
        XmAnimationSaveData aSaveData )
#endif /* _NO_PROTO */
{
    Cardinal	i;

    switch (aSaveData->animationStyle)
    {
        case XmDRAG_UNDER_SHADOW_IN:
        case XmDRAG_UNDER_SHADOW_OUT:
            XFreeGC (aSaveData->display, aSaveData->topShadowGC);
            XFreeGC (aSaveData->display, aSaveData->bottomShadowGC);
            XFreeGC (aSaveData->display, aSaveData->drawGC);
        break;

        case XmDRAG_UNDER_HIGHLIGHT:
            XFreeGC (aSaveData->display, aSaveData->highlightGC);
            XFreeGC (aSaveData->display, aSaveData->drawGC);
        break;

        case XmDRAG_UNDER_PIXMAP:
            XFreeGC (aSaveData->display, aSaveData->drawGC);

        case XmDRAG_UNDER_NONE:
        default:
        break;
    }

    if (aSaveData->numSavedPixmaps) {
        for (i = 0; i < aSaveData->numSavedPixmaps; i++) {
	    _XmFreeScratchPixmap (aSaveData->xmScreen,
				  aSaveData->savedPixmaps[i].pixmap);
        }
        XtFree ((char *)aSaveData->savedPixmaps);
    }

    XtFree ((char *)aSaveData);
}

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif /* min */

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif /* max */

/*****************************************************************************
 *
 *  SaveAll ()
 *
 *  Save the original contents of a dropsite window that will be overwritten
 *  by dropsite animation into a rectangular backing store.
 ***************************************************************************/

static Boolean 
#ifdef _NO_PROTO
SaveAll( aSaveData, x, y, width, height )
        XmAnimationSaveData aSaveData ;
        Position x ;
        Position y ;
        Dimension width ;
        Dimension height ;
#else
SaveAll(
        XmAnimationSaveData aSaveData,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        int height )
#else
        Position x,
        Position y,
        Dimension width,
        Dimension height )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    PixmapData		*pData;

    if (width <= 0 || height <= 0) {
	return (False);
    }

    aSaveData->numSavedPixmaps = 1;
    aSaveData->savedPixmaps = pData =
        (PixmapData *) XtMalloc (sizeof(PixmapData));
    if (!pData) {
	return (False);
    }

    pData->x = x;
    pData->y = y;
    pData->width = width;
    pData->height = height;
    pData->pixmap =
	_XmAllocScratchPixmap (aSaveData->xmScreen,
			       (Cardinal) aSaveData->windowDepth,
		               pData->width, pData->height);
    XCopyArea (aSaveData->display, aSaveData->window,
    	       pData->pixmap, aSaveData->drawGC,
               pData->x, pData->y,
	       pData->width, pData->height, 0, 0);

    return (True);
}

/*****************************************************************************
 *
 *  SaveSegments ()
 *
 *  Save the original contents of a dropsite window that will be overwritten
 *  by dropsite highlighting or shadowing of indicated thickness.  This will
 *  save 0, 1, or 4 strips into backing store, depending on the dimensions
 *  of the dropsite and the animation thickness.
 ***************************************************************************/

static Boolean 
#ifdef _NO_PROTO
SaveSegments( aSaveData, x, y, width, height, thickness )
        XmAnimationSaveData aSaveData ;
        Position x ;
        Position y ;
        Dimension width ;
        Dimension height ;
        Dimension *thickness ;
#else
SaveSegments(
        XmAnimationSaveData aSaveData,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        int height,
#else
        Position x,
        Position y,
        Dimension width,
        Dimension height,
#endif /* NeedWidePrototypes */
        Dimension *thickness )
#endif /* _NO_PROTO */
{
    PixmapData		*pData;
    Boolean		save_all = False;

    if (width <= 0 || height <= 0 || *thickness <= 0) {
        return (False);
    }
    if (*thickness > (width >> 1)) {
        *thickness = (width >> 1);
        save_all = True;
    }
    if (*thickness > (height >> 1)) {
        *thickness = (height >> 1);
        save_all = True;
    }

    if (save_all) {
        return (SaveAll (aSaveData, x, y, width, height));
    }

    aSaveData->numSavedPixmaps = 4;
    aSaveData->savedPixmaps = pData =
	    (PixmapData *) XtMalloc (sizeof(PixmapData) * 4);
    if (!pData) {
	    return (False);
    }

    pData->x = x;
    pData->y = y;
    pData->width = width;
    pData->height = *thickness;
    pData->pixmap =
	_XmAllocScratchPixmap (aSaveData->xmScreen,
			       (Cardinal) aSaveData->windowDepth,
		               pData->width, pData->height);
    XCopyArea (aSaveData->display, aSaveData->window,
    	       pData->pixmap, aSaveData->drawGC,
               pData->x, pData->y,
	       pData->width, pData->height, 0, 0);

    pData++;
    pData->x = x;
    pData->y = y + *thickness;
    pData->width = *thickness;
    pData->height = height - (*thickness << 1);
    pData->pixmap =
	_XmAllocScratchPixmap (aSaveData->xmScreen,
			       (Cardinal) aSaveData->windowDepth,
		               pData->width, pData->height);
    XCopyArea (aSaveData->display, aSaveData->window,
    	       pData->pixmap, aSaveData->drawGC,
               pData->x, pData->y,
	       pData->width, pData->height, 0, 0);

    pData++;
    pData->x = x;
    pData->y = y + height - *thickness;
    pData->width = width;
    pData->height = *thickness;
    pData->pixmap =
	_XmAllocScratchPixmap (aSaveData->xmScreen,
			       (Cardinal) aSaveData->windowDepth,
		               pData->width, pData->height);
    XCopyArea (aSaveData->display, aSaveData->window,
    	       pData->pixmap, aSaveData->drawGC,
               pData->x, pData->y,
	       pData->width, pData->height, 0, 0);

    pData++;
    pData->x = x + width - *thickness;
    pData->y = y + *thickness;
    pData->width = *thickness;
    pData->height = height - (*thickness << 1);
    pData->pixmap =
	_XmAllocScratchPixmap (aSaveData->xmScreen,
			       (Cardinal) aSaveData->windowDepth,
		               pData->width, pData->height);
    XCopyArea (aSaveData->display, aSaveData->window,
    	       pData->pixmap, aSaveData->drawGC,
               pData->x, pData->y,
	       pData->width, pData->height, 0, 0);

    return (True);
}

/*****************************************************************************
 *
 *  DrawHighlight ()
 *
 *  Draws a highlight around the indicated region.
 ***************************************************************************/

static void 
#ifdef _NO_PROTO
DrawHighlight( aSaveData )
        XmAnimationSaveData aSaveData ;
#else
DrawHighlight(
        XmAnimationSaveData aSaveData )
#endif /* _NO_PROTO */
{
    XGCValues		v;
    unsigned long	vmask;
    Dimension		offset;
    Position		x;
    Position		y;
    Dimension		width;
    Dimension		height;
    XRectangle		extents;

    /*
     *  Create the highlightGC
     */

    v.foreground = aSaveData->highlightColor;
    v.background = aSaveData->background;
    v.graphics_exposures = False;
    v.subwindow_mode = IncludeInferiors;
    vmask = GCGraphicsExposures|GCSubwindowMode|GCForeground|GCBackground;

    if (aSaveData->highlightPixmap != None &&
	aSaveData->highlightPixmap != XmUNSPECIFIED_PIXMAP) {
	v.fill_style = FillTiled;
	v.tile = aSaveData->highlightPixmap;
	vmask |= GCTile | GCFillStyle;
    }

    aSaveData->highlightGC =
	XCreateGC(aSaveData->display, aSaveData->window, vmask, &v);

    _XmRegionSetGCRegion (aSaveData->display, aSaveData->highlightGC,
			  0, 0, aSaveData->clipRegion);

    /* draw highlight */

    _XmRegionGetExtents (aSaveData->dropSiteRegion, &extents);
    offset = aSaveData->borderWidth;

    if (_XmRegionGetNumRectangles(aSaveData->dropSiteRegion) == 1L) {

        x = extents.x + offset;
        y = extents.y + offset;
        width = extents.width - (offset << 1);
        height = extents.height - (offset << 1);

        if (SaveSegments (aSaveData, x, y, width, height,
                          &aSaveData->highlightThickness)) {
            _XmDrawSimpleHighlight (aSaveData->display, aSaveData->window,
				    aSaveData->highlightGC,
				    x, y, width, height,
				    aSaveData->highlightThickness);
        }
    }
    else {
        if (SaveAll (aSaveData, extents.x, extents.y, extents.width,
		     extents.height)) {
            _XmRegionDrawShadow (aSaveData->display, aSaveData->window,
		                 aSaveData->highlightGC, aSaveData->highlightGC,
                                 aSaveData->dropSiteRegion,
                                 offset, aSaveData->highlightThickness,
				 XmSHADOW_OUT);
	}
    }
}

/*****************************************************************************
 *
 *  DrawShadow ()
 *
 *  Draws a 3-D shadow around the indicated region.
 ***************************************************************************/

static void 
#ifdef _NO_PROTO
DrawShadow( aSaveData )
        XmAnimationSaveData aSaveData ;
#else
DrawShadow(
        XmAnimationSaveData aSaveData )
#endif /* _NO_PROTO */
{
    XGCValues		v;
    unsigned long	vmask;
    Dimension		offset;
    Position		x;
    Position		y;
    Dimension		width;
    Dimension		height;
    XRectangle		extents;

    /*
     *  Create the topShadowGC
     */

    v.foreground = aSaveData->topShadowColor;
    v.background = aSaveData->foreground;
    v.graphics_exposures = False;
    v.subwindow_mode = IncludeInferiors;
    vmask = GCGraphicsExposures|GCSubwindowMode|GCForeground|GCBackground;

    if (aSaveData->topShadowPixmap != None &&
        aSaveData->topShadowPixmap != XmUNSPECIFIED_PIXMAP) {
	v.fill_style = FillTiled;
	v.tile = aSaveData->topShadowPixmap;
	vmask |= GCTile | GCFillStyle;
    }

    aSaveData->topShadowGC =
	XCreateGC(aSaveData->display, aSaveData->window, vmask, &v);

    _XmRegionSetGCRegion (aSaveData->display, aSaveData->topShadowGC,
			  0, 0, aSaveData->clipRegion);

    /*
     *  Create the bottomShadowGC
     */

    v.foreground = aSaveData->bottomShadowColor;
    v.background = aSaveData->foreground;
    v.graphics_exposures = False;
    v.subwindow_mode = IncludeInferiors;
    vmask = GCGraphicsExposures|GCSubwindowMode|GCForeground|GCBackground;

    if (aSaveData->bottomShadowPixmap != None &&
        aSaveData->bottomShadowPixmap != XmUNSPECIFIED_PIXMAP) {
	v.fill_style = FillTiled;
	v.tile = aSaveData->bottomShadowPixmap;
	vmask |= GCTile | GCFillStyle;
    }

    aSaveData->bottomShadowGC =
	XCreateGC(aSaveData->display, aSaveData->window, vmask, &v);

    _XmRegionSetGCRegion (aSaveData->display, aSaveData->bottomShadowGC,
			  0, 0, aSaveData->clipRegion);

    /*
     *  Draw the shadows.
     */

    _XmRegionGetExtents (aSaveData->dropSiteRegion, &extents);
    offset = aSaveData->borderWidth + aSaveData->highlightThickness;

    if (_XmRegionGetNumRectangles(aSaveData->dropSiteRegion) == 1L) {

        x = extents.x + offset;
        y = extents.y + offset;
        width = extents.width - (offset << 1);
        height = extents.height - (offset << 1);

        if (SaveSegments (aSaveData, x, y, width, height,
                          &aSaveData->shadowThickness)) {
            _XmDrawShadows (aSaveData->display, aSaveData->window,
		             aSaveData->topShadowGC,
                             aSaveData->bottomShadowGC,
                             x, y, width, height,
		             aSaveData->shadowThickness,
                             (aSaveData->animationStyle ==
				 XmDRAG_UNDER_SHADOW_IN) ?
		                     XmSHADOW_IN : XmSHADOW_OUT);
        }
    }
    else {
        if (SaveAll (aSaveData, extents.x, extents.y,
		     extents.width, extents.height)) {
            _XmRegionDrawShadow (aSaveData->display, aSaveData->window,
		                 aSaveData->topShadowGC,
				 aSaveData->bottomShadowGC,
                                 aSaveData->dropSiteRegion,
		                 offset, aSaveData->shadowThickness,
                                 (aSaveData->animationStyle ==
				     XmDRAG_UNDER_SHADOW_IN) ?
		                         XmSHADOW_IN : XmSHADOW_OUT);
	}
    }
}

/*****************************************************************************
 *
 *  DrawPixmap ()
 *
 *  Copy an animationPixmap, possibly masked, to the dropsite window.
 ***************************************************************************/

static void 
#ifdef _NO_PROTO
DrawPixmap( aSaveData )
        XmAnimationSaveData aSaveData ;
#else
DrawPixmap(
        XmAnimationSaveData aSaveData )
#endif /* _NO_PROTO */
{
    Position		x;
    Position		y;
    Dimension		width;
    Dimension		height;
    XRectangle		extents;
    XGCValues		v;
    unsigned long       vmask;
    Pixmap		mask = XmUNSPECIFIED_PIXMAP;
    GC			maskGC = NULL;

    if (aSaveData->animationPixmap == None ||
        aSaveData->animationPixmap == XmUNSPECIFIED_PIXMAP) {
	return;
    }

    /*
     *  Determine the destination location and dimensions -- the
     *  dropsite's bounding box.
     */

    _XmRegionGetExtents (aSaveData->dropSiteRegion, &extents);
    x = extents.x;
    y = extents.y;
    width = extents.width;
    height = extents.height;

    /*
     *  Save the original window contents.
     *  Draw the DrawUnder pixmap into the window.
     *  Assume correct depth -- checked in CreateAnimationSaveData().
     */

    if (SaveAll (aSaveData, x, y, width, height)) {

	if (aSaveData->animationMask != None && 
	    aSaveData->animationMask != XmUNSPECIFIED_PIXMAP) {

	    /*
	     *  AnimationMask specified:  create a composite mask consisting
	     *  of both the clipping region and the animationMask to use for
	     *  copying the animationPixmap into the dropSite.
	     *
	     *    Create a mask and maskGC.
	     *    Set the composite mask to 0's.
	     *    Or the animationMask into it through the ClipRegion.
	     *    Set the drawGC's ClipMask to the composite mask.
	     */

            mask = _XmAllocScratchPixmap (aSaveData->xmScreen, 1,
					  width, height);

	    v.background = 0;
	    v.foreground = 1;
	    v.function = GXclear;
	    v.graphics_exposures = False;
	    v.subwindow_mode = IncludeInferiors;
	    vmask = GCGraphicsExposures|GCSubwindowMode|
	            GCBackground|GCForeground|GCFunction;
	    maskGC = XCreateGC (aSaveData->display, mask, vmask, &v);

	    XFillRectangle (aSaveData->display, mask, maskGC,
		            0, 0, width, height);

	    XSetFunction (aSaveData->display, maskGC, GXor);
	    _XmRegionSetGCRegion (aSaveData->display, maskGC,
				  -x, -y, aSaveData->clipRegion);
	    XCopyArea (aSaveData->display,
		       aSaveData->animationMask,
    	               mask, maskGC,
                       0, 0, width, height, 0, 0);

	    XSetClipOrigin (aSaveData->display, aSaveData->drawGC, x, y);
	    XSetClipMask (aSaveData->display, aSaveData->drawGC, mask);

	    XFreeGC (aSaveData->display, maskGC);
	}
	else {
	    _XmRegionSetGCRegion (aSaveData->display, aSaveData->drawGC,
				  0, 0, aSaveData->clipRegion);
	}

	/*
	 *  Copy the animationPixmap to the window.
	 *  If the animationPixmapDepth is 1 we treat the animationPixmap
	 *  as a bitmap and use XCopyPlane.  For 1-deep dropsite windows,
	 *  this may not be the same as treating the animationPixmap as a
	 *  1-deep pixmap and using XCopyArea.
	 */

	if (aSaveData->animationPixmapDepth == 1) {
	    XCopyPlane (aSaveData->display,
			aSaveData->animationPixmap,
    	                aSaveData->window, aSaveData->drawGC,
                        0, 0, width, height, x, y, 1L);
	}
	else {
	    XCopyArea (aSaveData->display,
		       aSaveData->animationPixmap,
    	               aSaveData->window, aSaveData->drawGC,
                       0, 0, width, height, x, y);
	}
	if (mask != XmUNSPECIFIED_PIXMAP) {
	    _XmFreeScratchPixmap (aSaveData->xmScreen, mask);
	}
    }
}

/*****************************************************************************
 *
 *  AnimateEnter ()
 *
 ***************************************************************************/

static void 
#ifdef _NO_PROTO
AnimateEnter( dsm, aData, dpcb )
        XmDropSiteManagerObject dsm ;
        XmAnimationData aData ;
        XmDragProcCallbackStruct *dpcb ;
#else
AnimateEnter(
        XmDropSiteManagerObject dsm,
        XmAnimationData aData,
        XmDragProcCallbackStruct *dpcb )
#endif /* _NO_PROTO */
{
    Widget dc = dpcb->dragContext;
    XmAnimationSaveData	aSaveData;

    /*
     *  Create and fill an XmAnimationSaveData structure containing the
     *  data needed to animate the dropsite.  Save it for AnimateLeave().
     */

    aSaveData = CreateAnimationSaveData ((XmDragContext) dc, aData, dpcb);
    *((XtPointer *) aData->saveAddr) = (XtPointer) aSaveData;

    /*
     *  If dragging a pixmap or window, hide it while drawing the
     *  animation.
     */

    if (aSaveData->dragOver) {
	_XmDragOverHide (aSaveData->dragOver,
			 aSaveData->windowX, aSaveData->windowY,
			 aSaveData->clipRegion);
    }

    /* Draw the visuals. */

    switch(aSaveData->animationStyle)
    {
	default:
	case XmDRAG_UNDER_HIGHLIGHT:
	    DrawHighlight (aSaveData);
	break;

	case XmDRAG_UNDER_SHADOW_IN:
	case XmDRAG_UNDER_SHADOW_OUT:
	    DrawShadow (aSaveData);
	break;

	case XmDRAG_UNDER_PIXMAP:
	    DrawPixmap (aSaveData);
	break;

	case XmDRAG_UNDER_NONE:
	break;
    }

    /*
     *  If dragging a pixmap or window, show it.
     */

    if (aSaveData->dragOver) {
	_XmDragOverShow (aSaveData->dragOver,
			 aSaveData->windowX, aSaveData->windowY,
			 aSaveData->clipRegion);
    }
}

/*****************************************************************************
 *
 *  AnimateLeave ()
 *
 ***************************************************************************/

static void 
#ifdef _NO_PROTO
AnimateLeave( dsm, aData, dpcb )
        XmDropSiteManagerObject dsm ;
        XmAnimationData aData ;
        XmDragProcCallbackStruct *dpcb ;
#else
AnimateLeave(
        XmDropSiteManagerObject dsm,
        XmAnimationData aData,
        XmDragProcCallbackStruct *dpcb )
#endif /* _NO_PROTO */
{
    XmAnimationSaveData aSaveData =
	(XmAnimationSaveData) *((XtPointer *) aData->saveAddr);
	
    if (aSaveData) {
        Cardinal	i;
        PixmapData	*pData;

	/*
	 *  If dragging a pixmap or window, hide it while erasing the
	 *  animation.
	 */

        if (aSaveData->dragOver) {
	    _XmDragOverHide (aSaveData->dragOver,
    			     aSaveData->windowX, aSaveData->windowY,
			     aSaveData->clipRegion);
	}

	/*
	 *  Copy any saved segments back into the window.
	 *  Be sure GCRegion is set properly here.
	 */

        _XmRegionSetGCRegion (aSaveData->display, aSaveData->drawGC,
			      0, 0, aSaveData->clipRegion);
        for (pData = aSaveData->savedPixmaps, i = aSaveData->numSavedPixmaps;
	     i; pData++, i--) {
            XCopyArea (aSaveData->display,
		       pData->pixmap,
		       aSaveData->window,
		       aSaveData->drawGC,
                       0, 0,
		       pData->width,
		       pData->height, 
		       pData->x,
		       pData->y);
        }

	/*
	 *  If dragging a pixmap or window, show it.
         *  Free the XmAnimationSaveData structure created in AnimateEnter().
	 */

        if (aSaveData->dragOver) {
	    _XmDragOverShow (aSaveData->dragOver,
    			     aSaveData->windowX, aSaveData->windowY,
			     aSaveData->clipRegion);
	}

        FreeAnimationData (aSaveData);
	*((XtPointer *) aData->saveAddr) = (XtPointer) NULL;
    }
}

/*****************************************************************************
 *
 *  _XmDragUnderAnimation ()
 *
 ***************************************************************************/

void 
#ifdef _NO_PROTO
_XmDragUnderAnimation( w, clientData, callData )
    Widget w ;
    XtPointer clientData ;
    XtPointer callData ;
#else
_XmDragUnderAnimation(
    Widget w,
    XtPointer clientData,
    XtPointer callData )
#endif /* _NO_PROTO */
{
    XmDropSiteManagerObject dsm = (XmDropSiteManagerObject) w;
    XmDragProcCallbackStruct *dpcb = (XmDragProcCallbackStruct *) callData;
    XmAnimationData aData = (XmAnimationData) clientData;

    switch(dpcb->reason)
    {
        case XmCR_DROP_SITE_LEAVE_MESSAGE:
            AnimateLeave(dsm, aData, dpcb);
        break;

        case XmCR_DROP_SITE_ENTER_MESSAGE:
            AnimateEnter(dsm, aData, dpcb);
        break;

        default:
        break;
    }
}

