/*
 *	@(#) effFillSp.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Sat Dec 12 14:09:23 PST 1992	mikep@sco.com
 *	- Create file and function.  At least 15% faster than gen.
 *
 */
#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

void
effSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int u_npts )
{
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    register int npts = u_npts; 	/* We need this to be signed */

    EFF_CLEAR_QUEUE(5);
    EFF_SETMODE(EFF_M_ONES);
    EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[pGCPriv->rRop.alu]);
    EFF_SETCOL1(pGCPriv->rRop.fg);
    EFF_PLNWENBL(pGCPriv->rRop.planemask);
    EFF_PLNRENBL(EFF_RPLANES);

#define body \
	EFF_SETLX(*pwidth-1); \
	EFF_SETX0(ppt->x); \
	EFF_SETY0(ppt->y); \
	EFF_COMMAND(EFF_CMD_VECTOR | EFF_CMD_DO_ACCESS  \
		    | EFF_CMD_USE_C_DIR | EFF_CMD_ACCRUE_PELS \
		    | EFF_CMD_WRITE); 

    /*
     * inline code will really speed this up.  Maybe unroll the
     * loop more?
     */
    while((npts -= 2) >= 0)
	{ 
	EFF_CLEAR_QUEUE(8);
	body 
	ppt++; pwidth++; 
	body 
	ppt++; pwidth++; 
	}
    
    if (npts & 1)
	{
	EFF_CLEAR_QUEUE(4);
	body
	}

    return;
}
