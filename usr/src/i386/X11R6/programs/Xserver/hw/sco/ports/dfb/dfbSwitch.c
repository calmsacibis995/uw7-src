/*
 * @(#) dfbSwitch.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 * dfbSwitch.c
 *
 * dfb SwitchScreen routines.
 */

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "font.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "dfbProcs.h"
#include "dfbSwitch.h"


#define	NAME(subname) CATNAME(dfbSS,subname)

#if (__STDC__ && !defined(UNIXCPP)) || defined(ANSICPP)
#define CATNAME(prefix,subname) prefix##subname
#else
#define CATNAME(prefix,subname) prefix/**/subname
#endif


int dfbScreenNum = -1;
int dfbSSGeneration = 0;
int dfbSSScrnPrivIndex;
int dfbSSGCPrivIndex;


/*
 * screen wrappers
 */

static Bool		NAME(CloseScreen)();
static Bool		NAME(SaveScreen)();
static void		NAME(GetImage)();
static void		NAME(GetSpans)();
static void		NAME(SourceValidate)();
static void		NAME(PaintWindowBackground)();
static void		NAME(PaintWindowBorder)();
static void		NAME(CopyWindow)();
static void		NAME(ClearToBackground)();
static void		NAME(SaveDoomedAreas)();
static RegionPtr	NAME(RestoreAreas)();
static Bool		NAME(CreateGC)();
static void		NAME(InstallColormap)();
static void		NAME(StoreColors)();

#define	SCREEN_SETUP(pScr, pPriv)	\
	dfbSSScrnPrivPtr (pPriv) = DFB_SS_SCREEN_PRIV(pScr)

#define SCREEN_WRAP(pScr, pPriv, field) {	\
	(pPriv)->field = (pScr)->field;		\
	(pScr)->field  = NAME(field);		\
}

#define SCREEN_UNWRAP(pScr, pPriv, field) (pScr)->field = (pPriv)->field
#define SCREEN_TYPED_REWRAP(pScr, field, type) (pScr)->field = (type(*)())NAME(field)
#define SCREEN_REWRAP(pScr, field) SCREEN_TYPED_REWRAP(pScr, field, void)

/*
 * scoScreenInfo "wrappers"
 */

static void	NAME(SetGraphics)();
static void	NAME(SetText)();
static void	NAME(SaveGState)();
static void	NAME(RestoreGState)();


/*
 * GC func wrappers
 */

static void	NAME(ValidateGC)(),	NAME(CopyGC)();
static void	NAME(DestroyGC)(),	NAME(ChangeGC)();
static void	NAME(ChangeClip)(),	NAME(DestroyClip)();
static void	NAME(CopyClip)();

static GCFuncs dfbSSGCFuncs = {
	NAME(ValidateGC),
	NAME(ChangeGC),
	NAME(CopyGC),
	NAME(DestroyGC),
	NAME(ChangeClip),
	NAME(DestroyClip),
	NAME(CopyClip),
};

#define GC_FUNC_SETUP(pGC, pGCPriv)			\
	dfbSSGCPrivPtr (pGCPriv) = DFB_SS_GC_PRIV(pGC)

#define GC_FUNC_UNWRAP(pGC, pGCPriv) {			\
	(pGC)->funcs = (pGCPriv)->wrapFuncs;		\
	if ((pGCPriv)->wrapOps)				\
		(pGC)->ops = (pGCPriv)->wrapOps;	\
}

#define GC_FUNC_REWRAP(pGC, pGCPriv) {			\
	(pGCPriv)->wrapFuncs = (pGC)->funcs;		\
	(pGC)->funcs = &dfbSSGCFuncs;			\
	if ((pGCPriv)->wrapOps) {			\
		(pGCPriv)->wrapOps = (pGC)->ops;	\
		(pGC)->ops = &dfbSSGCOps;		\
	}						\
}


/*
 * GC op wrappers
 */

