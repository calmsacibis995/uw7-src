#ident	"@(#)ihvkit:display/lfb256/lfbLine.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <lfb.h>

/*	HARDWARE POINT PLOTTING		*/
/*		OPTIONAL		*/

SIBool lfbPlotPoints(count, ptsIn)
SIint32 count;
SIPointP ptsIn;

{
    Pixel fg = lfb_cur_GStateP->SGfg, nfg = ~fg;
    SIPointP ptP = ptsIn;
    int i = count;
    int mode = lfb_cur_GStateP->SGmode;
    Pixel msk = lfb_cur_GStateP->SGpmask, nmsk = ~msk;

    if ((mode == GXnoop) ||
	(msk == 0))
	return(SI_SUCCEED);

    /* 
     * Flush the graphics hardware
     *
     */
    if (lfbVendorFlush)
	(*lfbVendorFlush)();

    if (msk == PMSK) {
	switch (mode) {
	  case GXclear:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) = 0;
		ptP++;
	    }
	    break;

	  case GXand:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) &= fg;
		ptP++;
	    }
	    break;

	  case GXandReverse:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		     fg & ~ *(ScreenAddr(ptP->x, ptP->y));
		ptP++;
	    }
	    break;

	  case GXcopy:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) = fg;
		ptP++;
	    }
	    break;

	  case GXandInverted:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) &= nfg;
		ptP++;
	    }
	    break;

/*
 * This case was handled above
 *	  case GXnoop:
 *
 */

	  case GXxor:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) ^= fg;
		ptP++;
	    }
	    break;

	  case GXor:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) |= fg;
		ptP++;
	    }
	    break;

	  case GXnor:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ~(fg | *(ScreenAddr(ptP->x, ptP->y)));
		ptP++;
	    }
	    break;

	  case GXequiv:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) ^= nfg;
		ptP++;
	    }
	    break;

	  case GXinvert:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ~ *(ScreenAddr(ptP->x, ptP->y));
		ptP++;
	    }
	    break;

	  case GXorReverse:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    fg | ~ *(ScreenAddr(ptP->x, ptP->y));
		ptP++;
	    }
	    break;

	  case GXcopyInverted:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) = nfg;
		ptP++;
	    }
	    break;

	  case GXorInverted:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) |= nfg;
		ptP++;
	    }
	    break;

	  case GXnand:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) = 
		    ~(fg & *(ScreenAddr(ptP->x, ptP->y)));
		ptP++;
	    }
	    break;

	  case GXset:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) = PMSK;
		ptP++;
	    }
	    break;
	}
    }
    else {
	switch (mode) {
	  case GXclear:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) &= nmsk;
		ptP++;
	    }
	    break;

	  case GXand:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & *(ScreenAddr(ptP->x, ptP->y)) & fg));
		ptP++;
	    }
	    break;

	  case GXandReverse:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & ~ *(ScreenAddr(ptP->x, ptP->y)) & fg));
		ptP++;
	    }
	    break;

	  case GXcopy:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & fg));
		ptP++;
	    }
	    break;

	  case GXandInverted:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & *(ScreenAddr(ptP->x, ptP->y)) & nfg));
		ptP++;
	    }
	    break;

/*
 * This case was handled above
 *	  case GXnoop:
 *
 */

	  case GXxor:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & (*(ScreenAddr(ptP->x, ptP->y)) ^ fg)));
		ptP++;
	    }
	    break;

	  case GXor:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & (*(ScreenAddr(ptP->x, ptP->y)) | fg)));
		ptP++;
	    }
	    break;

	  case GXnor:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & ~(*(ScreenAddr(ptP->x, ptP->y)) | fg)));
		ptP++;
	    }
	    break;

	  case GXequiv:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & (*(ScreenAddr(ptP->x, ptP->y)) ^ nfg)));
		ptP++;
	    }
	    break;

	  case GXinvert:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & ~ *(ScreenAddr(ptP->x, ptP->y))));
		ptP++;
	    }
	    break;

	  case GXorReverse:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & (~ *(ScreenAddr(ptP->x, ptP->y)) | fg)));
		ptP++;
	    }
	    break;

	  case GXcopyInverted:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & nfg));
		ptP++;
	    }
	    break;

	  case GXorInverted:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & (*(ScreenAddr(ptP->x, ptP->y)) | nfg)));
		ptP++;
	    }
	    break;

	  case GXnand:
	    while (i--) {
		*(ScreenAddr(ptP->x, ptP->y)) =
		    ((nmsk & *(ScreenAddr(ptP->x, ptP->y))) |
		     (msk & ~ (*(ScreenAddr(ptP->x, ptP->y)) & fg)));
		ptP++;
	    }
	    break;

	  case GXset:
	    while (i--) {
		ptP++;
		*(ScreenAddr(ptP->x, ptP->y)) |= msk;
	    }
	    break;
	}
    }

    return(SI_SUCCEED);
}


/*	HARDWARE LINE DRAWING		*/
/*		OPTIONAL		*/

SIvoid lfbLineClip(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;

{
}

SIBool lfbThinLines(count, ptsIn)
SIint32 count;
SIPointP ptsIn;

{
    return(SI_FAIL);
}

SIBool lfbThinSegments(count, ptsIn)
SIint32 count;
SIPointP ptsIn;

{
    return(SI_FAIL);
}

SIBool lfbThinRect(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;

{
    return(SI_FAIL);
}
