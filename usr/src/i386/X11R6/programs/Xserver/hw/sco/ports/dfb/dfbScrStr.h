/*
 * @(#) dfbScrStr.h 12.1 95/05/09 
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
 * dfbScrStr.h
 *
 * dfb screen private structure.
 */

#include "ddxScreen.h"

typedef struct _dfbScrnPriv {
	pointer		fbBase;		 /* frame buffer base */
	ColormapPtr	currentCmap;	 /* installed colormap */
	int		dacShift;	 /* 16 - rgb significant bits */
	Bool		inGfxMode;	 /* in graphics mode ? */
	grafData	*pGraf;		 /* grafinfo data */
	codeType	*SetColorFunc;	 /* grafinfo function */
	codeType	*BlankScrFunc;	 /* grafinfo function */
	codeType	*UnblankScrFunc; /* grafinfo function */
} dfbScrnPriv, *dfbScrnPrivPtr;

#define DFB_SCREEN_PRIV(pscreen) ((dfbScrnPrivPtr) \
			((pscreen)->devPrivates[dfbScreenPrivateIndex].ptr))

extern int dfbScreenPrivateIndex;