static void		NAME(FillSpans)(),	NAME(SetSpans)();
static void		NAME(PutImage)();
static RegionPtr	NAME(CopyArea)(),	NAME(CopyPlane)();
static void		NAME(PolyPoint)(),	NAME(Polylines)();
static void		NAME(PolySegment)(),	NAME(PolyRectangle)();
static void		NAME(PolyArc)(),	NAME(FillPolygon)();
static void		NAME(PolyFillRect)(),	NAME(PolyFillArc)();
static int		NAME(PolyText8)(),	NAME(PolyText16)();
static void		NAME(ImageText8)(),	NAME(ImageText16)();
static void		NAME(ImageGlyphBlt)(),	NAME(PolyGlyphBlt)();
static void		NAME(PushPixels)();
static void		NAME(ChangeClip)(),	NAME(DestroyClip)();
static void		NAME(CopyClip)();

extern int NAME(NoopDDA);

static GCOps dfbSSGCOps = {
	NAME(FillSpans),
        NAME(SetSpans),
        NAME(PutImage),	
	NAME(CopyArea),
        NAME(CopyPlane),
        NAME(PolyPoint),
	NAME(Polylines),
        NAME(PolySegment),
        NAME(PolyRectangle),
	NAME(PolyArc),
        NAME(FillPolygon),
        NAME(PolyFillRect),
	NAME(PolyFillArc),
        NAME(PolyText8),
        NAME(PolyText16),
	NAME(ImageText8),
        NAME(ImageText16),
        NAME(ImageGlyphBlt),
	NAME(PolyGlyphBlt),
        NAME(PushPixels),
#ifdef NEED_LINEHELPER
        NoopDDA,
#endif
        0,
};

#define	GC_OP_SETUP(pDraw, pGC, pScr, pPriv)			\
	ScreenPtr (pScr) = (pDraw)->pScreen;			\
	dfbSSScrnPrivPtr (pPriv) = DFB_SS_SCREEN_PRIV(pScr);	\
	dfbSSGCPrivPtr pGCPriv = DFB_SS_GC_PRIV(pGC);		\
	GCFuncs *oldFuncs

#define GC_OP_UNWRAP(pGC) {			\
	oldFuncs = (pGC)->funcs;		\
	(pGC)->funcs = pGCPriv->wrapFuncs;	\
	(pGC)->ops   = pGCPriv->wrapOps;	\
}

#define GC_OP_REWRAP(pGC) {			\
	pGCPriv->wrapOps = (pGC)->ops;		\
	(pGC)->funcs = oldFuncs;		\
	(pGC)->ops = &dfbSSGCOps;		\
}


/*
 * prep routines called from Probe and Init
 * to enable access to hardware in those routines.
 */

dfbSSProbePrep(pReq, scrnum)
	ddxScreenRequest *pReq;
	int scrnum;
{
	grafData *pGraf = pReq->grafinfo;
	codeType *switchscr;

	if (grafGetFunction(pGraf, "SwitchScreen", &switchscr))
		grafRunFunction(pGraf, switchscr, NULL, scrnum);
}

dfbSSInitPrep(index, pScr)
	int index;
	ScreenPtr pScr;
{
	grafData *pGraf = DDX_GRAFINFO(pScr);
	codeType *switchscr;

	if (!grafGetFunction(pGraf, "SwitchScreen", &switchscr))
		return;

	if (grafGetFunction(pGraf, "SwitchScreen", &switchscr))
		grafRunFunction(pGraf, switchscr, NULL, index);
}


/*
 * dfbSSInitialize - wrap the screen functions for ScreenSwitching
 */
