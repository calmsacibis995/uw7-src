/*
 * @(#) dfbSwitch.h 12.1 95/05/09 
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
 * dfbSwitch.h
 *
 * dfb SwitchScreen definitions.
 */

#include "grafinfo.h"
#include "scoext.h"

/*
 * screen privates
 */

typedef struct _dfbSSScrnPriv {
	scoScreenInfo	*pOldSI;
	scoScreenInfo	*pNewSI;
	Bool		inGfxMode;	/* in graphics mode ? */
	grafData	*pGraf;		/* grafinfo data */
	codeType	*SwitchScrFunc;	/* grafinfo function */

	Bool		(*CloseScreen)();
	Bool		(*SaveScreen)();
	void		(*GetImage)();
	void		(*GetSpans)();
	void		(*SourceValidate)();
	void		(*PaintWindowBackground)();
	void		(*PaintWindowBorder)();
	void		(*CopyWindow)();
	void		(*ClearToBackground)();
	void		(*SaveDoomedAreas)();
	RegionPtr	(*RestoreAreas)();
	Bool		(*CreateGC)();
	void		(*InstallColormap)();
	void		(*StoreColors)();
} dfbSSScrnPriv, *dfbSSScrnPrivPtr;

#define DFB_SS_SCREEN_PRIV(pScr) ((dfbSSScrnPrivPtr) \
			((pScr)->devPrivates[dfbSSScrnPrivIndex].ptr))

extern int dfbSSScrnPrivIndex;

/*
 * GC privates
 */

typedef struct _dfbSSGCPriv {
	GCFuncs		*wrapFuncs;
	GCOps		*wrapOps;
} dfbSSGCPriv, *dfbSSGCPrivPtr;

#define DFB_SS_GC_PRIV(pGC) ((dfbSSGCPrivPtr) \
			((pGC)->devPrivates[dfbSSGCPrivIndex].ptr))

extern int dfbSSGCPrivIndex;

/*
 * switch screens
 */

#define	DFB_SWITCH_SCREEN(pPriv, scrnum) {		\
	if ((scrnum) != dfbScreenNum) {			\
		grafRunFunction((pPriv)->pGraf,		\
				(pPriv)->SwitchScrFunc,	\
				NULL, (scrnum));	\
		dfbScreenNum = (scrnum);		\
	}						\
}

extern int dfbScreenNum;
