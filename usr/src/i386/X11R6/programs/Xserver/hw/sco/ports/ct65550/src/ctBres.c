/*
 *	@(#)ctBres.c	11.1	10/22/97	12:34:37
 *	@(#) ctBres.c 61.1 97/02/26 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctBres.c 61.1 97/02/26 "

#include "X.h"
#include "windowstr.h"
#include "scrnintstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbRop.h"

#include "ctDefs.h"
#include "ctMacros.h"

/*******************************************************************************

				Private Routines

*******************************************************************************/

#define	InvertWithMask(dst, mask)	((dst) ^ (mask))

#define CT_BRES_SETUP() {						\
	xinc = plines->signdx;						\
	yinc = ctPriv->fbStride * plines->signdy;			\
	xyinc = xinc + yinc;						\
	e = plines->e;							\
	e1 = plines->e1;						\
	e2 = plines->e2;						\
	len = plines->len;						\
	pdst = CT_SCREEN_ADDRESS(ctPriv, plines->x1, plines->y1);	\
}

#define CT_BRES_INCREMENT(inc) {					\
	if (e > 0) {							\
		pdst += xyinc;						\
		e += e2;						\
	} else {							\
		pdst += (inc);						\
		e += e1;						\
	}								\
}

static void
ctSolidZeroSegsFB(plines, nlines, alu, fg, planemask, pDraw)
BresLinePtr plines;
int nlines;
CT_PIXEL alu;
CT_PIXEL fg;
CT_PIXEL planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	int xinc, yinc, xyinc, len, e, e1, e2;
	CT_PIXEL *pdst;
	CT_PIXEL *fbPtr = ctPriv->fbPointer;

	CT_WAIT_FOR_IDLE();

	switch (alu) {
	case GXclear:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = ClearWithMask(*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = ClearWithMask(*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXand:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnAND(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnAND(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXandReverse:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnANDREVERSE(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnANDREVERSE(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXcopy:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnCOPY(fg, 0),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnCOPY(fg, 0),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXandInverted:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnANDINVERTED(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnANDINVERTED(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXnoop:
		break;
	case GXxor:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnXOR(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnXOR(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXor:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnOR(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnOR(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXnor:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnNOR(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnNOR(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXequiv:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnEQUIV(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnEQUIV(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXinvert:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = InvertWithMask(*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = InvertWithMask(*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXorReverse:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnORREVERSE(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnORREVERSE(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXcopyInverted:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnCOPYINVERTED(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnCOPYINVERTED(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXorInverted:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnORINVERTED(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnORINVERTED(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXnand:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = RopWithMask(fnNAND(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = RopWithMask(fnNAND(fg, *pdst),
						*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	case GXset:
		while (nlines--) {
			CT_BRES_SETUP();
			if (plines->axis == X_AXIS) {
			    while (len--) {
				*pdst = SetWithMask(*pdst, planemask);
				CT_BRES_INCREMENT(xinc);
			    }
			} else {
			    while (len--) {
				*pdst = SetWithMask(*pdst, planemask);
				CT_BRES_INCREMENT(yinc);
			    }
			}
			plines++;
		}
		break;
	}

	CT_FLUSH_BITBLT(fbPtr);
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

/*
 * CT(SolidZeroSegs)() - Draw solid Bresenham line segments.
 *
 * NOTE: (nfbGCPrivPtr->ops->FillZeroSegs)() can call TWO (2) different function
 * prototypes depending on the value of:
 *
 *	NFB_SCREEN_PRIV(pDraw->pScreen)->nfb_options & NFB_POLYBRES;
 *
 * This option is specified by calling nfbSetOptions() at ScreenInit() time. 
 * When NFB_POLYBRES is specified, the line segments are all clipped before any
 * drawing takes place. By specifying NFB_POLYBRES, we eliminate a function call
 * and some additional overhead for each line segment. This 'should' be faster.
 * If NFB_POLYBRES is not specified, we need to implement CT(SolidZeroSeg)()
 * (prototype in ctProcs.h) and assign it to nfbGCPrivPtr->ops->FillZeroSegs.
 * CT(SolidZeroSeg)() draws only one segment at a time.
 */
void
CT(SolidZeroSegs)(pGC, pDraw, plines, nlines)
GCPtr pGC;
DrawablePtr pDraw;
BresLinePtr plines;
int nlines;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int xinc, yinc, xyinc, len, e, e1, e2;
	unsigned char alu = pGCPriv->rRop.alu;
	CT_PIXEL fg = (CT_PIXEL)pGCPriv->rRop.fg;
	CT_PIXEL planemask;
	CT_PIXEL *pdst;
	CT_PIXEL *fbPtr = ctPriv->fbPointer;

	planemask = (CT_PIXEL)(pGCPriv->rRop.planemask & ctPriv->allPlanes);
	if (planemask == (CT_PIXEL)0) {
		/*
		 * No planes.
		 */
		return;
	}

	if (planemask != ctPriv->allPlanes) {
		/*
		 * Call the frame buffer routine to handle planemask.
		 */
		ctSolidZeroSegsFB(plines, nlines, alu, fg, planemask,
				pDraw);
		return;
	}

	switch (alu) {
	case GXnoop:
		return;
	case GXclear:
		/* implicit alu = GXcopy */
		fg = (CT_PIXEL)0;
		break;
	case GXset:
		/* implicit alu = GXcopy */
		fg = (CT_PIXEL)~0;
		break;
	case GXcopy:
		break;
	default:
		/*
		 * Call the frame buffer routine to handle non-copy ALU's.
		 */
		ctSolidZeroSegsFB(plines, nlines, alu, fg, planemask, pDraw);
		return;
	}

	CT_WAIT_FOR_IDLE();

	while (nlines--) {
		CT_BRES_SETUP();
		if (plines->axis == X_AXIS) {
			while (len--) {
				*pdst = fg;
				CT_BRES_INCREMENT(xinc);
			}
		} else {
			while (len--) {
				*pdst = fg;
				CT_BRES_INCREMENT(yinc);
			}
		}
		plines++;
	}

	CT_FLUSH_BITBLT(fbPtr);
}