Bool
dfbSSInitialize(pScr, ppSysInfo)
	ScreenPtr	pScr;
	scoScreenInfo **ppSysInfo;
{
	dfbSSScrnPrivPtr pPriv;
	scoScreenInfo	*pOldSI, *pNewSI;
	grafData	*pGraf;
	codeType	*switchscr;
    
	pGraf = DDX_GRAFINFO(pScr);
	if (!grafGetFunction(pGraf, "SwitchScreen", &switchscr))
		return TRUE;

	if (dfbSSGeneration != serverGeneration) {
		if ((dfbSSScrnPrivIndex = AllocateScreenPrivateIndex()) < 0)
			return FALSE;
		dfbSSGCPrivIndex = AllocateGCPrivateIndex();
		dfbSSGeneration  = serverGeneration;
	}
	if (!(pNewSI = (scoScreenInfo *)xalloc(sizeof(scoScreenInfo))))
		return FALSE;
	if (!(pPriv = (dfbSSScrnPrivPtr)xalloc(sizeof(dfbSSScrnPriv))))
		return FALSE;
	if (!AllocateGCPrivate(pScr, dfbSSGCPrivIndex, sizeof(dfbSSGCPriv)))
		return FALSE;

	pOldSI	   = *ppSysInfo;
	*pNewSI    = *pOldSI;
	*ppSysInfo = pNewSI;

	/* "wrap" scoScreenInfo funcs */
	pNewSI->SetGraphics	= NAME(SetGraphics);
	pNewSI->SetText		= NAME(SetText);
	pNewSI->SaveGState	= NAME(SaveGState);
	pNewSI->RestoreGState	= NAME(RestoreGState);

        pScr->devPrivates[dfbSSScrnPrivIndex].ptr = (unsigned char *)pPriv;

	pPriv->pOldSI		= pOldSI;
	pPriv->pNewSI		= pNewSI;
	pPriv->inGfxMode	= TRUE;
	pPriv->pGraf		= pGraf;
	pPriv->SwitchScrFunc	= switchscr;

	/* wrap screen funcs */
	SCREEN_WRAP(pScr, pPriv, CloseScreen);
	SCREEN_WRAP(pScr, pPriv, SaveScreen);
	SCREEN_WRAP(pScr, pPriv, GetImage);
	SCREEN_WRAP(pScr, pPriv, GetSpans);
	SCREEN_WRAP(pScr, pPriv, SourceValidate);
	SCREEN_WRAP(pScr, pPriv, PaintWindowBackground);
	SCREEN_WRAP(pScr, pPriv, PaintWindowBorder);
	SCREEN_WRAP(pScr, pPriv, CopyWindow);
	SCREEN_WRAP(pScr, pPriv, ClearToBackground);
	SCREEN_WRAP(pScr, pPriv, SaveDoomedAreas);
	SCREEN_WRAP(pScr, pPriv, RestoreAreas);
	SCREEN_WRAP(pScr, pPriv, CreateGC);
	SCREEN_WRAP(pScr, pPriv, InstallColormap);
	SCREEN_WRAP(pScr, pPriv, StoreColors);

	return TRUE;
}


/*
 * Screen wrappers
 */

static Bool
NAME(CloseScreen)(i, pScr)
	int		i;
	ScreenPtr	pScr;
{
	SCREEN_SETUP(pScr, pPriv);

	/* unwrap screen funcs */
	SCREEN_UNWRAP(pScr, pPriv, CloseScreen);
	SCREEN_UNWRAP(pScr, pPriv, SaveScreen);
	SCREEN_UNWRAP(pScr, pPriv, GetImage);
	SCREEN_UNWRAP(pScr, pPriv, GetSpans);
	SCREEN_UNWRAP(pScr, pPriv, SourceValidate);
	SCREEN_UNWRAP(pScr, pPriv, PaintWindowBackground);
	SCREEN_UNWRAP(pScr, pPriv, PaintWindowBorder);
	SCREEN_UNWRAP(pScr, pPriv, CopyWindow);
	SCREEN_UNWRAP(pScr, pPriv, ClearToBackground);
	SCREEN_UNWRAP(pScr, pPriv, SaveDoomedAreas);
	SCREEN_UNWRAP(pScr, pPriv, RestoreAreas);
	SCREEN_UNWRAP(pScr, pPriv, CreateGC);
	SCREEN_UNWRAP(pScr, pPriv, InstallColormap);
	SCREEN_UNWRAP(pScr, pPriv, StoreColors);

	/* free allocated data */
	xfree((pointer)pPriv->pNewSI);
	xfree((pointer)pPriv);

	/* call through */
	return (*pScr->CloseScreen)(i, pScr);
}

static Bool
NAME(SaveScreen)(pScr, on)
	ScreenPtr	pScr;
	Bool		on;
{
	SCREEN_SETUP(pScr, pPriv);
	Bool result;
    
	SCREEN_UNWRAP(pScr, pPriv, SaveScreen);

	if (pPriv->inGfxMode)
		DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	result = (*pScr->SaveScreen)(pScr, on);

	SCREEN_TYPED_REWRAP(pScr, SaveScreen, int);

	return result;
}

