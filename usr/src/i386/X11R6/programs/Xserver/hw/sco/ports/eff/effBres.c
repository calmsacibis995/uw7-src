/*
 *	@(#) effBres.c 11.1 97/10/22
 *
 * Modification History
 *
 * S005, 12-Dec-92, mikep
 *	remove parmeter checks, they are now done in nfb
 * S004, 23-Nov-92, staceyc
 * 	range checks on error registers to avoid truncating MSBits of error
 *	terms when writing to 8514/A ports
 * S003, 28-Aug-91, staceyc
 * 	reworked command queue
 * S002, 15-Aug-91, staceyc
 * 	include defs.h file
 * S001, 24-Jun-91, staceyc
 * 	fixed parameters to match nfb
 * S000, 21-Jun-91, staceyc
 * 	created
 */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "region.h"
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "nfbProcs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

extern int effRasterOps[];

void
effFillZeroSeg(
	GC *gc,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x,
	int y,
	int e,
	int e1,
	int e2,
	int len)
{
	unsigned short command;
	unsigned char alu;
	unsigned long planemask;
	unsigned int fg;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(gc);

	/*
	 * e (0x92E8) and e1 (0x8AE8) ports have 12 bits and a sign bit,
	 * e2 (0x8EE8) has 13 bits and a sign bit, use gen to draw any
	 * line that would have its error terms truncated if they were
	 * written to these registers
	 */
	if (e > 0xfff || e1 > 0xfff || e < -0x1000 || e1 < -0x1000 ||
	    e2 > 0x1fff || e2 < -0x2000)
	{
		genSolidZeroSeg(gc, pDraw, signdx, signdy, axis, x, y, e, e1,
		    e2, len);
		return;
	}

	alu = pGCPriv->rRop.alu;
	planemask = pGCPriv->rRop.planemask;
	fg = pGCPriv->rRop.fg;

	if (axis == X_AXIS)
		command = 0x2017;           /* draw bres. line on X, W, LPN, */
	else
		command = 0x2057;                              /* same, on Y */
	if (signdx > 0)
		command |= 0x0020;
	if (signdy > 0)
		command |= 0x0080;

	EFF_CLEAR_QUEUE(8);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_ONES);
	EFF_SETET(e);
	EFF_SETK1(e1);
	EFF_SETK2(e2);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_SETCOL1(fg);

	EFF_CLEAR_QUEUE(4);
	/* going to use LAST PEL NULL in Line command to brecon */
	EFF_SETLX(len);
	EFF_SETX0(x);
	EFF_SETY0(y);

	EFF_COMMAND(command);
}