static void
NAME(GetImage)(pDraw, sx, sy, w, h, format, planemask, pdstLine)
	DrawablePtr	pDraw;
	int		sx, sy, w, h;
	unsigned int	format;
	unsigned long	planemask;
	pointer		pdstLine;
{
	ScreenPtr pScr = pDraw->pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, GetImage);

	if (pDraw->type == DRAWABLE_WINDOW)
		DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->GetImage)(pDraw, sx, sy, w, h, format, planemask, (void*)pdstLine);

	SCREEN_REWRAP(pScr, GetImage);
}

static void
NAME(GetSpans)(pDraw, wMax, ppt, pwidth, nspans, pdstStart)
	DrawablePtr	pDraw;
	int		wMax;
	DDXPointPtr	ppt;
	int		*pwidth;
	int		nspans;
	unsigned int	*pdstStart;
{
	ScreenPtr pScr = pDraw->pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, GetSpans);

	if (pDraw->type == DRAWABLE_WINDOW)
		DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->GetSpans)(pDraw, wMax, ppt, pwidth, nspans, (void*)pdstStart);

	SCREEN_REWRAP(pScr, GetSpans);
}

static void
NAME(SourceValidate)(pDraw, x, y, width, height)
	DrawablePtr	pDraw;
	int		x, y, width, height;
{
	ScreenPtr pScr = pDraw->pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, SourceValidate);

	if (pDraw->type == DRAWABLE_WINDOW)
		DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->SourceValidate)(pDraw, x, y, width, height);

	SCREEN_REWRAP(pScr, SourceValidate);
}

static void
NAME(PaintWindowBackground)(pWin, pRegion, what)
	WindowPtr	pWin;
	RegionPtr	pRegion;
	int		what;
{
	ScreenPtr pScr = pWin->drawable.pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, PaintWindowBackground);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->PaintWindowBackground)(pWin, pRegion, what);

	SCREEN_REWRAP(pScr, PaintWindowBackground);
}

static void
NAME(PaintWindowBorder)(pWin, pRegion, what)
	WindowPtr	pWin;
	RegionPtr	pRegion;
	int		what;
{
	ScreenPtr pScr = pWin->drawable.pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, PaintWindowBorder);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->PaintWindowBorder)(pWin, pRegion, what);

	SCREEN_REWRAP(pScr, PaintWindowBorder);
}

static void
NAME(CopyWindow)(pWin, ptOldOrg, pRegion)
	WindowPtr	pWin;
	DDXPointRec	ptOldOrg;
	RegionPtr	pRegion;
{
	ScreenPtr pScr = pWin->drawable.pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, CopyWindow);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->CopyWindow)(pWin, ptOldOrg, pRegion);

	SCREEN_REWRAP(pScr, CopyWindow);
}

static void
NAME(ClearToBackground)(pWin, x, y, w, h, generateExposures)
	WindowPtr	pWin;
	short		x,y;
	unsigned short	w,h;
	Bool		generateExposures;
{
	ScreenPtr pScr = pWin->drawable.pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, ClearToBackground);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->ClearToBackground)(pWin, x, y, w, h, generateExposures);

	SCREEN_REWRAP(pScr, ClearToBackground);
}

static void
NAME(SaveDoomedAreas)(pWin, pObscured, dx, dy)
	WindowPtr	pWin;
	RegionPtr	pObscured;
	int		dx, dy;
{
	ScreenPtr pScr = pWin->drawable.pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, SaveDoomedAreas);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->SaveDoomedAreas)(pWin, pObscured, dx, dy);

	SCREEN_REWRAP(pScr, SaveDoomedAreas);
}

static RegionPtr
NAME(RestoreAreas)(pWin, prgnExposed)
	WindowPtr	pWin;
	RegionPtr	prgnExposed;
{
	ScreenPtr pScr = pWin->drawable.pScreen;
	SCREEN_SETUP(pScr, pPriv);
	RegionPtr result;

	SCREEN_UNWRAP(pScr, pPriv, RestoreAreas);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	result = (*pScr->RestoreAreas)(pWin, prgnExposed);

	SCREEN_TYPED_REWRAP(pScr, RestoreAreas, RegionPtr);

	return result;
}

static Bool
NAME(CreateGC)(pGC)
	GCPtr	pGC;
{
	dfbSSGCPrivPtr pGCPriv = DFB_SS_GC_PRIV(pGC);
	ScreenPtr pScr = pGC->pScreen;
	SCREEN_SETUP(pScr, pPriv);
	Bool result;

	SCREEN_UNWRAP(pScr, pPriv, CreateGC);

	result = (*pScr->CreateGC)(pGC);

	pGCPriv->wrapOps   = NULL;
	pGCPriv->wrapFuncs = pGC->funcs;
	pGC->funcs	   = &dfbSSGCFuncs;

	SCREEN_TYPED_REWRAP(pScr, CreateGC, int);

	return result;
}

static void
NAME(InstallColormap)(pMap)
	ColormapPtr	pMap;
{
	ScreenPtr pScr = pMap->pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, InstallColormap);

	if (pPriv->inGfxMode)
		DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->InstallColormap)(pMap);

	SCREEN_REWRAP(pScr, InstallColormap);
}

static void
NAME(StoreColors)(pMap, ndef, pdef)
	ColormapPtr	pMap;
	int		ndef;
	xColorItem	*pdef;
{
	ScreenPtr pScr = pMap->pScreen;
	SCREEN_SETUP(pScr, pPriv);
    
	SCREEN_UNWRAP(pScr, pPriv, StoreColors);

	if (pPriv->inGfxMode)
		DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pScr->StoreColors)(pMap, ndef, pdef);

	SCREEN_REWRAP(pScr, StoreColors);
}


/*
 * scoSysInfo "wrappers"
 */

static void
NAME(SetGraphics)(pScr)
	ScreenPtr pScr;
{
	SCREEN_SETUP(pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pPriv->pOldSI->SetGraphics)(pScr);

	pPriv->inGfxMode = TRUE;
}

static void
NAME(SetText)(pScr)
	ScreenPtr pScr;
{
	SCREEN_SETUP(pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pPriv->pOldSI->SetText)(pScr);

	DFB_SWITCH_SCREEN(pPriv, 0);

	pPriv->inGfxMode = FALSE;
}

static void
NAME(SaveGState)(pScr)
	ScreenPtr pScr;
{
	SCREEN_SETUP(pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pPriv->pOldSI->SaveGState)(pScr);
}

static void
NAME(RestoreGState)(pScr)
	ScreenPtr pScr;
{
	SCREEN_SETUP(pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	(*pPriv->pOldSI->RestoreGState)(pScr);
}


/*
 * GC Func wrappers
 */

static void
NAME(ValidateGC)(pGC, changes, pDraw)
	GCPtr		pGC;
	Mask		changes;
	DrawablePtr	pDraw;
{
	GC_FUNC_SETUP(pGC, pGCPriv);

	GC_FUNC_UNWRAP(pGC, pGCPriv);

	(*pGC->funcs->ValidateGC)(pGC, changes, pDraw);
    
	pGCPriv->wrapOps = NULL;
	if (pDraw->type == DRAWABLE_WINDOW && ((WindowPtr)pDraw)->viewable) {
		WindowPtr   pWin = (WindowPtr)pDraw;
		RegionPtr   pRegion;

		pRegion = &pWin->clipList;
		if (pGC->subWindowMode == IncludeInferiors)
			pRegion = &pWin->borderClip;
		if ((*pDraw->pScreen->RegionNotEmpty)(pRegion))
			pGCPriv->wrapOps = pGC->ops;
	}

	GC_FUNC_REWRAP(pGC, pGCPriv);
}

static void
NAME(ChangeGC)(pGC, mask)
	GCPtr		pGC;
	unsigned long	mask;
{
	GC_FUNC_SETUP(pGC, pGCPriv);

	GC_FUNC_UNWRAP(pGC, pGCPriv);

	(*pGC->funcs->ChangeGC)(pGC, mask);
    
	GC_FUNC_REWRAP(pGC, pGCPriv);
}

static void
NAME(CopyGC)(pGCSrc, mask, pGCDst)
	GCPtr		pGCSrc, pGCDst;
	unsigned long	mask;
{
	GC_FUNC_SETUP(pGCDst, pGCPriv);

	GC_FUNC_UNWRAP(pGCDst, pGCPriv);

	(*pGCDst->funcs->CopyGC)(pGCSrc, mask, pGCDst);
    
	GC_FUNC_REWRAP(pGCDst, pGCPriv);
}

static void
NAME(DestroyGC)(pGC)
	GCPtr   pGC;
{
	GC_FUNC_SETUP(pGC, pGCPriv);

	GC_FUNC_UNWRAP(pGC, pGCPriv);

	(*pGC->funcs->DestroyGC)(pGC);
    
	GC_FUNC_REWRAP(pGC, pGCPriv);
}

static void
NAME(ChangeClip)(pGC, type, pvalue, nrects)
	GCPtr	pGC;
	pointer	pvalue;
	int	type, nrects;
{
	GC_FUNC_SETUP(pGC, pGCPriv);

	GC_FUNC_UNWRAP(pGC, pGCPriv);

	(*pGC->funcs->ChangeClip)(pGC, type, pvalue, nrects);

	GC_FUNC_REWRAP(pGC, pGCPriv);
}

static void
NAME(CopyClip)(pGCDst, pGCSrc)
	GCPtr	pGCDst, pGCSrc;
{
	GC_FUNC_SETUP(pGCDst, pGCPriv);

	GC_FUNC_UNWRAP(pGCDst, pGCPriv);

	(*pGCDst->funcs->CopyClip)(pGCDst, pGCSrc);

	GC_FUNC_REWRAP(pGCDst, pGCPriv);
}

static void
NAME(DestroyClip)(pGC)
	GCPtr	pGC;
{
	GC_FUNC_SETUP(pGC, pGCPriv);

	GC_FUNC_UNWRAP(pGC, pGCPriv);

	(*pGC->funcs->DestroyClip)(pGC);

	GC_FUNC_REWRAP(pGC, pGCPriv);
}


/*
 * GC Op wrappers
 */

static void
NAME(FillSpans)(pDraw, pGC, nInit, pptInit, pwidthInit, fSorted)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		nInit;
	DDXPointPtr	pptInit;
	int		*pwidthInit;
	int 		fSorted;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->FillSpans)(pDraw,pGC,nInit,pptInit,pwidthInit,fSorted);

	GC_OP_REWRAP(pGC);
}

static void
NAME(SetSpans)(pDraw, pGC, psrc, ppt, pwidth, nspans, fSorted)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		*psrc;
	DDXPointPtr	ppt;
	int		*pwidth;
	int		nspans;
	int		fSorted;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->SetSpans)(pDraw, pGC, (void*)psrc, ppt, pwidth, nspans, fSorted);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PutImage)(pDraw, pGC, depth, x, y, w, h, leftPad, format, pBits)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		depth;
	int		x, y, w, h;
	int		format;
	char		*pBits;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PutImage)(pDraw,pGC,depth,x,y,w,h,leftPad,format,pBits);

	GC_OP_REWRAP(pGC);
}

static RegionPtr
NAME(CopyArea)(pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty)
	DrawablePtr	pSrc, pDst;
	GCPtr		pGC;
	int		srcx, srcy;
	int		w, h;
	int		dstx, dsty;
{
	GC_OP_SETUP(pDst, pGC, pScr, pPriv);
	RegionPtr rgn;

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	rgn = (*pGC->ops->CopyArea)(pSrc,pDst,pGC,srcx,srcy,w,h,dstx,dsty);

	GC_OP_REWRAP(pGC);

	return rgn;
}

static RegionPtr
NAME(CopyPlane)(pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, plane)
	DrawablePtr	pSrc, pDst;
	GC		*pGC;
	int		srcx, srcy;
	int		w, h;
	int		dstx, dsty;
	unsigned long	plane;
{
	RegionPtr rgn;
	GC_OP_SETUP(pDst, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	rgn = (*pGC->ops->CopyPlane)(pSrc, pDst, pGC, srcx, srcy,
				     w, h, dstx, dsty, plane);

	GC_OP_REWRAP(pGC);

	return rgn;
}

static void
NAME(PolyPoint)(pDraw, pGC, mode, npt, pptInit)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		mode;
	int		npt;
	xPoint		*pptInit;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolyPoint)(pDraw, pGC, mode, npt, pptInit);

	GC_OP_REWRAP(pGC);
}

static void
NAME(Polylines)(pDraw, pGC, mode, npt, pptInit)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		mode;
	int		npt;
	DDXPointPtr	pptInit;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->Polylines)(pDraw, pGC, mode, npt, pptInit);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PolySegment)(pDraw, pGC, nseg, pSegs)
	DrawablePtr	pDraw;
	GCPtr 		pGC;
	int		nseg;
	xSegment	*pSegs;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolySegment)(pDraw, pGC, nseg, pSegs);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PolyRectangle)(pDraw, pGC, nrects, pRects)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		nrects;
	xRectangle	*pRects;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolyRectangle)(pDraw, pGC, nrects, pRects);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PolyArc)(pDraw, pGC, narcs, parcs)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		narcs;
	xArc		*parcs;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolyArc)(pDraw, pGC, narcs, parcs);

	GC_OP_REWRAP(pGC);
}

static void
NAME(FillPolygon)(pDraw, pGC, shape, mode, count, pPts)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		shape, mode;
	int		count;
	DDXPointPtr	pPts;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, count, pPts);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PolyFillRect)(pDraw, pGC, nrectFill, prectInit)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		nrectFill;
	xRectangle	*prectInit;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolyFillRect)(pDraw, pGC, nrectFill, prectInit);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PolyFillArc)(pDraw, pGC, narcs, parcs)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		narcs;
	xArc		*parcs;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolyFillArc)(pDraw, pGC, narcs, parcs);

	GC_OP_REWRAP(pGC);
}

static int
NAME(PolyText8)(pDraw, pGC, x, y, count, chars)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		x, y;
	int 		count;
	char		*chars;
{
	int ret;
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	ret = (*pGC->ops->PolyText8)(pDraw, pGC, x, y, count, chars);

	GC_OP_REWRAP(pGC);

	return ret;
}

static int
NAME(PolyText16)(pDraw, pGC, x, y, count, chars)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		x, y;
	int		count;
	unsigned short	*chars;
{
	int ret;
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	ret = (*pGC->ops->PolyText16)(pDraw, pGC, x, y, count, chars);

	GC_OP_REWRAP(pGC);

	return ret;
}

static void
NAME(ImageText8)(pDraw, pGC, x, y, count, chars)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		x, y;
	int 		count;
	char		*chars;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->ImageText8)(pDraw, pGC, x, y, count, chars);

	GC_OP_REWRAP(pGC);
}

static void
NAME(ImageText16)(pDraw, pGC, x, y, count, chars)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int		x, y;
	int		count;
	unsigned short	*chars;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->ImageText16)(pDraw, pGC, x, y, count, chars);

	GC_OP_REWRAP(pGC);
}

static void
NAME(ImageGlyphBlt)(pDraw, pGC, x, y, nglyph, ppci, pglyphBase)
	DrawablePtr	pDraw;
	GC 		*pGC;
	int 		x, y;
	unsigned long	nglyph;
	CharInfoPtr	*ppci;
	pointer 	pglyphBase;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->ImageGlyphBlt)(pDraw, pGC, x, y, nglyph, ppci, pglyphBase);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PolyGlyphBlt)(pDraw, pGC, x, y, nglyph, ppci, pglyphBase)
	DrawablePtr	pDraw;
	GCPtr		pGC;
	int 		x, y;
	unsigned long	nglyph;
	CharInfoPtr	*ppci;
	char 		*pglyphBase;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PolyGlyphBlt)(pDraw, pGC, x, y, nglyph, ppci, (void*)pglyphBase);

	GC_OP_REWRAP(pGC);
}

static void
NAME(PushPixels)(pGC, pBitMap, pDraw, w, h, x, y)
	GCPtr		pGC;
	PixmapPtr	pBitMap;
	DrawablePtr	pDraw;
	int		w, h, x, y;
{
	GC_OP_SETUP(pDraw, pGC, pScr, pPriv);

	DFB_SWITCH_SCREEN(pPriv, pScr->myNum);

	GC_OP_UNWRAP(pGC);

	(*pGC->ops->PushPixels)(pGC, pBitMap, pDraw, w, h, x, y);

	GC_OP_REWRAP(pGC);
}
